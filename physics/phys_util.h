
#pragma once

namespace PhysicsPrivate {

inline constexpr float QuakeToJoltFactor = 0.0254f;				// Factor to convert inches to metres
inline constexpr float JoltToQuakeFactor = 39.3700787402f;		// Factor to convert metres to inches

inline constexpr float QuakeToJolt( float x ) { return x * QuakeToJoltFactor; }
inline constexpr float JoltToQuake( float x ) { return x * JoltToQuakeFactor; }

//
// Position
//

inline JPH::Vec3 QuakePositionToJolt( const vec3_t vec )
{
	return JPH::Vec3( QuakeToJolt( vec[0] ), QuakeToJolt( vec[1] ), QuakeToJolt( vec[2] ) );
}

inline void JoltPositionToQuake( JPH::Vec3Arg vec, vec3_t out )
{
	vec *= JoltToQuakeFactor;
	vec.StoreFloat3( (JPH::Float3 *)out );
}

//
// Rotation
//

// Converts Quake Euler angles to a Jolt Quaternion
inline JPH::Quat QuakeEulerToJoltQuat( const vec3_t euler )
{
	vec4_t quat;
	AngleQuaternion( euler, quat );
	return JPH::Quat( quat[0], quat[1], quat[2], quat[3] );
}

inline void JoltQuatToQuakeEuler( JPH::QuatArg quat, vec3_t euler )
{
	vec4_t quakeQuat;
	quat.mValue.StoreFloat4( reinterpret_cast<JPH::Float4 *>( quakeQuat ) );
	QuaternionAngles( quakeQuat, euler );
}

inline JPH::EMotionType QuakeMotionTypeToJPHMotionType( motionType_t type )
{
	return static_cast<JPH::EMotionType>( type ); // Types are equivalent
}

} // namespace PhysicsPrivate
