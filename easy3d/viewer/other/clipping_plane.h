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

#ifndef _OPENGL_CLIPPING_PLANE_H_
#define _OPENGL_CLIPPING_PLANE_H_

#include <opengl/opengl_common.h>
#include <math/math_types.h>


class Frame;
class ShaderProgram;

class OPENGL_API ClippingPlane
{
public:
	ClippingPlane();
	virtual ~ClippingPlane();

	bool is_enabled() const { return enabled_; }
	void set_enabled(bool b) { enabled_ = b; }

	const Box3& scene_bbox() const { return scene_bbox_; }
	void fit_scene(const Box3& box);

	bool has_been_fit() const { return fit_scene_; }

	vec3 center() const;
	vec3 normal() const;

	Plane3 plane0() const;	// clipping plane0
	Plane3 plane1() const;	// clipping plane1

	bool cross_section() const { return cross_section_; }
	void set_cross_section(bool b) { cross_section_ = b; }

	// relative to scene bounding box. Default value is 0.01;
	float cross_section_width() { return cross_section_width_; }
	void  set_cross_section_width(float w) { cross_section_width_ = w; }

	void set_program(ShaderProgram* program, bool uniform_block = true);

	// arrow = true: also draw an arrow in the center of the plane. 
	void draw(bool arrow = false) const;

	virtual Frame* manipulated_frame();
	virtual const Frame* manipulated_frame() const;

protected:
	Frame*  manipulated_frame_;

	bool	enabled_;
	bool    cross_section_;
	float	cross_section_width_;

	float	size_;
	Box3	scene_bbox_;

	bool	fit_scene_;
};


#endif // _OPENGL_CLIPPING_PLANE_H_
