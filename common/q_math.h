// q_math.h

#pragma once

#include <cmath>

#include "q_defs.h"
#include "q_types.h"

//-------------------------------------------------------------------------------------------------
//
// New Maths
//
//-------------------------------------------------------------------------------------------------

namespace math
{
	struct vec3
	{
		float x, y, z;

		vec3()
		{
		}

		vec3(float X, float Y, float Z)
			: x(X), y(Y), z(Z)
		{
		}

		void Clear()
		{
			x = 0;
			y = 0;
			z = 0;
		}

		void Set(float X, float Y, float Z)
		{
			x = X;
			y = Y;
			z = Z;
		}

		float Length()
		{
			return sqrtf(x*x + y*y + z*z);
		}

		float Normalize()
		{
			float	length, ilength;

			length = x * x + y * y + z * z;
			length = sqrtf( length );

			if ( length )
			{
				ilength = 1 / length;
				x *= ilength;
				y *= ilength;
				z *= ilength;
			}

			return length;
		}

		void Scale(float scale)
		{
			x *= scale;
			y *= scale;
			z *= scale;
		}

		void Negate()
		{
			x = -x;
			y = -y;
			z = -z;
		}

		void AddBy(const vec3 &v)
		{
			x += v.x;
			y += v.y;
			z += v.z;
		}

		void SubtractBy(const vec3 &v)
		{
			x -= v.x;
			y -= v.y;
			z -= v.z;
		}

		void MultiplyBy(const vec3 &v)
		{
			x *= v.x;
			y *= v.y;
			z *= v.z;
		}

		void DivideBy(const vec3 &v)
		{
			x /= v.x;
			y /= v.y;
			z /= v.z;
		}

	};

	inline float DotProduct(const vec3 & v1, const vec3& v2)
	{
		return (v1.x*v2.x + v1.y*v2.y + v1.z*v2.z);
	}

	inline void CrossProduct(const vec3& v1, const vec3& v2, vec3& cross)
	{
		cross.x = v1.y*v2.z - v1.z*v2.y;
		cross.y = v1.z*v2.x - v1.x*v2.z;
		cross.z = v1.x*v2.y - v1.y*v2.x;
	}

	inline void VectorSubtract(const vec3& veca, const vec3& vecb, vec3& out)
	{
		out.x = veca.x - vecb.x;
		out.y = veca.y - vecb.y;
		out.z = veca.z - vecb.z;
	}

	inline void VectorAdd(const vec3& veca, const vec3& vecb, vec3& out)
	{
		out.x = veca.x + vecb.x;
		out.y = veca.y + vecb.y;
		out.z = veca.z + vecb.z;
	}

	inline void VectorCopy(const vec3& in, vec3& out)
	{
		out.x = in.x;
		out.y = in.y;
		out.z = in.z;
	}

	inline void VectorMA(const vec3& veca, float scale, const vec3& vecb, vec3& out)
	{
		out.x = veca.x + scale * vecb.x;
		out.y = veca.y + scale * vecb.y;
		out.z = veca.z + scale * vecb.z;
	}

	inline void VectorScale(const vec3& in, float scale, vec3& out)
	{
		out.x = in.x * scale;
		out.y = in.y * scale;
		out.z = in.z * scale;
	}

	inline bool VectorCompare(const vec3& v1, const vec3& v2)
	{
		return (v1.x != v2.x || v1.y != v2.y || v1.z != v2.z);
	}

	inline void ClearBounds(vec3& mins, vec3& maxs)
	{
		mins.x = mins.y = mins.z = 99999;
		maxs.x = maxs.y = maxs.z = -99999;
	}

}

//-------------------------------------------------------------------------------------------------
//
// Old maths
//
//-------------------------------------------------------------------------------------------------

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

extern vec3_t vec3_origin;

// angle indexes
#define	PITCH		0		// up / down
#define	YAW			1		// left / right
#define	ROLL		2		// fall over

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#ifndef M_PI_F
#define M_PI_F		3.14159265358979323846f
#endif

// These suck, the namespace name also sucks
namespace mathconst
{
	constexpr float	Pi = 3.14159265358979323846f;
	constexpr float	TwoPi = 2.0f * Pi;
	constexpr float	HalfPi = 0.5f * Pi;
	constexpr float	OneFourth_Pi = 0.25f * Pi;
	constexpr float OneOver_Pi = 1.0f / Pi;
	constexpr float OneOver_TwoPi = 1.0f / TwoPi;
	constexpr float E = 2.71828182845904523536f;
	constexpr float Sqrt_2 = 1.41421356237309504880f;
	constexpr float Sqrt_3 = 1.73205080756887729352f;
	constexpr float	Sqrt_1Over2 = 0.70710678118654752440f;
	constexpr float	Sqrt_1Over3 = 0.57735026918962576450f;
	constexpr float	DegreesToRadians = Pi / 180.0f;
	constexpr float	RadiansToDegrees = 180.0f / Pi;
	constexpr float	SecondsToMilliseconds = 1000.0f;
	constexpr float	MillisecondsToSeconds = 0.001f;
}

struct cplane_t;

inline float DotProduct(const vec3_t v1, const vec3_t v2)
{
	return (v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2]);
}

inline void CrossProduct(const vec3_t v1, const vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

inline float VectorLength(vec3_t v)
{
	return sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

inline void VectorSubtract(const vec3_t veca, const vec3_t vecb, vec3_t out)
{
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
}

inline void VectorAdd(const vec3_t veca, const vec3_t vecb, vec3_t out)
{
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
}

inline void VectorMultiply(const vec3_t veca, const vec3_t vecb, vec3_t out)
{
	out[0] = veca[0] * vecb[0];
	out[1] = veca[1] * vecb[1];
	out[2] = veca[2] * vecb[2];
}

inline void VectorMultiply(const vec3_t veca, const float b, vec3_t out)
{
	out[0] = veca[0] * b;
	out[1] = veca[1] * b;
	out[2] = veca[2] * b;
}

inline void VectorCopy(const vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

inline void VectorClear(vec3_t vec)
{
	vec[0] = 0.0f;
	vec[1] = 0.0f;
	vec[2] = 0.0f;
}

inline void VectorNegate(const vec3_t in, vec3_t out)
{
	out[0] = -in[0];
	out[1] = -in[1];
	out[2] = -in[2];
}

inline void VectorSet(vec3_t v, float x, float y, float z)
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

inline void VectorMA(const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
}

inline void VectorScale(const vec3_t in, float scale, vec3_t out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

inline void VectorRotate( const vec3_t in1, const float in2[3][4], vec3_t out )
{
	out[0] = DotProduct( in1, in2[0] );
	out[1] = DotProduct( in1, in2[1] );
	out[2] = DotProduct( in1, in2[2] );
}

// rotate by the inverse of the matrix
inline void VectorIRotate( const vec3_t in1, const float in2[3][4], vec3_t out )
{
	out[0] = in1[0] * in2[0][0] + in1[1] * in2[1][0] + in1[2] * in2[2][0];
	out[1] = in1[0] * in2[0][1] + in1[1] * in2[1][1] + in1[2] * in2[2][1];
	out[2] = in1[0] * in2[0][2] + in1[1] * in2[1][2] + in1[2] * in2[2][2];
}

inline void VectorTransform( const vec3_t in1, const float in2[3][4], vec3_t out )
{
	out[0] = DotProduct( in1, in2[0] ) + in2[0][3];
	out[1] = DotProduct( in1, in2[1] ) + in2[1][3];
	out[2] = DotProduct( in1, in2[2] ) + in2[2][3];
}

inline bool VectorCompare(const vec3_t v1, const vec3_t v2)
{
	return (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2]);
}

inline void ClearBounds(vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999.0f;
	maxs[0] = maxs[1] = maxs[2] = -99999.0f;
}

inline float LerpAngle(float a2, float a1, float frac)
{
	if (a1 - a2 > 180)
		a1 -= 360;
	if (a1 - a2 < -180)
		a1 += 360;
	return a2 + frac * (a1 - a2);
}

inline float anglemod(float a)
{
	return (360.0f / 65536.0f) * ((int)(a * (65536.0f / 360.0f)) & 65535);
}

inline float DegreesToRadians( float degrees )
{
	return degrees * mathconst::DegreesToRadians;
}

inline float RadiansToDegrees( float radians )
{
	return radians * mathconst::RadiansToDegrees;
}

void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs);
vec_t VectorNormalize (vec3_t v);		// returns vector length

void R_ConcatRotations (float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms (float in1[3][4], float in2[3][4], float out[3][4]);

void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, cplane_t *plane);

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal );
void PerpendicularVector( vec3_t dst, const vec3_t src );
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );

void AngleQuaternion( const vec3_t angles, vec4_t quaternion );
void QuaternionMatrix( const vec4_t quaternion, float( *matrix )[4] );
void QuaternionSlerp( const vec4_t p, vec4_t q, float t, vec4_t qt );

#define _DotProduct(x,y)		(x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define _VectorSubtract(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2])
#define _VectorAdd(a,b,c)		(c[0]=a[0]+b[0],c[1]=a[1]+b[1],c[2]=a[2]+b[2])
#define _VectorCopy(a,b)		(b[0]=a[0],b[1]=a[1],b[2]=a[2])
#define _VectorClear(a)			(a[0]=a[1]=a[2]=0)
#define _VectorNegate(a,b)		(b[0]=-a[0],b[1]=-a[1],b[2]=-a[2])
#define _VectorSet(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))
