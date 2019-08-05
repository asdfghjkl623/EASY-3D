
#include "manipulatable.h"
#include "frame.h"
#include "transform.h"
#include "constraint.h"
#include "model.h"
#include "primitives.h"


Manipulatable::Manipulatable(Model* model)
	: model_(model) 
	, manipulated_frame_(0)
{
}


Manipulatable::~Manipulatable() {
	if (manipulated_frame_)
		delete manipulated_frame_;
}


void Manipulatable::reset_manipulated_frame() {
	if (model_) {
		const vec3& center = model_->bounding_box().center();
		manipulated_frame()->setPositionAndOrientation(center, quat());
	}	
}


Frame* Manipulatable::manipulated_frame() {
	if (manipulated_frame_ == 0)
		manipulated_frame_ = new Frame;
	return manipulated_frame_;
}


const Frame* Manipulatable::manipulated_frame() const {
	return const_cast<Manipulatable*>(this)->manipulated_frame();
}


mat4 Manipulatable::manipulated_matrix() const {
	if (model_) {
		const mat4& mat = manipulated_frame()->matrix();

		// ------- the real transformation w.r.t. the center of the object -------
		// NOTE: The total transformation of the *frame* contains three parts:
		//  (1) an extra translation that moves the frame to the center of the object (for display).
		//  (2) a pure rotation
		//  (3) a pure translation (this is the real translation should be applied to the object).  
		// So we have to compensate the extra translation.
		const vec3& center = model_->bounding_box(false).center();
		return mat * mat4::translation(-center);
	}
	else
		return mat4::identity();
}


void Manipulatable::draw_manipulated_frame() const {
	if (model_) {
		glPushMatrix();
		glMultMatrixf(model_->manipulated_frame()->matrix());
		float radius = model_->bounding_box(false).diagonal() * 0.5f;
		OpenGL::draw_axis(radius); // the size of radius is large enough to be always visible.
		glPopMatrix();
	}
}


//////////////////////////////////////////////////////////////////////////


void ManipulationAgent::clear()
{
	frames_.clear();
}

void ManipulationAgent::add_frame(Frame* frame)
{
	frames_.insert(frame);
}

void ManipulationAgent::remove_frame(Frame* frame) {
	frames_.erase(frame);
}

void ManipulationAgent::constrainTranslation(vec3 &translation, Frame *const)
{
	std::set<Frame*>::iterator it = frames_.begin();
	for (; it != frames_.end(); ++it) {
		(*it)->translate(translation);
	}
}

void ManipulationAgent::constrainRotation(quat &rotation, Frame *const frame)
{
	// Prevents failure introduced by numerical error. In Quat::axis() and Quat::angle(),
	// we call std::acos(q_[3]). However, q_[3] equaling to 1 can actually be 1.000000119.
	rotation.normalize();

	// A little bit of math. Easy to understand, hard to guess (tm).
	// rotation is expressed in the frame local coordinates system. Convert it back to world coordinates.
	const vec3& worldAxis = frame->inverseTransformOf(rotation.axis());
	const vec3& pos = frame->position();
	const float angle = rotation.angle();

	std::set<Frame*>::iterator it = frames_.begin();
	for (; it != frames_.end(); ++it) {
		// Rotation has to be expressed in the object local coordinates system.
		quat qObject((*it)->transformOf(worldAxis), angle);
		(*it)->rotate(qObject);

		// The following two lines enable all selected objects rotating around the frame world position 
		// 'pos' (rotated as a unity around the same point). Comment these lines will enable each object
		// rotating around its bbox center.
		//quat qWorld(worldAxis, angle);
		//(*it)->setPosition(pos + qWorld.rotate((*it)->position() - pos));
	}
}