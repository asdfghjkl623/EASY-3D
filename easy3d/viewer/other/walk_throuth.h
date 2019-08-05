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


#ifndef _OPENGL_WALK_THROUGH_H_
#define _OPENGL_WALK_THROUGH_H_

#include <opengl/opengl_common.h>
#include "frame.h"
#include <math/math_types.h>
#include <basic/smart_pointer.h>
#include <basic/counted.h>

class Frame;
class LinesDrawable;
class PointsDrawable;

// NOTE: walk through requires that the object is in 
//       upright orientation, i.e., (0, 0, 1).
class OPENGL_API WalkThrough : public Counted
{
public: 
	typedef SmartPointer<WalkThrough>   Ptr;

public:
	WalkThrough();
	virtual ~WalkThrough();

	void set_active(bool b) { active_ = b; }
	bool is_active() const { return active_; }

	void set_scene_bbox(const Box3& box);

	// the character will be standing at pos looking in view_dir direction
	void add_position(const vec3& pos, const vec3& view_dir);
	void add_position(const vec3& pos); // looking at the scene center
	
	void delete_path();

	// w.r.t scene height
	float height_factor() const { return height_factor_; }
	void  set_height_factor(float f);
	
	// w.r.t character height
	float distance_factor() const { return distance__factor_; }
	void  set_distance_factor(float f);

	// the actual height of the character
	float character_height() const;
	// the actual distance from character's current eye pos to camera pos
	float character_distance() const;

	// Places the person at idx_th position. Does nothing if the position is
	// not valid and returns the actual current position index.
	int move_to(int idx, bool animation = true, float duration = 0.5f);

	// convert the idx_th position in the path to a key frame
	Frame get_frame(int idx) const;
	// convert the entire path to key frames
	std::vector<Frame> get_key_frames() const;

	bool show_path() const { return show_path_; }
	void set_show_path(bool b) { show_path_ = b; }
	
	void draw() const;

protected:
	void  draw_path() const;
	void  draw_character() const;

	vec3  character_head(const vec3& pos) const;
	vec3  camera_position(const vec3& pos, const vec3& view_dir) const;

protected:
	bool	active_;
	Box3	scene_box_;
	int		current_position_idx_;

	// the height of the character w.r.t. the scene height
	float   height_factor_;
	// make the character step back a bit to simulate 3rd person view
	// w.r.t. the character's height.
	float	distance__factor_;

	bool	show_path_;

	struct StandPoint {
		StandPoint(const vec3& _pos, const vec3& _view_dir) : pos(_pos), view_dir(_view_dir) {}
		vec3 pos;
		vec3 view_dir;
	};
	std::vector<StandPoint> path_;
	bool buffer_up_to_date_;

	LinesDrawable*	character_drawable_;
	PointsDrawable* character_head_drawable_;
	LinesDrawable*	view_direction_base_drawable_;
	LinesDrawable*	view_direction_arraw_drawable_;
};


#endif // _OPENGL_WALK_THROUGH_H_
