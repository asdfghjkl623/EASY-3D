#include "euler_angles.h"

namespace OpenGL
{
	mat4 euler_angle_x(float angleX) {
		float cosX = std::cos(angleX);
		float sinX = std::sin(angleX);

		return mat4(
			1,     0,     0,    0,
			0,  cosX,  -sinX,   0,
			0,  sinX,   cosX,   0,
			0,     0,      0,   1);
	}


	mat4 euler_angle_y(float angleY) {
		float cosY = std::cos(angleY);
		float sinY = std::sin(angleY);

		return mat4(
			 cosY,    0,  sinY, 0,
			    0,    1,     0, 0,
			-sinY,    0,  cosY, 0,
			    0,    0,     0, 1);
	}


	mat4 euler_angle_z(float angleZ) {
		float cosZ = std::cos(angleZ);
		float sinZ = std::sin(angleZ);

		return mat4(
			cosZ, -sinZ, 0, 0,
			sinZ,  cosZ, 0, 0,
			0,        0, 1, 0,
			0,        0, 0, 1);
	}

	// see the equations here
	// http://inside.mines.edu/fs_home/gmurray/ArbitraryAxisRotation/
	mat4 euler_angle_xyz(float x, float y, float z) {
		float cr = std::cos(x);
		float sr = std::sin(x);
		float cp = std::cos(y);
		float sp = std::sin(y);
		float cy = std::cos(z);
		float sy = std::sin(z);

		mat4 result(1);
		result(0, 0) = cp*cy;
		result(1, 0) = cp*sy;
		result(2, 0) = -sp;

		float srsp = sr*sp;
		float crsp = cr*sp;

		result(0, 1) = srsp*cy - cr*sy;
		result(1, 1) = srsp*sy + cr*cy;
		result(2, 1) = sr*cp;

		result(0, 2) = crsp*cy + sr*sy;
		result(1, 2) = crsp*sy - sr*cy;
		result(2, 2) = cr*cp;

		return result;
	}

	mat4 euler_angle_xyz(const vec3& angles) {
		return euler_angle_xyz(angles.x, angles.y, angles.z);
	}

	// Code taken from glm/gtx/euler_angles.inl  (GLM 0.9.8.0 - 2016-09-11)
	// Mode detailed explanations can be found at
	// http://gamedev.stackexchange.com/questions/50963/how-to-extract-euler-angles-from-transformation-matrix
	void extract_euler_angles(const mat4& M, float& angle_x, float& angle_y, float& angle_z) {
		float T1 = std::atan2(M(2, 1), M(2, 2));
		float C2 = std::sqrt(M(0, 0) * M(0, 0) + M(1, 0) * M(1, 0));
		float T2 = std::atan2(-M(2, 0), C2);
		float S1 = std::sin(T1);
		float C1 = std::cos(T1);
		float T3 = std::atan2(S1*M(0, 2) - C1*M(0, 1), C1*M(1, 1) - S1*M(1, 2));
		// Liangliang: why these negative signs? This must be a bug!!!
		//angle_x = -T1;
		//angle_y = -T2;
		//angle_z = -T3;
		angle_x = T1;
		angle_y = T2;
		angle_z = T3;
	}


}//namespace OpenGL
