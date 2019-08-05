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



#ifndef _OPENGL_TEXTURE_FORMAT_H_
#define _OPENGL_TEXTURE_FORMAT_H_

#include <opengl/opengl_common.h>
#include "glew.h"


class OPENGL_API TextureFormat
{
public:
	TextureFormat();
	TextureFormat(const TextureFormat& other);
	~TextureFormat();

	TextureFormat& operator=(const TextureFormat& other);
	bool operator==(const TextureFormat& other);

	// NOTE: now only GL_TEXTURE_2D and GL_TEXTURE_3D are supported. Default is GL_TEXTURE_2D.
	void   set_target(GLenum target);
	GLenum target() const;

	void   set_internal_format(GLenum internal_format);
	GLenum internal_format() const;

	// Sets the wrapping behavior when a texture coordinate falls outside the range of [0,1]. 
	// Possible values are GL_REPEAT, GL_CLAMP_TO_EDGE, etc. Default is GL_CLAMP_TO_EDGE.
	void   set_wrap(GLenum wrap_s, GLenum wrap_t);
	void   set_wrap_s(GLenum wrap_s);
	void   set_wrap_t(GLenum wrap_t);

	GLenum wrap_s() const;
	GLenum wrap_t() const;

	// Sets the filtering behavior when a texture is displayed at a lower resolution than its 
	// native resolution. Default is GL_LINEAR unless mipmapping is enabled, in which case 
	// GL_LINEAR_MIPMAP_LINEAR
	void   set_min_filter(GLenum min_filter);
	// Sets the filtering behavior when a texture is displayed at a higher resolution than its 
	// native resolution. Default is GL_LINEAR.
	void   set_mag_filter(GLenum mag_filter);
	void   set_filter(GLenum min_filter, GLenum mag_filter);

	GLenum min_filter() const;
	GLenum mag_filter() const;

private:
	GLenum	target_;			// default is TEXTURE_2D
	GLenum	internal_format_;

	GLenum  min_filter_;
	GLenum  mag_filter_;

	GLenum	wrap_s_;
	GLenum	wrap_t_;
};


#endif // _OPENGL_TEXTURE_FORMAT_H_
