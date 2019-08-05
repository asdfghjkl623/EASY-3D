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

#ifndef _MPL_OPENGL_FXAA_H_
#define _MPL_OPENGL_FXAA_H_


#include <opengl/opengl_common.h>

#include <string>


// Fast Approximate Anti-Aliasing(FXAA) algorithm created by Timothy Lottes under NVIDIA.
// see http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf

/*
 * How to use it?
 * 
 * in the end of your fragment shader, add the following line (NOTE: not work with blending!)
 *		outputF.a = dot(outputF.rgb, vec3(0.299, 0.587, 0.114));
 *
 * code example: 
 * 		fxaa_->begin(screen_width, screen_height);
 *		draw(); // put your rendering code here
 *		fxaa_->end();
 *
 */

class Camera;
class ShaderProgram;
class FramebufferObject;

class OPENGL_API Fxaa
{
public:
	Fxaa(unsigned int samples = 0);
	~Fxaa();

	void begin(int screen_width, int screen_height);
	void end();

private:
	FramebufferObject*	fbo_;
	unsigned int		fbo_samples_;
	float				bkg_color_[4];

	ShaderProgram*  shader_;
	static bool		try_load_shader_;
};


#endif
