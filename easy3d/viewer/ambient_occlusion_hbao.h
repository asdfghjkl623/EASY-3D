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


#ifndef EASY_AMBIENT_OCCLUSION_HBAO_H
#define EASY_AMBIENT_OCCLUSION_HBAO_H


#include <string>
#include <vector>

#include <easy3d/core/types.h>

namespace easy3d {

	class Model;
	class Camera;
	class ShaderProgram;
	class FramebufferObject;

    /* This class implements screen space ambient occlusion (SSAO) using horizon-based ambient occlusion (HBAO).
     * You can find some details about HBAO in "Bavoil et al. 2008. Image-Space Horizon-Based Ambient Occlusion"
     * It provides two alternative implementations the original hbao as well as an enhanced version that is more
     * efficient in improved leveraging of the hardware's texture sampling cache, using de-interleaved texturing.
     *
     *  - HBAO - Classic:
     *      To achieve the effect a 4x4 texture that contains random directions is tiled across the screen and
     *      used to sample the neighborhood of a pixel's depth values.The distance of the sampling depends on a
     *      customize-able world-size radius, for which the depth-buffer values are typically linearized first.
     *      As the sampling radius depends on the pixel's depth, a big variability in the texture lookups can
     *      exist from one pixel to another. To reduce the costs the effect can be computed at lower-resolution
     *      and up-scaled for final display. As AO is typically a low-frequency effect this often can be sufficient.
     *      Dithering artifacts can occur due to the 4x4 texture tiling. The image is blurred using cross-bilateral
     *      filtering that takes the depth values into account, to further improve quality.
     *
     *  - HBAO - Cache-Aware:
     *      The performance is vastly improved by grouping all pixels that share the same direction values. This
     *      means the screen-space linear depth buffer is stored in 16 layers each representing one direction of
     *      the 4x4 texture. Each layer has a quarter of the original resolution. The total amount of pixels is
     *      not reduced, but the sampling is performed in equal directions for the entire layer, yielding better
     *      texture cache utilization. Linearizing the depth-buffer now stores into 16 texture layers.
     *      The actual HBAO effect is performed in each layer individually, however all layers are independent of
     *      each other, allowing them to be processed in parallel. Finally the results are stored scattered to
     *      their original locations in screen-space. Compared to the regular HBAO approach, the efficiency gains
     *      allow using the effect on full-resolution, improving the image quality.
     *
     * MSAA support:
     *  The effect is run on a per-sample level N times (N matching the MSAA level).
     *  For each pass glSampleMask( 1 << sample); is used to update only the relevant samples in the target framebuffer.
     *
     * Blur:
     *  A cross-bilteral blur is used to eliminate the typical dithering artifacts. It makes use of the depth buffer
     *  to avoid smoothing over geometric discontinuities.
     *
     * The core rendering code is take from the NVidia sample here: https://github.com/nvpro-samples/gl_ssao
     */


    class AmbientOcclusion_HBAO
    {
    public:
        enum Algorithm {
            SSAO_NONE = 0,      // SSAO disabled
            SSAO_CLASSIC = 1,
            SSAO_HBO = 2,
            SSAO_HBO_PLUS = 3
        };

    public:
        AmbientOcclusion_HBAO(Camera* cam);
        virtual ~AmbientOcclusion_HBAO();

        void set_algorithm(Algorithm algo) { algorithm_ = algo; }
        Algorithm algorithm() const { return algorithm_; }

        // used to compute sample radius (in pixels [0, 4]?).
        float radius() const { return radius_; }
        void  set_radius(float r) { radius_ = r; }

        float intensity() const { return intensity_; }
        void  set_intensity(float i) { intensity_ = i; }

        float bias() const { return bias_; }
        void  set_bias(float b) { bias_ = b; }

        float sharpness() const { return sharpness_; }
        void  set_sharpness(float s) { sharpness_ = s; }

        void  draw(const std::vector<Model*>& models, int samples = 0);

        // return the ssao texture
        unsigned int ssao_texture() const;

		// show the occlusion texture on the screen region
		void  draw_occlusion(int x, int y, int w, int h);

    protected:

		void init_programs();
		void init_framebuffer(int w, int h, int samples);
		void init_misc();

		void prepare_hbao_data(int width, int height);

		void geometry_pass(const std::vector<Model*>& models);
        void linear_depth_pass(int sampleIdx);

        void ssao(int sampleIdx);
        void hbao(int sampleIdx);
        void hbao_plus(int sampleIdx);

		void blur_hbao(int sampleIdx);

    private:
        Camera* camera_;

        Algorithm algorithm_;

        int     samples_;
        int     width_;
        int     height_;

        float	radius_;
        float	intensity_;
        float	bias_;
        float	sharpness_;

        ShaderProgram*  shaderDrawScene = nullptr;
		ShaderProgram*  shaderLinearDepth = nullptr;
		ShaderProgram*  shaderLinearDepth_msaa = nullptr;
		ShaderProgram*  program_view_normal_ = nullptr;
        ShaderProgram*  hbao_calc_blur = nullptr;

		ShaderProgram*  hbao_blur = nullptr;
		ShaderProgram*  hbao_blur2 = nullptr;

		ShaderProgram*  program_ssao_ = nullptr;
        ShaderProgram*  hbao2_calc_blur = nullptr;
        ShaderProgram*  hbao2_deinterleave = nullptr;
        ShaderProgram*  hbao2_reinterleave_blur = nullptr;

		// TODO: less fbo but more color attachments.
		FramebufferObject* fbo_scene_;
		FramebufferObject* fbo_depthlinear_;
		FramebufferObject* fbo_viewnormal_;
		FramebufferObject* fbo_hbao_calc_;
		FramebufferObject* fbo_hbao2_deinterleave_;
		FramebufferObject* fbo_hbao2_calc_;

        std::vector<vec3> ssao_kernel_;
        unsigned int	  noise_texture_;

    private:
        //copying disabled
		AmbientOcclusion_HBAO(const AmbientOcclusion_HBAO&);
        AmbientOcclusion_HBAO& operator=(const AmbientOcclusion_HBAO&);
    };

} // namespace easy3d


#endif	//  EASY_AMBIENT_OCCLUSION_HBAO_H
