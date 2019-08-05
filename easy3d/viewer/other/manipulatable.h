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

#ifndef _OPENGL_MANIPULATABLE_H_
#define _OPENGL_MANIPULATABLE_H_

#include <opengl/opengl_common.h>
#include "constraint.h"
#include <math/math_types.h>

#include <set>

/* TODO: 
*  move the ManipulatedFrame from canvas module to OpenGL module, then it would be
*  easier to attach each model a ManipulatedFrame.
*/

/********************************************************************************************
*
* A Manipulatable manages
*  1) a manipulated frame attached on it. This frame is used to manipulate this object.
*  2) a clipping plane frame attached on it. This frame is used to manipulate the cross
*     section plane of this object.
*
*	Version:	 1.0
*	created:	 Dec. 14, 2015
*	author:		 Liangliang Nan
*	contact:     liangliang.nan@gmail.com
*
*********************************************************************************************/


class Model;
class Frame;


// NOTE: the frame is always at the center of the object

class OPENGL_API Manipulatable
{
public:
	// the model to be manipulated
	Manipulatable(Model* model);
	virtual ~Manipulatable();

	// reset the manipulated frame
	virtual void reset_manipulated_frame();

	//------------------  manipulation ----------------------

	virtual Frame* manipulated_frame();
	virtual const Frame* manipulated_frame() const;

	// This is the extra transformation introduced by the manipulation.
	// NOTE: Rotation is performed around 'center'
	virtual mat4 manipulated_matrix() const;

	// ------------------- rendering ------------------------

	virtual void draw_manipulated_frame() const;

protected:
	Model*  model_; // the model to be manipulated
	Frame*  manipulated_frame_;
};


// Liangliang: ManipulationAgent is intended for manipulating multiple objects. Using it 
//             is quite simply: providing the frames to ManipulationAgent. Manipulating 
//			   single objects is easier, you just need to provide your ManipulatedFrame 
//			   by calling canvas()->setManipulatedFrame(your_frame). NOTE: this is not 
//             supported by now, I have to move ManipulatedFrame to opengl module.
class OPENGL_API ManipulationAgent : public Constraint
{
public:
	void clear();

	void add_frame(Frame* frame);	// the frame to be manipulated
	void remove_frame(Frame* frame);// the frame to be excluded from manipulation

	virtual void constrainTranslation(vec3 &translation, Frame *const frame);
	virtual void constrainRotation(quat &rotation, Frame *const frame);

private:
	std::set<Frame*> frames_;
};


#endif
