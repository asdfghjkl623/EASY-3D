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

#ifndef _OPENGL_SELECTABLE_H_
#define _OPENGL_SELECTABLE_H_

#include <opengl/opengl_common.h>
#include <basic/assertions.h>

#include <cstdint>
#include <vector>
#include <cassert>

/********************************************************************************************
*
* An Selectable manages the selection of its primitives, e.g., vertices, edges, faces, etc.
*     When selection changes, it controls the update of selection status cached in the GPU memory.
*     To make the data uploaded to GPU as minimum as possible, I use "1-bit-per-primitive", so an 
*	  value of type uint32_t can represent the status of 32 primitives.
*
*	Version:	 1.0
*	created:	 Dec. 14, 2015
*	author:		 Liangliang Nan
*	contact:     liangliang.nan@gmail.com
*
*********************************************************************************************/



// TODO: use BitArray

class OPENGL_API Selectable
{
public:
    Selectable() {}
	virtual ~Selectable() {}

	//--------------- selection of its primitives ----------------------

	virtual std::size_t num_primitives() const { return num_primitives_; }

	// returns the indices of selected primitives
	virtual std::vector<int> selected() const;
	// returns the number of selected primitives
    virtual int  num_selected() const;

    virtual bool is_selected(std::size_t idx) const;
	virtual void set_selected(std::size_t idx, bool b);

	virtual void invert_selection();
	virtual void clear_selection();

	// these two are here only for OpenGL shaders to use
    std::vector<uint32_t>& selections() { return selections_; }
    const std::vector<uint32_t>& selections() const { return selections_; }

protected:
	virtual void set_num_primitives(std::size_t num);

protected:
	std::size_t				num_primitives_; // the total number of elements that can be selected
    std::vector<uint32_t>	selections_;

	friend class Drawable;
};


inline bool Selectable::is_selected(std::size_t idx) const {
    assert(idx < num_primitives_);
	//std::size_t addr = idx / 32;
	//std::size_t offs = idx % 32;
	std::size_t addr = idx >> 5;
	std::size_t offs = idx & 31;
	if (addr >= selections_.size())
		return false;
	return (selections_[addr] & (1 << offs)) != 0;
}


inline void Selectable::set_selected(std::size_t idx, bool b) {
    assert(idx < num_primitives_);
	//std::size_t addr = idx / 32;
	//std::size_t offs = idx % 32;
	std::size_t addr = idx >> 5;
	std::size_t offs = idx & 31;
	if (addr >= selections_.size())
		return;
	if (b)	// select
		selections_[addr] |= (1 << offs);
	else	// deselect
		selections_[addr] &= ~(1 << offs);
}


#endif
