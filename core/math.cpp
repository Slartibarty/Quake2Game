/*
===================================================================================================

	Mathematics!

===================================================================================================
*/

#include "core.h"

#include "math.h"

vec3_t vec3_origin;

/*
===================================================================================================

	Vectors

===================================================================================================
*/

float VectorNormalize( vec3_t v )
{
	float length = VectorLength( v );

	// prevent divide by zero
	float ilength = 1.0f / ( length + FLT_EPSILON );

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;

	return length;
}

float ColorNormalize( const vec3_t in, vec3_t out )
{
	float max, scale;

	max = in[0];
	if ( in[1] > max )
		max = in[1];
	if ( in[2] > max )
		max = in[2];

	if ( max == 0.0f )
		return 0;

	scale = 1.0f / max;

	VectorScale( in, scale, out );

	return max;
}

float VectorDistance( const vec3_t v1, const vec3_t v2 )
{
	vec3_t delta;
	VectorSubtract( v1, v2, delta );
	return VectorLength( delta );
}

void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up )
{
	float angle;
	float sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * ( M_PI_F * 2 / 360 );
	sy = sinf( angle );
	cy = cosf( angle );
	angle = angles[PITCH] * ( M_PI_F * 2 / 360 );
	sp = sinf( angle );
	cp = cosf( angle );
	angle = angles[ROLL] * ( M_PI_F * 2 / 360 );
	sr = sinf( angle );
	cr = cosf( angle );

	if ( forward )
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if ( right )
	{
		right[0] = ( -1 * sr * sp * cy + -1 * cr * -sy );
		right[1] = ( -1 * sr * sp * sy + -1 * cr * cy );
		right[2] = -1 * sr * cp;
	}
	if ( up )
	{
		up[0] = ( cr * sp * cy + -sr * -sy );
		up[1] = ( cr * sp * sy + -sr * cy );
		up[2] = cr * cp;
	}
}

void AddPointToBounds( vec3_t v, vec3_t mins, vec3_t maxs )
{
	int i;
	vec_t val;

	for ( i = 0; i < 3; i++ )
	{
		val = v[i];
		if ( val < mins[i] )
			mins[i] = val;
		if ( val > maxs[i] )
			maxs[i] = val;
	}
}

void R_ConcatRotations( float in1[3][3], float in2[3][3], float out[3][3] )
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
}

void R_ConcatTransforms( float in1[3][4], float in2[3][4], float out[3][4] )
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
				in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
				in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
				in1[2][2] * in2[2][3] + in1[2][3];
}

int SignbitsForPlane( const cplane_t &in )
{
	int	bits, j;

	bits = 0;
	for ( j = 0; j < 3; j++ )
	{
		if ( in.normal[j] < 0.0f ) {
			bits |= 1 << j;
		}
	}
	return bits;
}

int BoxOnPlaneSide( const vec3_t emins, const vec3_t emaxs, const cplane_t *p )
{
	float dist1, dist2;
	int sides;

	// fast axial cases
	if ( p->type < 3 )
	{
		if ( p->dist <= emins[p->type] )
			return 1;
		if ( p->dist >= emaxs[p->type] )
			return 2;
		return 3;
	}

	// general case
	switch ( p->signbits )
	{
	case 0:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 1:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 2:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 3:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 4:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	default:
		dist1 = 0.0f;
		dist2 = 0.0f;
		Assert( 0 );
		break;
	}

	sides = 0;
	if ( dist1 >= p->dist ) {
		sides = 1;
	}
	if ( dist2 < p->dist ) {
		sides |= 2;
	}

	Assert( sides != 0 );

	return sides;
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	const float inv_denom = 1.0f / DotProduct( normal, normal );
	const float d = DotProduct( normal, p ) * inv_denom;

	dst[0] = p[0] - d * ( normal[0] * inv_denom );
	dst[1] = p[1] - d * ( normal[1] * inv_denom );
	dst[2] = p[2] - d * ( normal[2] * inv_denom );
}

void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int	pos;
	int i;
	float minelem = 1.0f;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabs( src[i] ) < minelem )
		{
			pos = i;
			minelem = fabsf( src[i] );
		}
	}
	VectorClear( tempvec );
	tempvec[pos] = 1.0f;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees )
{
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int		i;
	vec3_t	vr, vup, vf;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector( vr, dir );
	CrossProduct( vr, vf, vup );

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy( im, m, sizeof( im ) );

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0f;

	zrot[0][0] = cos( DEG2RAD( degrees ) );
	zrot[0][1] = sin( DEG2RAD( degrees ) );
	zrot[1][0] = -sin( DEG2RAD( degrees ) );
	zrot[1][1] = cos( DEG2RAD( degrees ) );

	R_ConcatRotations( m, zrot, tmpmat );
	R_ConcatRotations( tmpmat, im, rot );

	for ( i = 0; i < 3; i++ )
	{
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

void AngleQuaternion( const vec3_t angles, vec4_t quat )
{
	float angle;
	float sr, sp, sy, cr, cp, cy;

	angle = DEG2RAD( angles[1] ) * 0.5f;
	sy = sin( angle );
	cy = cos( angle );
	angle = DEG2RAD( angles[0] ) * 0.5f;
	sp = sin( angle );
	cp = cos( angle );
	angle = DEG2RAD( angles[2] ) * 0.5f;
	sr = sin( angle );
	cr = cos( angle );

	quat[0] = sr * cp * cy - cr * sp * sy; // X
	quat[1] = cr * sp * cy + sr * cp * sy; // Y
	quat[2] = cr * cp * sy - sr * sp * cy; // Z
	quat[3] = cr * cp * cy + sr * sp * sy; // W
}

void QuaternionAngles( const vec4_t quat, vec3_t angles )
{
	// FIXME: doing it this way calculates too much data, needs to do an optimized version...
	float matrix[3][4];
	QuaternionMatrix( quat, matrix );
	MatrixAngles( matrix, angles );
}

void QuaternionMatrix( const vec4_t quat, float( *matrix )[4] )
{
	matrix[0][0] = 1.0f - 2.0f * quat[1] * quat[1] - 2.0f * quat[2] * quat[2];
	matrix[1][0] = 2.0f * quat[0] * quat[1] + 2.0f * quat[3] * quat[2];
	matrix[2][0] = 2.0f * quat[0] * quat[2] - 2.0f * quat[3] * quat[1];

	matrix[0][1] = 2.0f * quat[0] * quat[1] - 2.0f * quat[3] * quat[2];
	matrix[1][1] = 1.0f - 2.0f * quat[0] * quat[0] - 2.0f * quat[2] * quat[2];
	matrix[2][1] = 2.0f * quat[1] * quat[2] + 2.0f * quat[3] * quat[0];

	matrix[0][2] = 2.0f * quat[0] * quat[2] + 2.0f * quat[3] * quat[1];
	matrix[1][2] = 2.0f * quat[1] * quat[2] - 2.0f * quat[3] * quat[0];
	matrix[2][2] = 1.0f - 2.0f * quat[0] * quat[0] - 2.0f * quat[1] * quat[1];

	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
}

void QuaternionSlerp( const vec4_t p, vec4_t q, float t, vec4_t qt )
{
	int i;
	float omega, cosom, sinom, sclp, sclq;

	// decide if one of the quaternions is backwards
	float a = 0.0f;
	float b = 0.0f;
	for ( i = 0; i < 4; i++ ) {
		a += ( p[i] - q[i] ) * ( p[i] - q[i] );
		b += ( p[i] + q[i] ) * ( p[i] + q[i] );
	}
	if ( a > b ) {
		for ( i = 0; i < 4; i++ ) {
			q[i] = -q[i];
		}
	}

	cosom = p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];

	constexpr float epsilon = 0.000001f;

	if ( ( 1.0f + cosom ) > epsilon ) {
		if ( ( 1.0f - cosom ) > epsilon ) {
			omega = acos( cosom );
			sinom = sin( omega );
			sclp = sin( ( 1.0f - t ) * omega ) / sinom;
			sclq = sin( t * omega ) / sinom;
		}
		else {
			sclp = 1.0f - t;
			sclq = t;
		}
		for ( i = 0; i < 4; i++ ) {
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	}
	else {
		qt[0] = -p[1];
		qt[1] = p[0];
		qt[2] = -p[3];
		qt[3] = p[2];
		sclp = sin( ( 1.0f - t ) * 0.5f * M_PI_F );
		sclq = sin( t * 0.5f * M_PI_F );
		for ( i = 0; i < 3; i++ ) {
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
}

void MatrixAngles( const float( *matrix )[4], float *angles )
{
	float forward[3];
	float left[3];
	float up[3];

	//
	// Extract the basis vectors from the matrix. Since we only need the Z
	// component of the up vector, we don't get X and Y.
	//
	forward[0] = matrix[0][0];
	forward[1] = matrix[1][0];
	forward[2] = matrix[2][0];
	left[0] = matrix[0][1];
	left[1] = matrix[1][1];
	left[2] = matrix[2][1];
	up[2] = matrix[2][2];

	float xyDist = sqrtf( forward[0] * forward[0] + forward[1] * forward[1] );

	// enough here to get angles?
	if ( xyDist > 0.001f )
	{
		// (yaw)	y = ATAN( forward.y, forward.x );		-- in our space, forward is the X axis
		angles[1] = RAD2DEG( atan2f( forward[1], forward[0] ) );

		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG( atan2f( -forward[2], xyDist ) );

		// (roll)	z = ATAN( left.z, up.z );
		angles[2] = RAD2DEG( atan2f( left[2], up[2] ) );
	}
	else	// forward is mostly Z, gimbal lock-
	{
		// (yaw)	y = ATAN( -left.x, left.y );			-- forward is mostly z, so use right for yaw
		angles[1] = RAD2DEG( atan2f( -left[0], left[1] ) );

		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG( atan2f( -forward[2], xyDist ) );

		// Assume no roll in this case as one degree of freedom has been lost (i.e. yaw == roll)
		angles[2] = 0.0f;
	}
}

/*
	  ____             _    _
	 |  _ \           | |  | |
	 | |_) |_ __ _   _| |__| |
	 |  _ <| '__| | | |  __  |
	 | |_) | |  | |_| | |  | |
	 |____/|_|   \__,_|_|  |_|

*/
