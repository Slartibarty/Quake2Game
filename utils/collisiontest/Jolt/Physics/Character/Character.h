// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#pragma once

#include <Core/Reference.h>
#include <Physics/EActivation.h>

namespace JPH {

class Character;
class PhysicsSystem;

/// Contains the configuration of a character
class CharacterSettings : public RefTarget<CharacterSettings>
{
public:
	/// Layer that this character will be added to
	ObjectLayer							mLayer = 0;

	/// Mass of the character
	float								mMass = 80.0f;

	/// Maximum angle of slope that character can still walk on (radians)
	float								mMaxSlopeAngle = DegreesToRadians(50.0f);

	/// Friction for the character
	float								mFriction = 0.2f;

	/// Value to multiply gravity with for this character
	float								mGravityFactor = 1.0f;

	/// Initial shape that represents the character's volume.
	/// Usually this is a capsule, make sure the shape is made so that the bottom of the shape is at (0, 0, 0).
	RefConst<Shape>						mShape;
};

/// Runtime character object.
/// This object usually represents the player or a humanoid AI. It uses a single rigid body, 
/// usually with a capsule shape to simulate movement and collision for the character.
/// The character is a keyframed object, the application controls it by setting the velocity.
class Character : public RefTarget<Character>
{
public:
	/// Constructor
	/// @param inSettings The settings for the character
	/// @param inPosition Initial position for the character
	/// @param inRotation Initial rotation for the character (usually only around Y)
	/// @param inUserData Application specific data pointer
	/// @param inSystem Physics system that this character will be added to later
										Character(CharacterSettings *inSettings, Vec3Arg inPosition, QuatArg inRotation, void *inUserData, PhysicsSystem *inSystem);

	/// Destructor
										~Character();

	/// Add bodies and constraints to the system and optionally activate the bodies
	void								AddToPhysicsSystem(EActivation inActivationMode = EActivation::Activate, bool inLockBodies = true);

	/// Remove bodies and constraints from the system
	void								RemoveFromPhysicsSystem(bool inLockBodies = true);

	/// Wake up the character
	void								Activate(bool inLockBodies = true);

	/// Needs to be called after every PhysicsSystem::Update
	/// @param inMaxSeparationDistance Max distance between the floor and the character to still consider the character standing on the floor
	/// @param inLockBodies If the collision query should use the locking body interface (true) or the non locking body interface (false)
	void								PostSimulation(float inMaxSeparationDistance, bool inLockBodies = true);

	/// Control the velocity of the character
	void								SetLinearAndAngularVelocity(Vec3Arg inLinearVelocity, Vec3Arg inAngularVelocity, bool inLockBodies = true);

	/// Get the linear velocity of the character (m / s)
	Vec3								GetLinearVelocity(bool inLockBodies = true) const;

	/// Set the linear velocity of the character (m / s)
	void								SetLinearVelocity(Vec3Arg inLinearVelocity, bool inLockBodies = true);

	/// Add world space linear velocity to current velocity (m / s)
	void								AddLinearVelocity(Vec3Arg inLinearVelocity, bool inLockBodies = true);

	/// Add impulse to the center of mass of the character
	void								AddImpulse(Vec3Arg inImpulse, bool inLockBodies = true);

	/// Get the body associated with this character
	BodyID								GetBodyID() const										{ return mBodyID; }

	/// Get position / rotation of the body
	void								GetPositionAndRotation(Vec3 &outPosition, Quat &outRotation, bool inLockBodies = true) const;

	/// Set the position / rotation of the body, optionally activating it.
	void								SetPositionAndRotation(Vec3Arg inPosition, QuatArg inRotation, EActivation inActivationMode = EActivation::Activate, bool inLockBodies = true) const;

	/// Get the position of the character
	Vec3								GetPosition(bool inLockBodies = true) const;

	/// Set the position of the character, optionally activating it.
	void								SetPosition(Vec3Arg inPostion, EActivation inActivationMode = EActivation::Activate, bool inLockBodies = true);

	/// Get the rotation of the character
	Quat								GetRotation(bool inLockBodies = true) const;
	
	/// Set the rotation of the character, optionally activating it.
	void								SetRotation(QuatArg inRotation, EActivation inActivationMode = EActivation::Activate, bool inLockBodies = true);

	/// Position of the center of mass of the underlying rigid body
	Vec3								GetCenterOfMassPosition(bool inLockBodies = true) const;

	/// Update the layer of the character
	void								SetLayer(ObjectLayer inLayer, bool inLockBodies = true);

	/// Set the maximum angle of slope that character can still walk on (radians)
	void								SetMaxSlopeAngle(float inMaxSlopeAngle)					{ mCosMaxSlopeAngle = cos(inMaxSlopeAngle); }

	/// Switch the shape of the character (e.g. for stance). When inMaxPenetrationDepth is not FLT_MAX, it checks
	/// if the new shape collides before switching shape. Returns true if the switch succeeded.
	bool								SetShape(const Shape *inShape, float inMaxPenetrationDepth, bool inLockBodies = true);

	/// Get the current shape that the character is using.
	const Shape *						GetShape() const										{ return mShape; }

	enum class EGroundState
	{
		OnGround,						///< Character is on the ground and can move freely
		Sliding,						///< Character is on a slope that is too steep and should start sliding
		InAir,							///< Character is in the air
	};

	///@name Properties of the ground this character is standing on

	/// Current ground state (updated during PostSimlulation())
	EGroundState						GetGroundState() const;

	/// Get the contact point with the ground
	Vec3 								GetGroundPosition() const								{ return mGroundPosition; }

	/// Get the contact normal with the ground
	Vec3	 							GetGroundNormal() const									{ return mGroundNormal; }

	/// Velocity in world space of the point that we're standing on
	Vec3								GetGroundVelocity(bool inLockBodies = true) const;
	
	/// Material that the character is standing on.
	const PhysicsMaterial *				GetGroundMaterial() const								{ return mGroundMaterial; }

	/// BodyID of the object the character is standing on. Note may have been removed!
	BodyID								GetGroundBodyID() const									{ return mGroundBodyID; }

	/// Sub part of the body that we're standing on.
	SubShapeID							GetGroundSubShapeID() const								{ return mGroundBodySubShapeID; }

	/// User data pointer of the body that we're standing on
	void *								GetGroundUserData(bool inLockBodies = true) const;

private:
	/// Check collisions between inShape and the world
	void								CheckCollision(const Shape *inShape, float inMaxSeparationDistance, CollideShapeCollector &ioCollector, bool inLockBodies = true) const;

	/// The body of this character
	BodyID								mBodyID;

	/// The layer the body is in
	ObjectLayer							mLayer;

	/// Cosine of the maximum angle of slope that character can still walk on
	float								mCosMaxSlopeAngle;

	/// The shape that the body currently has
	RefConst<Shape>						mShape;

	/// Cached physics system
	PhysicsSystem *						mSystem;

	// Ground properties
	BodyID								mGroundBodyID;
	SubShapeID							mGroundBodySubShapeID;
	Vec3								mGroundPosition = Vec3::sZero();
	Vec3								mGroundNormal = Vec3::sZero();
	RefConst<PhysicsMaterial>			mGroundMaterial;
};

} // JPH