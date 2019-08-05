#include "clipping_plane.h"
#include "primitives.h"
#include "frame.h"
#include "transform.h"
#include "opengl_error.h"
#include "shader_program.h"
#include "constraint.h"


ClippingPlane::ClippingPlane()
	: manipulated_frame_(0)
	, size_(1.0f)
	, enabled_(false)
	, cross_section_(false)
	, cross_section_width_(0.01f)
	, fit_scene_(false)
{
}


ClippingPlane::~ClippingPlane() {
	if (manipulated_frame_)
		delete manipulated_frame_;
}


Frame* ClippingPlane::manipulated_frame() {
	if (manipulated_frame_ == 0) {
		manipulated_frame_ = new Frame;

		static LocalConstraint constaint;
		//constaint.setTranslationConstraintType(AxisPlaneConstraint::AXIS);
		constaint.setTranslationConstraint(AxisPlaneConstraint::AXIS, vec3(0, 0, 1));
		manipulated_frame_->setConstraint(&constaint);
	}
	return manipulated_frame_;
}


const Frame* ClippingPlane::manipulated_frame() const {
	return const_cast<ClippingPlane*>(this)->manipulated_frame();
}


void ClippingPlane::fit_scene(const Box3& box) {
	scene_bbox_ = box;
	size_ = box.diagonal() * 0.5f;

	//manipulated_frame()->setPositionAndOrientation(center_, quat());
	manipulated_frame()->setPosition(box.center()); // keep the orientation

	fit_scene_ = true;
}


Plane3 ClippingPlane::plane0() const {
	return Plane3(center(), normal());
}


Plane3 ClippingPlane::plane1() const {
	const vec3& n = normal();
	return Plane3(center() + cross_section_width_ * size_ * n, -n);
}


vec3 ClippingPlane::center() const {
	return manipulated_frame()->position();
}


vec3 ClippingPlane::normal() const {
	const mat4& CS = manipulated_frame()->matrix();
	const vec3& n = OpenGL::normal_matrix(CS) * vec3(0, 0, 1);
	return n;
}


void ClippingPlane::set_program(ShaderProgram* program, bool uniform_block) {
	if (enabled_) {
		glEnable(GL_CLIP_DISTANCE0);
		if (cross_section_)
			glEnable(GL_CLIP_DISTANCE1);
		else
			glDisable(GL_CLIP_DISTANCE1);
	}
	else {
		glDisable(GL_CLIP_DISTANCE0);
		glDisable(GL_CLIP_DISTANCE1);
	}

	if (uniform_block) {
		program->set_block_uniform("Matrices", "clippingPlaneEnabled", &enabled_);	mpl_debug_gl_error;
		program->set_block_uniform("Matrices", "crossSectionEnabled", &cross_section_);	mpl_debug_gl_error;
		program->set_block_uniform("Matrices", "clippingPlane0", plane0());		mpl_debug_gl_error;
		program->set_block_uniform("Matrices", "clippingPlane1", plane1());		mpl_debug_gl_error;
	}
	else {
		program->set_uniform("clippingPlaneEnabled", &enabled_);	mpl_debug_gl_error;
		program->set_uniform("crossSectionEnabled", &cross_section_);	mpl_debug_gl_error;
		program->set_uniform("clippingPlane0", plane0());		mpl_debug_gl_error;
		program->set_uniform("clippingPlane1", plane1());		mpl_debug_gl_error;
	}
}


void ClippingPlane::draw(bool arrow /* = false*/) const {
	if (!enabled_)
	 return;
	
	glEnable(GL_LIGHTING);
	glPushMatrix();

	// Since the Clipping Plane equation is multiplied by the current modelView, we can define a
	// constant equation (plane normal along Z and passing by the origin) since we are here in the
	// coordinates system of the clipping plane frame (we glMultMatrixf() with the matrix of the 
	// clipping plane frame matrix()).
	// TODO: consider the manipulation transformation for the clipping plane
	glMultMatrixf(manipulated_frame()->matrix());
	if (arrow) {
		glColor4f(1.0f, 0.5f, 0.5f, 1.0f);
		OpenGL::draw_arrow(size_ * 0.4f, size_ * 0.015f);
	}
	
	glColor4f(1.0f, 0.5f, 0.5f, 0.2f);
	glNormal3fv(normal());
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  //GL_ONE_MINUS_DST_ALPHA

	const GLfloat v[8] = { -size_, -size_, size_, -size_, size_, size_, -size_, size_ };
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, v);
	glDrawArrays(GL_QUADS, 0, 4);
	glColor4f(0, 0, 0, 1.0f);
	glLineWidth(1.0f);
	glDrawArrays(GL_LINE_LOOP, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	glPopMatrix();
}
