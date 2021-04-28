/*
===================================================================================================

	Mathematics!

===================================================================================================
*/

#pragma once

#include <cmath>

#include <numbers> // std::numbers

#include "sys_types.h"

#ifdef M_PI
#error M_PI was defined by an external library
#endif

//=================================================================================================

#ifndef _In_reads_
#define _In_reads_( x )
#endif

struct vec3
{
	union
	{
		struct
		{
			float x, y, z;
		};
		float v[3];
	};

	vec3() = default;

	vec3( const vec3 & ) = default;
	vec3 &operator=( const vec3 & ) = default;

	vec3( vec3 && ) = default;
	vec3 &operator=( vec3 && ) = default;

	constexpr vec3( float X, float Y, float Z ) noexcept : x( X ), y( Y ), z( Z ) {}
	//explicit vec3( _In_reads_( 3 ) const float *pArray ) noexcept : x( pArray[0] ), y( pArray[1] ), z( pArray[2] ) {}

	// I wonder how the compiler deals with this
	friend bool operator==( const vec3 &, const vec3 & ) = default;

	void Set( float X, float Y, float Z )
	{
		x = X; y = Y; z = Z;
	}

	void SetFromLegacy( const float *pArray )
	{
		x = pArray[0]; y = pArray[1]; z = pArray[2];
	}

	void Replicate( float f )
	{
		x = f; y = f; z = f;
	}

	void Zero()
	{
		x = 0.0f; y = 0.0f; z = 0.0f;
	}

	float Length() const
	{
		return sqrt( x*x + y*y + z*z );
	}

	float Normalize()
	{
		float length = Length();

		// prevent divide by zero
		float ilength = 1.0f / ( length + FLT_EPSILON );

		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;

		return length;
	}

	void NormalizeFast()
	{
		// prevent divide by zero
		float ilength = 1.0f / ( Length() + FLT_EPSILON );

		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	float *Base()
	{
		return reinterpret_cast<float *>( this );
	}

	const float *Base() const
	{
		return reinterpret_cast<const float *>( this );
	}

	// avoid, use vec.v[i]
	float &vec3::operator[]( int i )
	{
		return v[i];
	}

	// avoid, use vec.v[i]
	float vec3::operator[]( int i ) const
	{
		return v[i];
	}
};

// ensure triviality
static_assert( std::is_trivial<vec3>::value, "vec3 is not trivial" );

inline float Vec3DotProduct( const vec3 &v1, const vec3 &v2 )
{
	return ( v1.x*v2.x + v1.y*v2.y + v1.z*v2.z );
}

inline void Vec3Add( const vec3 &veca, const vec3 &vecb, vec3 &out )
{
	out.x = veca.x + vecb.x;
	out.y = veca.y + vecb.y;
	out.z = veca.z + vecb.z;
}

inline void Vec3Subtract( const vec3 &veca, const vec3 &vecb, vec3 &out )
{
	out.x = veca.x - vecb.x;
	out.y = veca.y - vecb.y;
	out.z = veca.z - vecb.z;
}

inline void Vec3Multiply( const vec3 &veca, const vec3 &vecb, vec3 &out )
{
	out.x = veca.x * vecb.x;
	out.y = veca.y * vecb.y;
	out.z = veca.z * vecb.z;
}

inline void Vec3Lerp( const vec3 &src1, const vec3 &src2, float t, vec3 &out )
{
	out.x = src1.x + ( src2.x - src1.x ) * t;
	out.y = src1.y + ( src2.y - src1.y ) * t;
	out.z = src1.z + ( src2.z - src1.z ) * t;
}

inline void Vec3Copy( const vec3 &in, vec3 &out )
{
	out = in;
}

inline bool Vec3Compare( const vec3 &v1, const vec3 &v2 )
{
	return v1 == v2;
}

inline void Vec3Test()
{
	vec3 vec1( 4.0f, 1.0f, 1.0f );
	vec3 vec2( 8.0f, 3.0f, 2.0f );

	if ( Vec3Compare( vec1, vec2 ) )
	{

	}
}

//=================================================================================================

typedef float vec_t;			// deprecated, use float instead
typedef float vec3_t[3];
typedef float vec4_t[4];
typedef float vec5_t[5];

extern vec3_t vec3_origin;		// avoid, use VectorClear

// plane_t structure
struct cplane_t
{
	vec3_t	normal;
	float	dist;
	byte	type;			// for fast side tests
	byte	signbits;		// signx + (signy<<1) + (signz<<1)
};

// angle indices
#define	PITCH		0		// up / down
#define	YAW			1		// left / right
#define	ROLL		2		// fall over

// side indices
#define	SIDE_FRONT		0
#define	SIDE_ON			2
#define	SIDE_BACK		1
#define	SIDE_CROSS		-2

// utils glue
#define EQUAL_EPSILON	0.001f

#define M_PI		3.14159265358979323846
#define M_PI_F		3.14159265358979323846f

// these suck, the namespace name also sucks
// Use std::numbers instead of these
namespace mconst
{
	constexpr float		Pi = 3.14159265358979323846f;
	constexpr float		TwoPi = 2.0f * Pi;
	constexpr float		HalfPi = 0.5f * Pi;
	constexpr float		OneFourth_Pi = 0.25f * Pi;
	constexpr float		OneOver_Pi = 1.0f / Pi;
	constexpr float		OneOver_TwoPi = 1.0f / TwoPi;
	constexpr float		E = 2.71828182845904523536f;
	constexpr float		Sqrt_2 = 1.41421356237309504880f;
	constexpr float		Sqrt_3 = 1.73205080756887729352f;
	constexpr float		Sqrt_1Over2 = 0.70710678118654752440f;
	constexpr float		Sqrt_1Over3 = 0.57735026918962576450f;
	constexpr float		DegreesToRadians = Pi / 180.0f;
	constexpr float		RadiansToDegrees = 180.0f / Pi;
	constexpr float		SecondsToMilliseconds = 1000.0f;
	constexpr float		MillisecondsToSeconds = 0.001f;
}

/*
===================================================================================================

	Unit conversions using C++20 concepts

===================================================================================================
*/

namespace mpriv
{
	constexpr double DegreesToRadians = std::numbers::pi / 180.0;
	constexpr double RadiansToDegrees = 180.0 / std::numbers::pi;
}

// floating point

template< std::floating_point T >
constexpr T DEG2RAD( T degrees ) {
	return degrees * static_cast<T>( mpriv::DegreesToRadians );
}

template< std::floating_point T >
constexpr T RAD2DEG( T radians ) {
	return radians * static_cast<T>( mpriv::RadiansToDegrees );
}

template< std::floating_point T >
constexpr T SEC2MS( T seconds ) {
	return seconds * static_cast<T>( 1000.0 );
}

template< std::floating_point T >
constexpr T MS2SEC( T milliseconds ) {
	return milliseconds * static_cast<T>( 0.001 );
}

// integers

template< std::integral T >
constexpr T SEC2MS( T seconds ) {
	static_assert( sizeof( T ) >= sizeof( int32 ) );
	return seconds * static_cast<T>( 1000 );
}

// Avoid using this... It'll return 0 if you input a value below 1000, use floats
template< std::integral T >
constexpr T MS2SEC( T milliseconds ) {
	static_assert( sizeof( T ) >= sizeof( int32 ) );
	return milliseconds / static_cast<T>( 1000 );
}

/*
===================================================================================================

	Miscellaneous

===================================================================================================
*/

inline float LerpAngle( float a2, float a1, float frac )
{
	if ( a1 - a2 > 180.0f ) {
		a1 -= 360.0f;
	}
	if ( a1 - a2 < -180.0f ) {
		a1 += 360.0f;
	}
	return a2 + frac * ( a1 - a2 );
}

inline float anglemod( float a )
{
	return ( 360.0f / 65536.0f ) * ( (int)( a * ( 65536.0f / 360.0f ) ) & 65535 );
}

// obsolete, utils glue
inline int Q_rint( float a )
{
	return static_cast<int>( a + 0.5f );
}

/*
===================================================================================================

	Vector maths

===================================================================================================
*/

inline float DotProduct( const vec3_t v1, const vec3_t v2 )
{
	return ( v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2] );
}

inline float DotProductAbs( const vec3_t v1, const vec3_t v2 )
{
	return ( fabs( v1[0] * v2[0] ) + fabs( v1[1] * v2[1] ) + fabs( v1[2] * v2[2] ) );
}

inline void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross )
{
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

inline float VectorLength( vec3_t v )
{
	return sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
}

inline void VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
}

inline void VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
}

inline void VectorMultiply( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
	out[0] = veca[0] * vecb[0];
	out[1] = veca[1] * vecb[1];
	out[2] = veca[2] * vecb[2];
}

inline void VectorLerp( const vec3_t src1, const vec3_t src2, float t, vec3_t out )
{
	out[0] = src1[0] + ( src2[0] - src1[0] ) * t;
	out[1] = src1[1] + ( src2[1] - src1[1] ) * t;
	out[2] = src1[2] + ( src2[2] - src1[2] ) * t;
}

inline void VectorCopy( const vec3_t in, vec3_t out )
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

inline void VectorClear( vec3_t vec )
{
	vec[0] = 0.0f;
	vec[1] = 0.0f;
	vec[2] = 0.0f;
}

inline void VectorNegate( const vec3_t in, vec3_t out )
{
	out[0] = -in[0];
	out[1] = -in[1];
	out[2] = -in[2];
}

inline void VectorSet( vec3_t v, float x, float y, float z )
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

inline void VectorSetAll( vec3_t v, float f )
{
	v[0] = f;
	v[1] = f;
	v[2] = f;
}

inline void VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc )
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
}

inline void VectorScale( const vec3_t in, float scale, vec3_t out )
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

inline bool VectorCompare( const vec3_t v1, const vec3_t v2 )
{
	return ( v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2] );
}

inline void ClearBounds( vec3_t mins, vec3_t maxs )
{
	mins[0] = mins[1] = mins[2] = 99999.0f;
	maxs[0] = maxs[1] = maxs[2] = -99999.0f;
}

// returns vector length
float VectorNormalize( vec3_t v );
float ColorNormalize( const vec3_t in, vec3_t out );
float VectorDistance( const vec3_t v1, const vec3_t v2 );

void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up );

void AddPointToBounds( vec3_t v, vec3_t mins, vec3_t maxs );

void R_ConcatRotations( float in1[3][3], float in2[3][3], float out[3][3] );
void R_ConcatTransforms( float in1[3][4], float in2[3][4], float out[3][4] );

// for fast box on planeside test
int SignbitsForPlane( const cplane_t &in );
// Returns 1, 2, or 1 + 2
int BoxOnPlaneSide( const vec3_t emins, const vec3_t emaxs, const cplane_t *plane );

#define BOX_ON_PLANE_SIDE BoxOnPlaneSide

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal );
void PerpendicularVector( vec3_t dst, const vec3_t src );
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );

// converts radian-euler axis aligned angles to a quaternion
void AngleQuaternion( const vec3_t angles, vec4_t quaternion );
void QuaternionMatrix( const vec4_t quaternion, float( *matrix )[4] );
void QuaternionSlerp( const vec4_t p, vec4_t q, float t, vec4_t qt );

// legacy macros
#define _DotProduct(x,y)		(x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define _VectorSubtract(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2])
#define _VectorAdd(a,b,c)		(c[0]=a[0]+b[0],c[1]=a[1]+b[1],c[2]=a[2]+b[2])
#define _VectorCopy(a,b)		(b[0]=a[0],b[1]=a[1],b[2]=a[2])
#define _VectorClear(a)			(a[0]=a[1]=a[2]=0)
#define _VectorNegate(a,b)		(b[0]=-a[0],b[1]=-a[1],b[2]=-a[2])
#define _VectorSet(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))
