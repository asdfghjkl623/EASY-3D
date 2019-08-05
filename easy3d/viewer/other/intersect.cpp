#include "intersect.h"
#include <cfloat>
#include <limits>


namespace OpenGL {

	bool intersectRayPlane(const vec3& orig, const vec3& dir,
		const vec3& planeOrig, const vec3& planeNormal,
		float& intersectionDistance)
	{
		float d = dot(dir, planeNormal);
		float Epsilon = std::numeric_limits<float>::epsilon();

		if (d < Epsilon)
		{
			intersectionDistance = dot(planeOrig - orig, planeNormal) / d;
			return true;
		}

		return false;
	}


	bool intersectRayTriangle(const vec3& orig, const vec3& dir,
		const vec3& v0, const vec3& v1, const vec3& v2,
		vec3& baryPosition)
	{
		vec3 e1 = v1 - v0;
		vec3 e2 = v2 - v0;

		vec3 p = cross(dir, e2);

		float a = dot(e1, p);

		float Epsilon = std::numeric_limits<float>::epsilon();
		if (a < Epsilon)
			return false;

		float f = float(1.0f) / a;

		vec3 s = orig - v0;
		baryPosition.x = f * dot(s, p);
		if (baryPosition.x < float(0.0f))
			return false;
		if (baryPosition.x > float(1.0f))
			return false;

		vec3 q = cross(s, e1);
		baryPosition.y = f * dot(dir, q);
		if (baryPosition.y < float(0.0f))
			return false;
		if (baryPosition.y + baryPosition.x > float(1.0f))
			return false;

		baryPosition.z = f * dot(e2, q);

		return baryPosition.z >= float(0.0f);
	}


	bool intersectLineTriangle(const vec3& orig, const vec3& dir,
		const vec3& vert0, const vec3& vert1, const vec3& vert2,
		vec3& position)
	{
		float Epsilon = std::numeric_limits<float>::epsilon();

		vec3 edge1 = vert1 - vert0;
		vec3 edge2 = vert2 - vert0;

		vec3 pvec = cross(dir, edge2);

		float det = dot(edge1, pvec);

		if (det > -Epsilon && det < Epsilon)
			return false;
		float inv_det = float(1) / det;

		vec3 tvec = orig - vert0;

		position.y = dot(tvec, pvec) * inv_det;
		if (position.y < float(0) || position.y > float(1))
			return false;

		vec3 qvec = cross(tvec, edge1);

		position.z = dot(dir, qvec) * inv_det;
		if (position.z < float(0) || position.y + position.z > float(1))
			return false;

		position.x = dot(edge2, qvec) * inv_det;

		return true;
	}


	bool intersectRaySphere(const vec3& rayStarting, const vec3& rayNormalizedDirection,
		const vec3& sphereCenter, const float sphereRadiusSquered,
		float& intersectionDistance)
	{
		float Epsilon = std::numeric_limits<float>::epsilon();
		vec3 diff = sphereCenter - rayStarting;
		float t0 = dot(diff, rayNormalizedDirection);
		float dSquared = dot(diff, diff) - t0 * t0;
		if (dSquared > sphereRadiusSquered)
		{
			return false;
		}
		float t1 = sqrt(sphereRadiusSquered - dSquared);
		intersectionDistance = t0 > t1 + Epsilon ? t0 - t1 : t0 + t1;
		return intersectionDistance > Epsilon;
	}


	bool intersectRaySphere(const vec3& rayStarting, const vec3& rayNormalizedDirection,
		const vec3& sphereCenter, const float sphereRadius,
		vec3& intersectionPosition, vec3& intersectionNormal)
	{
		float distance;
		if (intersectRaySphere(rayStarting, rayNormalizedDirection, sphereCenter, sphereRadius * sphereRadius, distance))
		{
			intersectionPosition = rayStarting + rayNormalizedDirection * distance;
			intersectionNormal = (intersectionPosition - sphereCenter) / sphereRadius;
			return true;
		}
		return false;
	}


	bool intersectLineSphere(const vec3& point0, const vec3& point1,
		const vec3& sphereCenter, float sphereRadius,
		vec3& intersectionPoint1, vec3& intersectionNormal1,
		vec3& intersectionPoint2, vec3& intersectionNormal2)
	{
		float Epsilon = std::numeric_limits<float>::epsilon();
		vec3 dir = normalize(point1 - point0);
		vec3 diff = sphereCenter - point0;
		float t0 = dot(diff, dir);
		float dSquared = dot(diff, diff) - t0 * t0;
		if (dSquared > sphereRadius * sphereRadius)
		{
			return false;
		}
		float t1 = sqrt(sphereRadius * sphereRadius - dSquared);
		if (t0 < t1 + Epsilon)
			t1 = -t1;
		intersectionPoint1 = point0 + dir * (t0 - t1);
		intersectionNormal1 = (intersectionPoint1 - sphereCenter) / sphereRadius;
		intersectionPoint2 = point0 + dir * (t0 + t1);
		intersectionNormal2 = (intersectionPoint2 - sphereCenter) / sphereRadius;
		return true;
	}

}