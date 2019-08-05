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

#ifndef _MPL_OPENGL_INTERSECT_H_
#define _MPL_OPENGL_INTERSECT_H_


#include <opengl/opengl_common.h>
#include <math/math_types.h>

// There are many other intersection routines available for all sorts of collision shapes; 
// see http://www.realtimerendering.com/intersections.html for instance.

// Intersection test functions

namespace OpenGL {

	//! Compute the intersection of a ray and a triangle.
	//! Ray direction and plane normal must be unit length.
	//! From GLM_GTX_intersect extension.

	OPENGL_API bool intersectRayPlane(
		const vec3& orig, const vec3& dir,
		const vec3& planeOrig, const vec3& planeNormal,
		float& intersectionDistance);

	//! Compute the intersection of a ray and a triangle.
	//! From GLM_GTX_intersect extension.

	OPENGL_API bool intersectRayTriangle(
		const vec3& orig, const vec3& dir,
		const vec3& vert0, const vec3& vert1, const vec3& vert2,
		vec3& baryPosition);

	//! Compute the intersection of a line and a triangle.
	//! From GLM_GTX_intersect extension.

	OPENGL_API bool intersectLineTriangle(
		const vec3& orig, const vec3& dir,
		const vec3& vert0, const vec3& vert1, const vec3& vert2,
		vec3& position);

	//! Compute the intersection distance of a ray and a sphere. 
	//! The ray direction vector is unit length.
	//! From GLM_GTX_intersect extension.

	OPENGL_API bool intersectRaySphere(
		const vec3& rayStarting, const vec3& rayNormalizedDirection,
		const vec3& sphereCenter, float sphereRadiusSquered,
		float& intersectionDistance);

	//! Compute the intersection of a ray and a sphere.
	//! From GLM_GTX_intersect extension.

	OPENGL_API bool intersectRaySphere(
		const vec3& rayStarting, const vec3& rayNormalizedDirection,
		const vec3& sphereCenter, float sphereRadius,
		vec3& intersectionPosition, vec3& intersectionNormal);

	//! Compute the intersection of a line and a sphere.
	//! From GLM_GTX_intersect extension

	OPENGL_API bool intersectLineSphere(
		const vec3& point0, const vec3& point1,
		const vec3& sphereCenter, float sphereRadius,
		vec3& intersectionPosition1, vec3& intersectionNormal1,
        vec3& intersectionPosition2, vec3& intersectionNormal2);

}


#endif
