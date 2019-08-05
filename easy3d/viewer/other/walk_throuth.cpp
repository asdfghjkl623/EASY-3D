#include "walk_throuth.h"
#include "opengl_error.h"
#include "camera.h"
#include "primitives.h"
#include "drawable.h"
#include "ground_plane.h"
#include "rendering_engine.h"
#include <basic/logger.h>


WalkThrough::WalkThrough()
	: active_(false)
	, buffer_up_to_date_(false)
	, show_path_(true)
	, character_drawable_(nil)
	, character_head_drawable_(nil)
	, view_direction_base_drawable_(nil)
	, view_direction_arraw_drawable_(nil)
	, height_factor_(0.6f)
	, distance__factor_(2.5f)
	, current_position_idx_(-1)
{
}


WalkThrough::~WalkThrough() {
	delete_path();
}


void WalkThrough::delete_path() {
	path_.clear();
	
	if (character_drawable_) {
		delete character_drawable_;
		character_drawable_ = 0;
	}
	if (character_head_drawable_) {
		delete character_head_drawable_;
		character_head_drawable_ = 0;
	}
	if (view_direction_base_drawable_) {
		delete view_direction_base_drawable_;
		view_direction_base_drawable_ = 0;
	}
	if (view_direction_arraw_drawable_) {
		delete view_direction_arraw_drawable_;
		view_direction_arraw_drawable_ = 0;
	}

	current_position_idx_ = -1;

	buffer_up_to_date_ = false;
}


void WalkThrough::set_scene_bbox(const Box3& box) {
	scene_box_ = box;
	buffer_up_to_date_ = false;
}


float WalkThrough::character_height() const {
	return scene_box_.z_range() * height_factor_;
}


float WalkThrough::character_distance() const {
	return character_height() * distance__factor_;
}


vec3 WalkThrough::character_head(const vec3& pos) const {
	const vec3& head = pos + RenderingEngine::ground_plane()->normal() * character_height();
	return head;
}


vec3 WalkThrough::camera_position(const vec3& pos, const vec3& view_dir) const {
	const vec3& cam_pos = character_head(pos) - view_dir * character_distance();
	return cam_pos;
}


// the character will be standing at pos looking in view_dir direction
void WalkThrough::add_position(const vec3& pos, const vec3& view_dir) {
	vec3 dir = view_dir;
	dir.z = 0; // force looking at horizontal direction

	path_.push_back(StandPoint(pos, normalize(dir)));
	buffer_up_to_date_ = false;
}


// the character will be standing at pos looking in view_dir direction
void WalkThrough::add_position(const vec3& pos) {
	const vec3& head = character_head(pos);
	const vec3& view_dir = scene_box_.center() - head;
	add_position(pos, view_dir);
}


void WalkThrough::set_height_factor(float f) { 
	height_factor_ = f;
	buffer_up_to_date_ = false;
	move_to(current_position_idx_);
}


void WalkThrough::set_distance_factor(float f) { 
	distance__factor_ = f; 
	buffer_up_to_date_ = false;
	move_to(current_position_idx_);
}


int WalkThrough::move_to(int idx, bool animation /* = true */, float duration /* = 0.5f */) {
	if (path_.empty() || idx < 0 || idx >= path_.size())
		return current_position_idx_;

	const vec3& pos = path_[idx].pos;

	if (animation) {
		const Frame& frame = get_frame(idx);
		RenderingEngine::camera()->interpolateTo(frame, duration);
	}
	else {
		const vec3& view_dir = path_[idx].view_dir;
		const vec3& cam_pos = camera_position(pos, view_dir);
		const vec3& up_dir = RenderingEngine::ground_plane()->normal();

		RenderingEngine::camera()->setPosition(cam_pos);
		RenderingEngine::camera()->setViewDirection(view_dir);
		RenderingEngine::camera()->setUpVector(up_dir);
	}

	RenderingEngine::camera()->setPivotPoint(pos);

	current_position_idx_ = idx;
	return current_position_idx_;
}


Frame WalkThrough::get_frame(int idx) const {
	if (path_.empty() || idx < 0 || idx >= path_.size()) {
		Logger::warn(title()) << "position index (" << idx << ") out of range" << std::endl;
		return Frame();
	}

	const vec3& pos = path_[idx].pos;
	const vec3& view_dir = path_[idx].view_dir;
	const vec3& cam_pos = camera_position(pos, view_dir);

	const vec3& up_dir = RenderingEngine::ground_plane()->normal();
	vec3 xAxis = cross(view_dir, up_dir);
	if (xAxis.length2() < 1E-10) {	// target is aligned with upVector, this means a rotation around X axis
		// X axis is then unchanged, let's keep it !
		// rightVector() == RenderingEngine::camera()->frame()->inverseTransformOf(vec3(1.0, 0.0, 0.0));
		xAxis = RenderingEngine::camera()->rightVector();
	}

	quat orient;
	orient.set_from_rotated_basis(xAxis, up_dir, -view_dir);

	return Frame(cam_pos, orient);
}


std::vector<Frame> WalkThrough::get_key_frames() const {
	std::vector<Frame> key_frames;
	for (std::size_t i = 0; i < path_.size(); ++i) {
		const Frame& frame = get_frame(static_cast<int>(i));
		key_frames.push_back(frame);
	}
	return key_frames;
}


void WalkThrough::draw() const {
	if (!active_)
		return;

	if (show_path_)
		draw_path();
}


void WalkThrough::draw_path() const {
	if (!character_drawable_)
		const_cast<WalkThrough*>(this)->character_drawable_ = new LinesDrawable;
	if (!character_head_drawable_)
		const_cast<WalkThrough*>(this)->character_head_drawable_ = new PointsDrawable;
	if (!view_direction_base_drawable_)
		const_cast<WalkThrough*>(this)->view_direction_base_drawable_ = new LinesDrawable;
	if (!view_direction_arraw_drawable_)
		const_cast<WalkThrough*>(this)->view_direction_arraw_drawable_ = new LinesDrawable;

	if (!buffer_up_to_date_) {
		float height = character_height();
		std::vector<vec3> character_points, head_points, dir_base_points, dir_arrow_points;
		for (std::size_t i = 0; i < path_.size(); ++i) {
			const vec3& feet = path_[i].pos;
			const vec3& head = character_head(feet);
			character_points.push_back(feet);
			character_points.push_back(head);

			head_points.push_back(head);

			const vec3& dir = path_[i].view_dir;
			const vec3& base_end = head + dir * height * 0.2f;
			const vec3& arrow_end = base_end + dir * height * 0.1f;
			dir_base_points.push_back(head);
			dir_base_points.push_back(base_end);
			dir_arrow_points.push_back(base_end);
			dir_arrow_points.push_back(arrow_end);
		}
		const_cast<WalkThrough*>(this)->character_drawable_->update_vertex_buffer(character_points);
		const_cast<WalkThrough*>(this)->character_head_drawable_->update_vertex_buffer(head_points);
		const_cast<WalkThrough*>(this)->view_direction_base_drawable_->update_vertex_buffer(dir_base_points);
		const_cast<WalkThrough*>(this)->view_direction_arraw_drawable_->update_vertex_buffer(dir_arrow_points);
		const_cast<WalkThrough*>(this)->buffer_up_to_date_ = true;
	}

	float character_width = 6.0f;
	// characters
	RenderingEngine::draw_line_imposters(character_drawable_, character_width, false, vec3(0.5f, 1.0f, 0.5f), true);
	// heads
	RenderingEngine::draw_spheres_geometry(character_head_drawable_, character_width * 2.0f, false, vec3(0.5f, 0.5f, 1.0f), false);
	// view directions
	RenderingEngine::draw_line_imposters(view_direction_base_drawable_, character_width, false, vec3(1.0f, 0.5f, 0.5f), true);
	RenderingEngine::draw_line_imposters(view_direction_arraw_drawable_, character_width * 2.0f, false, vec3(1.0f, 0.5f, 0.5f), false);
}