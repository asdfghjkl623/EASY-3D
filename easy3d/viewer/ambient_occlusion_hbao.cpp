/*
*	Copyright (C) 2015 by Liangliang Nan (liangliang.nan@gmail.com)
*	https://3d.bk.tudelft.nl/liangliang/
*
*	This file is part of Easy3D. If it is useful in your research/work,
*   I would be grateful if you show your appreciation by citing it:
*   ------------------------------------------------------------------
*           Liangliang Nan.
*           Easy3D: a lightweight, easy-to-use, and efficient C++
*           library for processing and rendering 3D data. 2018.
*   ------------------------------------------------------------------
*
*	Easy3D is free software; you can redistribute it and/or modify
*	it under the terms of the GNU General Public License Version 3
*	as published by the Free Software Foundation.
*
*	Easy3D is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#include <easy3d/viewer/ambient_occlusion_hbao.h>

#include <random>

#include <easy3d/viewer/camera.h>
#include <easy3d/viewer/shader_program.h>
#include <easy3d/viewer/framebuffer_object.h>
#include <easy3d/viewer/opengl_error.h>
#include <easy3d/viewer/opengl_info.h>
#include <easy3d/viewer/primitives.h>
#include <easy3d/viewer/shader_manager.h>
#include <easy3d/viewer/transform.h>
#include <easy3d/viewer/model.h>
#include <easy3d/viewer/drawable.h>
#include <easy3d/viewer/opengl.h>
#include <easy3d/core/constant.h>

#include <easy3d/viewer/read_pixel.h>
#include <easy3d/viewer/setting.h>


using namespace easy3d;

const int  NUM_MRT = 8;
const int  HBAO_RANDOM_SIZE = 4;
const int  HBAO_RANDOM_ELEMENTS = HBAO_RANDOM_SIZE * HBAO_RANDOM_SIZE;
const int  MAX_SAMPLES = 8;

const int  KERNEL_SIZE = 64;
const int  NOISE_RES = 4;
const int  LOW_RES_RATIO = 4;

const int  AO_RANDOMTEX_SIZE = 4;

//FBO
struct {
	GLuint
		hbao2_deinterleave,
		hbao2_calc;

}fbos;

struct {
	GLuint
		hbao_random,
		hbao_randomview[MAX_SAMPLES],
		hbao2_depthview[HBAO_RANDOM_ELEMENTS],
		hbao2_deptharray,
		hbao2_resultarray;
} textures;

//uniforms
struct hbao {
	vec4 projInfo;
	int  projOrtho;

	vec2 InvFullResolution;
	vec2 InvQuarterResolution;

	float NegInvR2;
	float RadiusToScreen;
	float PowExponent;
	float NDotVBias;
	float AOMultiplier;

	vec4    float2Offsets[AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];
	vec4    jitters[AO_RANDOMTEX_SIZE*AO_RANDOMTEX_SIZE];

}hbaoData;




// Properties

vec4  hbaoRandom[HBAO_RANDOM_ELEMENTS * MAX_SAMPLES];
signed short hbaoRandomShort[HBAO_RANDOM_ELEMENTS*MAX_SAMPLES * 4];


GLfloat lerp(GLfloat a, GLfloat b, GLfloat f)
{
	return a + f * (b - a);
}


namespace easy3d {


	AmbientOcclusion_HBAO::AmbientOcclusion_HBAO(Camera* cam)
		: camera_(cam)
		, algorithm_(SSAO_NONE)
        , samples_(0)
		, width_(1024)
		, height_(768)
		, radius_(2.0f)
		, intensity_(1.5f)
		, bias_(0.1f)
		, sharpness_(40.0f)
		, shaderDrawScene(nullptr)
		, shaderLinearDepth(nullptr)
		, shaderLinearDepth_msaa(nullptr)
        , program_view_normal_(nullptr)
		, hbao_calc_blur(nullptr)
		, hbao_blur(nullptr)
		, hbao_blur2(nullptr)
        , program_ssao_(nullptr)
		, hbao2_calc_blur(nullptr)
		, hbao2_deinterleave(nullptr)
		, hbao2_reinterleave_blur(nullptr)
		, fbo_scene_(nullptr)
		, fbo_depthlinear_(nullptr)
		, fbo_viewnormal_(nullptr)
		, fbo_hbao_calc_(nullptr)
		, fbo_hbao2_deinterleave_(nullptr)
		, fbo_hbao2_calc_(nullptr)
	{
	}


	AmbientOcclusion_HBAO::~AmbientOcclusion_HBAO() {
		if (fbo_scene_) { delete fbo_scene_; fbo_scene_ = nullptr; }
		if (fbo_depthlinear_) { delete fbo_depthlinear_; fbo_depthlinear_ = nullptr; }
		if (fbo_viewnormal_) { delete fbo_viewnormal_; fbo_viewnormal_ = nullptr; }
		if (fbo_hbao_calc_) { delete fbo_hbao_calc_; fbo_hbao_calc_ = nullptr; }
		if (fbo_hbao2_deinterleave_) { delete fbo_hbao2_deinterleave_; fbo_hbao2_deinterleave_ = nullptr; }
		if (fbo_hbao2_calc_) { delete fbo_hbao2_calc_; fbo_hbao2_calc_ = nullptr; }
		easy3d_debug_gl_error;
	}


    GLint defaultFBO = 0;

	void AmbientOcclusion_HBAO::init_programs() {
		std::vector<ShaderProgram::Attribute> attributes = {
			ShaderProgram::Attribute(ShaderProgram::POSITION, "vtx_position"),
			ShaderProgram::Attribute(ShaderProgram::NORMAL, "vtx_normal"),
			ShaderProgram::Attribute(ShaderProgram::COLOR, "vtx_color")
		};
		if (!shaderDrawScene)
			shaderDrawScene = ShaderManager::create_program_from_files(
				"HBAO+/scene.vs", "HBAO+/scene.fs", "", "", "", "", attributes);

		if (!shaderLinearDepth)
			shaderLinearDepth = ShaderManager::create_program_from_files(
				"HBAO+/fullscreenquad.vs", "HBAO+/depthlinearize.fs", "", "", "#define DEPTHLINEARIZE_MSAA 0\n");

		if (!shaderLinearDepth_msaa)
			shaderLinearDepth_msaa = ShaderManager::create_program_from_files(
				"HBAO+/fullscreenquad.vs", "HBAO+/depthlinearize.fs", "", "", "#define DEPTHLINEARIZE_MSAA 1\n");

        if (!program_view_normal_)
            program_view_normal_ = ShaderManager::create_program_from_files(
				"HBAO+/fullscreenquad.vs", "HBAO+/viewnormal.fs");

		if (!hbao_calc_blur)
			hbao_calc_blur = ShaderManager::create_program_from_files(
				"HBAO+/fullscreenquad.vs", "HBAO+/hbao.fs", "", "", "#define AO_DEINTERLEAVED 0\n#define AO_BLUR 1\n");

		if (!hbao_blur)
			hbao_blur = ShaderManager::create_program_from_files(
				"HBAO+/fullscreenquad.vs", "HBAO+/blur.fs", "", "", "#define AO_BLUR_PRESENT 0\n");

		if (!hbao_blur2)
			hbao_blur2 = ShaderManager::create_program_from_files(
				"HBAO+/fullscreenquad.vs", "HBAO+/blur.fs", "", "", "#define AO_BLUR_PRESENT 1\n");

        if (!program_ssao_)
            program_ssao_ = ShaderManager::create_program_from_files("HBAO+/fullscreenquad.vs", "HBAO+/ssao.fs");

		if (!hbao2_calc_blur)
			hbao2_calc_blur = ShaderManager::create_program_from_files(
				"HBAO+/fullscreenquad.vs", "HBAO+/hbao.fs", "HBAO+/fullscreenquad.gs", "", "#define AO_DEINTERLEAVED 1\n#define AO_BLUR 1\n");

		if (!hbao2_deinterleave)
			hbao2_deinterleave = ShaderManager::create_program_from_files(
				"HBAO+/fullscreenquad.vs", "HBAO+/deinter.fs");

		if (!hbao2_reinterleave_blur)
			hbao2_reinterleave_blur = ShaderManager::create_program_from_files(
				"HBAO+/fullscreenquad.vs", "HBAO+/reinter.fs", "", "", "#define AO_BLUR 1\n");
	}


	void AmbientOcclusion_HBAO::init_framebuffer(int width, int height, int samples) {
		if (fbo_scene_)
			delete fbo_scene_;
		fbo_scene_ = new FramebufferObject(width, height, samples);
		fbo_scene_->add_color_texture();
		fbo_scene_->add_depth_texture();

		//////////////////////////////////////////////////////////////////////////

		if (fbo_depthlinear_)
			delete fbo_depthlinear_;
		fbo_depthlinear_ = new FramebufferObject(width, height, 0);
		fbo_depthlinear_->add_color_texture(GL_R32F, GL_RED);

		//////////////////////////////////////////////////////////////////////////

		if (fbo_viewnormal_)
			delete fbo_viewnormal_;
		fbo_viewnormal_ = new FramebufferObject(width, height, 0);
		fbo_viewnormal_->add_color_texture(GL_RGBA8);

        easy3d_debug_gl_error;
        easy3d_debug_frame_buffer_error;

		//////////////////////////////////////////////////////////////////////////

		GLenum formatAO = GL_RG16F;
		GLint swizzle[4] = { GL_RED,GL_GREEN,GL_ZERO,GL_ZERO };

        float gl_version = OpenglInfo::gl_version_number();

		static GLuint hbao_result = 0;
		glGenTextures(1, &hbao_result);
		glBindTexture(GL_TEXTURE_2D, hbao_result);
        if (gl_version >= 4.2f) // "glTexStorage2D" requires 4.2 or above
            glTexStorage2D(GL_TEXTURE_2D, 1, formatAO, width, height);
        else
        {// see "glTexStorage2D" at https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexStorage2D.xhtml
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, formatAO, width, height, 0, GL_RG, GL_FLOAT, nullptr);
        }
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);

		static GLuint hbao_blur = 0;
		glGenTextures(1, &hbao_blur);
		glBindTexture(GL_TEXTURE_2D, hbao_blur);
        if (gl_version >= 4.2f) // "glTexStorage2D" requires 4.2 or above
            glTexStorage2D(GL_TEXTURE_2D, 1, formatAO, width, height);
        else
        {// see "glTexStorage2D" at https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexStorage2D.xhtml
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, formatAO, width, height, 0, GL_RG, GL_FLOAT, nullptr);
        }
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);

		if (fbo_hbao_calc_)
			delete fbo_hbao_calc_;
		fbo_hbao_calc_ = new FramebufferObject(width, height, 0);
        fbo_hbao_calc_->attach_color_texture(GL_TEXTURE_2D, hbao_result, GL_COLOR_ATTACHMENT0);
		fbo_hbao_calc_->attach_color_texture(GL_TEXTURE_2D, hbao_blur, GL_COLOR_ATTACHMENT1);

        easy3d_debug_gl_error;
        easy3d_debug_frame_buffer_error;

		//////////////////////////////////////////////////////////////////////////

		int low_reso_width = ((width + LOW_RES_RATIO - 1) / LOW_RES_RATIO);
		int low_reso_height = ((height + LOW_RES_RATIO - 1) / LOW_RES_RATIO);

		//interleaved
		if (glIsTexture(textures.hbao2_deptharray))
			glDeleteTextures(1, &textures.hbao2_deptharray);
		glGenTextures(1, &textures.hbao2_deptharray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textures.hbao2_deptharray);  easy3d_debug_gl_error;

        if (gl_version >= 4.2f) // "glTexStorage3D" requires 4.2 or above
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, low_reso_width, low_reso_height, HBAO_RANDOM_ELEMENTS);
        else
        {// see "glTexStorage3D" at https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexStorage3D.xhtml
            // Liangliang: this may not work because (later to make a view of the texture) glTextureView requires
            //             the texture has an immutable format. Textures become immutable if their storage is
            //             specified with glTexStorage2D. If this is true. This piece of code will never run on macOS.
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, low_reso_width, low_reso_height, HBAO_RANDOM_ELEMENTS, 0, GL_RED, GL_FLOAT, nullptr);
        }

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);  easy3d_debug_gl_error;

		for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++) {
			if (glIsTexture(textures.hbao2_depthview[i]))
				glDeleteTextures(1, &textures.hbao2_depthview[i]);
			glGenTextures(1, &textures.hbao2_depthview[i]);
            glTextureView(textures.hbao2_depthview[i], GL_TEXTURE_2D, textures.hbao2_deptharray, GL_R32F, 0, 1, i, 1);
			glBindTexture(GL_TEXTURE_2D, textures.hbao2_depthview[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glBindTexture(GL_TEXTURE_2D, 0);  easy3d_debug_gl_error;
		}

		if (fbos.hbao2_deinterleave)
			glDeleteFramebuffers(1, &fbos.hbao2_deinterleave);
		glGenFramebuffers(1, &fbos.hbao2_deinterleave);
		glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_deinterleave);
		GLenum drawbuffers[NUM_MRT];
		for (int layer = 0; layer < NUM_MRT; layer++)
			drawbuffers[layer] = GL_COLOR_ATTACHMENT0 + layer;
		glDrawBuffers(NUM_MRT, drawbuffers);
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);

        easy3d_debug_gl_error;
        easy3d_debug_frame_buffer_error;

		//////////////////////////////////////////////////////////////////////////

		if (glIsTexture(textures.hbao2_resultarray))
			glDeleteTextures(1, &textures.hbao2_resultarray);
		glGenTextures(1, &textures.hbao2_resultarray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textures.hbao2_resultarray);  easy3d_debug_gl_error;

        if (gl_version >= 4.2f) // "glTexStorage3D" requires 4.2 or above
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, formatAO, low_reso_width, low_reso_height, HBAO_RANDOM_ELEMENTS);
        else
        {// see "glTexStorage3D" at https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexStorage3D.xhtml
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, formatAO, low_reso_width, low_reso_height, HBAO_RANDOM_ELEMENTS, 0, GL_RG, GL_FLOAT, nullptr);
        }

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glGenFramebuffers(1, &fbos.hbao2_calc);
		glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_calc);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textures.hbao2_resultarray, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);

        easy3d_debug_gl_error;
        easy3d_debug_frame_buffer_error;
	}


	void AmbientOcclusion_HBAO::init_misc()
	{
		ssao_kernel_.clear();

		// generates random floats between 0.0 and 1.0
		std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
		std::default_random_engine generator;

		// generate sample kernel
		for (unsigned int i = 0; i < KERNEL_SIZE; ++i) {
			vec3 sample(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator));
			sample = normalize(sample);
			sample *= randomFloats(generator);
			float scale = float(i) / KERNEL_SIZE;

			// scale samples s.t. they're more aligned to center of kernel
			scale = lerp(0.1f, 1.0f, scale * scale);
			sample *= scale;
			ssao_kernel_.push_back(sample);
		}

		// generate noise texture
		std::vector<vec3> ssaoNoise;
		for (int i = 0; i < NOISE_RES * NOISE_RES; i++) {
			//vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);//ssao
			vec3 noise(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator)); // rotate around z-axis (in tangent space)
			ssaoNoise.push_back(noise);
		}
		glGenTextures(1, &noise_texture_);
		glBindTexture(GL_TEXTURE_2D, noise_texture_);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, NOISE_RES, NOISE_RES, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glBindTexture(GL_TEXTURE_2D, 0);

		//////////////////////////////////////////////////////////////////////////

        const float num_dir = 8.0f; // keep in sync to glsl
		for (int i = 0; i < HBAO_RANDOM_ELEMENTS * MAX_SAMPLES; i++)
		{
            float r1 = randomFloats(generator);
            float r2 = randomFloats(generator);
            float r3 = randomFloats(generator);

            // Use random rotation angles in [0, 2PI/NUM_DIRECTIONS).
            float angle = static_cast<float>(2.0 * M_PI * r1 / num_dir);
            hbaoRandom[i].x = std::cosf(angle);
            hbaoRandom[i].y = std::sinf(angle);
            hbaoRandom[i].z = r2;
            hbaoRandom[i].w = r3;
#define SCALE ((1<<15))
            hbaoRandomShort[i * 4 + 0] = static_cast<signed short>(SCALE*hbaoRandom[i].x);
            hbaoRandomShort[i * 4 + 1] = static_cast<signed short>(SCALE*hbaoRandom[i].y);
            hbaoRandomShort[i * 4 + 2] = static_cast<signed short>(SCALE*hbaoRandom[i].z);
            hbaoRandomShort[i * 4 + 3] = static_cast<signed short>(SCALE*hbaoRandom[i].w);
#undef SCALE

		}

		//noise Tex

		glGenTextures(1, &textures.hbao_random);
		glBindTexture(GL_TEXTURE_2D_ARRAY, textures.hbao_random);

        //
        if (OpenglInfo::gl_version_number() >= 4.2f) // "glTexStorage3D" requires 4.2 or above
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA16_SNORM, HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE, MAX_SAMPLES);
        else
        {// see "glTexStorage3D" at https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexStorage3D.xhtml
            // Liangliang: this may not work because (later to make a view of the texture) glTextureView requires
            //             the texture has an immutable format. Textures become immutable if their storage is
            //             specified with glTexStorage2D. If this is true. This piece of code will never run on macOS.
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA16_SNORM, HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE, MAX_SAMPLES, 0, GL_RGBA, GL_SHORT, nullptr);
        }

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, HBAO_RANDOM_SIZE, HBAO_RANDOM_SIZE, MAX_SAMPLES, GL_RGBA, GL_SHORT, hbaoRandomShort);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		glGenTextures(MAX_SAMPLES, textures.hbao_randomview);
		for (int i = 0; i < MAX_SAMPLES; i++) {
            glTextureView(textures.hbao_randomview[i], GL_TEXTURE_2D, textures.hbao_random, GL_RGBA16_SNORM, 0, 1, i, 1);
			glBindTexture(GL_TEXTURE_2D, textures.hbao_randomview[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}


	void AmbientOcclusion_HBAO::prepare_hbao_data(int width, int height) {
		const mat4 PROJ = camera_->projectionMatrix();

		int low_reso_width = ((width + LOW_RES_RATIO - 1) / LOW_RES_RATIO);
		int low_reso_height = ((height + LOW_RES_RATIO - 1) / LOW_RES_RATIO);

		//hbao uniforms
		hbaoData.InvFullResolution = vec2(1.0f / float(width), 1.0f / float(height));
		hbaoData.InvQuarterResolution = vec2(1.0f / float(low_reso_width), 1.0f / float(low_reso_height));

		float projScale;
		if (camera_->type() == Camera::ORTHOGRAPHIC) {
            hbaoData.projOrtho = 1;
			hbaoData.projInfo = vec4(
				2.0f / PROJ(0, 0),					// ((x) * R - L)
				2.0f / PROJ(1, 1),					// ((y) * T - B)
				-(1.0f + PROJ(3, 0)) / PROJ(0, 0),	// L
				-(1.0f - PROJ(3, 1)) / PROJ(1, 1)	// B
			);
			projScale = float(height_) / hbaoData.projInfo[1];
		}
		else {
            hbaoData.projOrtho = 0;
			hbaoData.projInfo = vec4(
				2.0f / PROJ(0, 0),					// (x) * (R - L)/N
				2.0f / PROJ(1, 1),					// (y) * (T - B)/N
				-(1.0f - PROJ(2, 0)) / PROJ(0, 0),	// L/N
				-(1.0f + PROJ(2, 1)) / PROJ(1, 1)	// B/N
			);
			projScale = float(height_) / (std::tanf(camera_->fieldOfView() * 0.5f) * 2.0f);

		}

		float meters2viewspace = 1.0f;
		float R = radius_ * meters2viewspace;
		hbaoData.NegInvR2 = -1.0f / (R * R);
		hbaoData.RadiusToScreen = R * 0.5f * projScale;

		hbaoData.PowExponent = std::max(intensity_, 0.0f);
		hbaoData.NDotVBias = std::min(std::max(0.0f, bias_), 1.0f);
		hbaoData.AOMultiplier = 1.0f / (1.0f - hbaoData.NDotVBias);

		for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i++) {
			hbaoData.float2Offsets[i] = vec4(float(i % 4) + 0.5f, float(i / 4) + 0.5f, 0, 0);
			hbaoData.jitters[i] = hbaoRandom[i];
		}
	}


    void AmbientOcclusion_HBAO::linear_depth_pass(int sampleIdx) {
		fbo_depthlinear_->bind();

		// z_n * z_f,  z_n - z_f,  z_f, perspective = 1 : 0
		vec4 clipinfo(
			camera_->zNear() * camera_->zFar(),
			camera_->zNear() - camera_->zFar(),
			camera_->zFar(),
			camera_->type() == Camera::ORTHOGRAPHIC ? 0.0f : 1.0f
		);

		if (samples_ > 1) {
			shaderLinearDepth_msaa->bind();
			shaderLinearDepth_msaa->set_uniform("sampleIndex", sampleIdx);
			shaderLinearDepth_msaa->set_uniform("clipInfo", clipinfo);
			shaderLinearDepth_msaa->bind_texture("inputTexture", fbo_scene_->depth_texture(false), 0, GL_TEXTURE_2D_MULTISAMPLE);
			opengl::draw_full_screen_quad(ShaderProgram::POSITION, ShaderProgram::TEXCOORD, 0.0f); easy3d_debug_gl_error;
			shaderLinearDepth_msaa->release_texture(GL_TEXTURE_2D_MULTISAMPLE);
			shaderLinearDepth_msaa->release();
		}
		else {
			shaderLinearDepth->bind();
			shaderLinearDepth->set_uniform("clipInfo", clipinfo);
			shaderLinearDepth->bind_texture("inputTexture", fbo_scene_->depth_texture(true), 0);
			opengl::draw_full_screen_quad(ShaderProgram::POSITION, ShaderProgram::TEXCOORD, 0.0f); easy3d_debug_gl_error;
			shaderLinearDepth->release_texture();
			shaderLinearDepth->release();
		}
		fbo_depthlinear_->release();
		//fbo_depthlinear_->snapshot_color_ppm(0, "C:/tmp/fbo_depthlinear_-color.ppm");
	}


	void AmbientOcclusion_HBAO::geometry_pass(const std::vector<Model*>& models) {
		fbo_scene_->bind();

		vec4  bgColor(1.0, 1.0, 1.0, 1.0);
        glClearBufferfv(GL_COLOR, 0, &bgColor.x);

		glClearDepth(1.0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		// camera position is defined in world coordinate system.
		const vec3& wCamPos = camera_->position();
		// it can also be computed as follows:
		//const vec3& wCamPos = invMV * vec4(0, 0, 0, 1);
		const mat4& MV = camera_->modelViewMatrix();
		const vec4& wLightPos = inverse(MV) * setting::light_position;

		shaderDrawScene->bind();
		shaderDrawScene->set_uniform("MVP", camera_->projectionMatrix() * MV);
		shaderDrawScene->set_uniform("wLightPos", wLightPos);
		shaderDrawScene->set_uniform("wCamPos", wCamPos);

		for (auto model : models) {
			if (model->is_visible()) {
				for (auto d : model->points_drawables()) {
					if (d->is_visible()) {
						shaderDrawScene->set_uniform("default_color", d->default_color());
						shaderDrawScene->set_uniform("per_vertex_color", d->per_vertex_color() && d->color_buffer());
						d->draw(false); easy3d_debug_gl_error
					}
				}
				for (auto d : model->triangles_drawables()) {
					if (d->is_visible()) {
						shaderDrawScene->set_uniform("default_color", d->default_color());
						shaderDrawScene->set_uniform("per_vertex_color", d->per_vertex_color() && d->color_buffer());
						d->draw(false); easy3d_debug_gl_error
					}
				}
				for (auto d : model->lines_drawables()) {
					if (d->is_visible()) {
						shaderDrawScene->set_uniform("default_color", d->default_color());
						shaderDrawScene->set_uniform("per_vertex_color", d->per_vertex_color() && d->color_buffer());
						d->draw(false); easy3d_debug_gl_error
					}
				}
			}
		}

		shaderDrawScene->release();
		fbo_scene_->release();

		//fbo_scene_->snapshot_color_ppm(0, "C:/tmp/fbo_scene_-color.ppm");
		//fbo_scene_->snapshot_depth_ppm("C:/tmp/fbo_scene_-depth.ppm");
	}


	void AmbientOcclusion_HBAO::draw(const std::vector<Model*>& models, int samples /* = 0*/)
	{
		int viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);
		int w = viewport[2];
		int h = viewport[3];

		static bool initialized = false;
        if (!initialized || w != width_ || h != height_ || samples != samples_) {
            width_ = w;
            height_ = h;
            samples_ = samples;
            init_framebuffer(width_, height_, samples_);
        }

		if (!initialized) {
			init_programs();
			init_misc();
			initialized = true;
		}

		glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&defaultFBO);	easy3d_debug_gl_error;

        //-----------------------------------------------------------------------------
        // IMPORTANT : so that depth information is propagated
        // Liangliang: I don't know if this can resolve the boundary issue in tiled rendering
        //glDisable(GL_SCISSOR_TEST);
        //-----------------------------------------------------------------------------

		geometry_pass(models);

		prepare_hbao_data(width_, height_);

        // make sure it will be executed once even samples_ is 0.
        int num_samples = samples_ > 0 ? samples_ : 1;

		for (int sample = 0; sample < num_samples; sample++) {
            linear_depth_pass(sample);

			switch (algorithm_)
			{
			case SSAO_NONE:
				break;
			case SSAO_CLASSIC:
                ssao(sample);
				break;
			case SSAO_HBO:
                hbao(sample);
				break;
			case SSAO_HBO_PLUS:
                hbao_plus(sample);
				break;
			}

            blur_hbao(sample); easy3d_debug_gl_error;
		}

		{
            // blit to the default fbo
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_scene_->handle());
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFBO); easy3d_debug_gl_error;
            glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            easy3d_debug_gl_error;
            easy3d_debug_frame_buffer_error;
            glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO); easy3d_debug_gl_error;
        }

        //-----------------------------------------------------------------------------
        // IMPORTANT : restore default state
        // Liangliang: I don't know if this can resolve the boundary issue in tiled rendering
        //glEnable(GL_SCISSOR_TEST);
        //-----------------------------------------------------------------------------
	}


    void AmbientOcclusion_HBAO::ssao(int sampleIdx)
	{
		fbo_hbao_calc_->bind();
		fbo_hbao_calc_->activate_draw_buffer(0);

        program_ssao_->bind();
        program_ssao_->set_uniform("samples[0]", ssao_kernel_[0]);
        program_ssao_->set_uniform("projection", camera_->projectionMatrix());
        program_ssao_->set_uniform("projInfo", hbaoData.projInfo);
        program_ssao_->set_uniform("projOrtho", hbaoData.projOrtho);
        program_ssao_->set_uniform("viewerSize", vec2(width_, height_));
        program_ssao_->set_uniform("InvFullResolution", hbaoData.InvFullResolution);
        program_ssao_->bind_texture("texLinearDepth", fbo_depthlinear_->color_texture(0), 0);
        program_ssao_->bind_texture("texRandom", noise_texture_, 1);
        program_ssao_->set_uniform("radius", radius_);
        program_ssao_->set_uniform("bias", bias_);
		opengl::draw_full_screen_quad(ShaderProgram::POSITION, ShaderProgram::TEXCOORD, 0.0f); easy3d_debug_gl_error;
        program_ssao_->release_texture();
        program_ssao_->release();

		fbo_hbao_calc_->release();
	}


    void AmbientOcclusion_HBAO::hbao(int sampleIdx)
	{
		fbo_hbao_calc_->bind();
		fbo_hbao_calc_->activate_draw_buffer(0);

		hbao_calc_blur->bind();
		hbao_calc_blur->set_uniform("projInfo", hbaoData.projInfo);
		hbao_calc_blur->set_uniform("projOrtho", hbaoData.projOrtho);
		hbao_calc_blur->set_uniform("RadiusToScreen", hbaoData.RadiusToScreen);
		hbao_calc_blur->set_uniform("InvFullResolution", hbaoData.InvFullResolution);
		hbao_calc_blur->set_uniform("NegInvR2", hbaoData.NegInvR2);
		hbao_calc_blur->set_uniform("NDotVBias", hbaoData.NDotVBias);
		hbao_calc_blur->set_uniform("AOMultiplier", hbaoData.AOMultiplier);
		hbao_calc_blur->set_uniform("PowExponent", hbaoData.PowExponent);

		hbao_calc_blur->bind_texture("texLinearDepth", fbo_depthlinear_->color_texture(0), 0);
		hbao_calc_blur->bind_texture("texRandom", textures.hbao_randomview[sampleIdx], 1);

		opengl::draw_full_screen_quad(ShaderProgram::POSITION, ShaderProgram::TEXCOORD, 0.0f); easy3d_debug_gl_error;
		hbao_calc_blur->release_texture();
		hbao_calc_blur->release();

		fbo_hbao_calc_->release();
		//fbo_hbao_calc_->snapshot_color_ppm(0, "C:/tmp/fbo_hbao_calc_-color-0.ppm");

	}


    void AmbientOcclusion_HBAO::hbao_plus(int sampleIdx)
	{
		{
			//viewnormal
			fbo_viewnormal_->bind();
            program_view_normal_->bind();
            program_view_normal_->set_uniform("projInfo", hbaoData.projInfo);
            program_view_normal_->set_uniform("projOrtho", hbaoData.projOrtho);
            program_view_normal_->set_uniform("InvFullResolution", hbaoData.InvFullResolution);
            program_view_normal_->bind_texture("texLinearDepth", fbo_depthlinear_->color_texture(0), 0);
			opengl::draw_full_screen_quad(ShaderProgram::POSITION, ShaderProgram::TEXCOORD, 0.0f); easy3d_debug_gl_error;
            program_view_normal_->release_texture();
            program_view_normal_->release();
			fbo_viewnormal_->release();

			//fbo_viewnormal_->snapshot_color_bmp(0, "c:/tmp/fbo_viewnormal_-color0.bmp");
		}

        int low_reso_width = ((width_ + LOW_RES_RATIO - 1) / LOW_RES_RATIO);
        int low_reso_height = ((height_ + LOW_RES_RATIO - 1) / LOW_RES_RATIO);
		glViewport(0, 0, low_reso_width, low_reso_height);

		{
			//deinter
			glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_deinterleave);

			//GLenum drawbuffers[NUM_MRT];
			//for (int layer = 0; layer < NUM_MRT; layer++)
			//	drawbuffers[layer] = GL_COLOR_ATTACHMENT0 + layer;
			//glDrawBuffers(NUM_MRT, drawbuffers);

			hbao2_deinterleave->bind();
			hbao2_deinterleave->bind_texture("texLinearDepth", fbo_depthlinear_->color_texture(0), 0);
			for (int i = 0; i < HBAO_RANDOM_ELEMENTS; i += NUM_MRT) {
				const vec4 info(float(i % 4) + 0.5f, float(i / 4) + 0.5f, hbaoData.InvFullResolution[0], hbaoData.InvFullResolution[1]);
				hbao2_deinterleave->set_uniform("info", info);
				for (int layer = 0; layer < NUM_MRT; layer++)
					glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + layer, textures.hbao2_depthview[i + layer], 0);
				opengl::draw_full_screen_quad(ShaderProgram::POSITION, ShaderProgram::TEXCOORD, 0.0f); easy3d_debug_gl_error;
			}
			hbao2_deinterleave->release_texture();
			hbao2_deinterleave->release();

			//	opengl::snapshot_color_ppm("c:/tmp/fbos.hbao2_deinterleave-color0.ppm");

			glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
		}
		{
			//hbaoplus
			glBindFramebuffer(GL_FRAMEBUFFER, fbos.hbao2_calc);

			hbao2_calc_blur->bind();
			hbao2_calc_blur->bind_texture("texViewNormal", fbo_viewnormal_->color_texture(0), 1);

			// instead of drawing to each layer individually
            // we draw all layers at once, and use image writes to update the array texture
            // this buys additional performance :)
			hbao2_calc_blur->bind_texture("texLinearDepth", textures.hbao2_deptharray, 0, GL_TEXTURE_2D_ARRAY);

            hbao2_calc_blur->set_uniform("projInfo", hbaoData.projInfo);
            hbao2_calc_blur->set_uniform("projOrtho", hbaoData.projOrtho);
            hbao2_calc_blur->set_uniform("InvQuarterResolution", hbaoData.InvQuarterResolution);
            hbao2_calc_blur->set_uniform("NegInvR2", hbaoData.NegInvR2);
            hbao2_calc_blur->set_uniform("NDotVBias", hbaoData.NDotVBias);
            hbao2_calc_blur->set_uniform("AOMultiplier", hbaoData.AOMultiplier);
            hbao2_calc_blur->set_uniform("RadiusToScreen", hbaoData.RadiusToScreen);
            hbao2_calc_blur->set_uniform("PowExponent", hbaoData.PowExponent);
            hbao2_calc_blur->set_uniform("float2Offsets[0]", hbaoData.float2Offsets[0]);
            hbao2_calc_blur->set_uniform("jitters[0]", hbaoData.jitters[0]);

			static GLuint defaultVAO = 0;
			if (defaultVAO == 0)
				glGenVertexArrays(1, &defaultVAO);
			glBindVertexArray(defaultVAO);
			glDrawArrays(GL_TRIANGLES, 0, 3 * HBAO_RANDOM_ELEMENTS); easy3d_debug_gl_error;
			glBindVertexArray(0);

			hbao2_calc_blur->release_texture(GL_TEXTURE_2D);
			hbao2_calc_blur->release_texture(GL_TEXTURE_2D_ARRAY);
			hbao2_calc_blur->release();

			//	opengl::snapshot_color_ppm("c:/tmp/fbos.hbao2_calc-color0.ppm");

			glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
		}

		glViewport(0, 0, width_, height_);
		{
			//reinter
			fbo_hbao_calc_->bind();
			fbo_hbao_calc_->activate_draw_buffer(0);

			hbao2_reinterleave_blur->bind();
			hbao2_reinterleave_blur->bind_texture("texResultsArray", textures.hbao2_resultarray, 0, GL_TEXTURE_2D_ARRAY);
			opengl::draw_full_screen_quad(ShaderProgram::POSITION, ShaderProgram::TEXCOORD, 0.0f); easy3d_debug_gl_error;
			hbao2_reinterleave_blur->release_texture(GL_TEXTURE_2D_ARRAY);
			hbao2_reinterleave_blur->release();
			fbo_hbao_calc_->release();

            //fbo_hbao_calc_->snapshot_color_bmp(0, "c:/tmp/fbo_hbao_calc_-color0.bmp");
		}
	}


	void AmbientOcclusion_HBAO::blur_hbao(int sampleIdx) {
		fbo_hbao_calc_->bind();
		fbo_hbao_calc_->activate_draw_buffer(1);

		float meters2viewspace = 1.0f;
		hbao_blur->bind();
		hbao_blur->set_uniform("g_Sharpness", sharpness_ / meters2viewspace);
		hbao_blur->set_uniform("g_InvResolutionDirection", vec2(1.0f / float(width_), 0));
		hbao_blur->bind_texture("texSource", fbo_hbao_calc_->color_texture(0), 0);
		//	fbo_hbao_calc_->snapshot_color_bmp(0, "c:/tmp/fbo_hbao_calc_-color0.bmp");

		opengl::draw_full_screen_quad(ShaderProgram::POSITION, ShaderProgram::TEXCOORD, 0.0f); easy3d_debug_gl_error;

		hbao_blur->release_texture();
		hbao_blur->release();

		//fbo_hbao_calc_->snapshot_color_bmp(0, "c:/tmp/fbo_hbao_calc_-color0.bmp");
		//fbo_hbao_calc_->snapshot_color_bmp(1, "c:/tmp/fbo_hbao_calc_-color1.bmp");
		fbo_hbao_calc_->release();

		// final output to main fbo
		fbo_scene_->bind();
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		if (samples_ > 1) {
			glEnable(GL_SAMPLE_MASK);
			glSampleMaski(0, 1 << sampleIdx);
		}

		hbao_blur2->bind();
		hbao_blur2->set_uniform("g_Sharpness", sharpness_ / meters2viewspace);
		hbao_blur2->bind_texture("texSource", fbo_hbao_calc_->color_texture(1), 0);
		hbao_blur2->set_uniform("g_InvResolutionDirection", vec2(0.0f, 1.0f / float(height_)));
		opengl::draw_full_screen_quad(ShaderProgram::POSITION, ShaderProgram::TEXCOORD, 0.0f); easy3d_debug_gl_error;
		hbao_blur2->release_texture();
		hbao_blur2->release();

		//fbo_scene_->snapshot_color_bmp(0, "c:/tmp/fbo_scene_-color0.bmp");
		fbo_scene_->release();

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_SAMPLE_MASK);
		glSampleMaski(0, ~0);
	}


	unsigned int AmbientOcclusion_HBAO::ssao_texture() const {
		return fbo_scene_->color_texture(0);
	}


	void AmbientOcclusion_HBAO::draw_occlusion(int x, int y, int w, int h) {
		static const std::string name = "screen_space/textured_quad";
		ShaderProgram* program = ShaderManager::get_program(name);
		if (!program) {
			std::vector<ShaderProgram::Attribute> attributes;
			attributes.push_back(ShaderProgram::Attribute(ShaderProgram::POSITION, "vertexMC"));
			attributes.push_back(ShaderProgram::Attribute(ShaderProgram::TEXCOORD, "tcoordMC"));
			program = ShaderManager::create_program_from_files(name, attributes);
		}
		if (!program)
			return;

		program->bind();		easy3d_debug_gl_error;

		program->bind_texture("textureID", ssao_texture(), 0);
		opengl::draw_quad(ShaderProgram::POSITION, ShaderProgram::TEXCOORD, x, y, w, h, width_, height_, -1.0f); easy3d_debug_gl_error;
		program->release_texture();
		program->release();		easy3d_debug_gl_error;
	}


} // namespace easy3d
