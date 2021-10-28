// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#pragma once

#include <Physics/Constraints/TwoBodyConstraint.h>
#include <Physics/Constraints/PathConstraintPath.h>
#include <Physics/Constraints/MotorSettings.h>
#include <Physics/Constraints/ConstraintPart/AxisConstraintPart.h>
#include <Physics/Constraints/ConstraintPart/DualAxisConstraintPart.h>
#include <Physics/Constraints/ConstraintPart/HingeRotationConstraintPart.h>
#include <Physics/Constraints/ConstraintPart/RotationQuatConstraintPart.h>

namespace JPH {

/// How to constrain the rotation of the body to a PathConstraint
enum class EPathRotationConstraintType
{
	Free,							///< Do not constrain the rotation of the body at all
	ConstrainAroundTangent,			///< Only allow rotation around the tangent vector (following the path)
	ConstrainAroundNormal,			///< Only allow rotation around the normal vector (perpendicular to the path)
	ConstrainAroundBinormal,		///< Only allow rotation around the binormal vector (perpendicular to the path)
	ConstaintToPath,				///< Fully constrain the rotation of body 2 to the path (follwing the tangent and normal of the path)
	FullyConstrained,				///< Fully constrain the rotation of the body 2 to the rotation of body 1
};

/// Point constraint settings, used to create a point constraint
class PathConstraintSettings final : public TwoBodyConstraintSettings
{
public:
	JPH_DECLARE_SERIALIZABLE_VIRTUAL(PathConstraintSettings)

	// See: ConstraintSettings::SaveBinaryState
	virtual void					SaveBinaryState(StreamOut &inStream) const override;

	/// Create an an instance of this constraint
	virtual TwoBodyConstraint *		Create(Body &inBody1, Body &inBody2) const override;

	/// The path that constrains the two bodies
	RefConst<PathConstraintPath>	mPath;

	/// The position of the path start relative to world transform of body 1
	Vec3							mPathPosition = Vec3::sZero();

	/// The rotation of the path start relative to world transform of body 1
	Quat							mPathRotation = Quat::sIdentity();

	/// The fraction along the path that corresponds to the initial position of body 2. Usually this is 0, the beginning of the path. But if you want to start an object halfway the path you can calculate this with mPath->GetClosestPoint(point on path to attach body to).
	float							mPathFraction = 0.0f;

	/// Maximum amount of friction force to apply (N) when not driven by a motor.
	float							mMaxFrictionForce = 0.0f;

	/// In case the constraint is powered, this determines the motor settings along the path
	MotorSettings					mPositionMotorSettings;

	/// How to constrain the rotation of the body to the path
	EPathRotationConstraintType		mRotationConstraintType = EPathRotationConstraintType::Free;

protected:
	// See: ConstraintSettings::RestoreBinaryState
	virtual void					RestoreBinaryState(StreamIn &inStream) override;
};

/// A point constraint constrains 2 bodies on a single point (removing 3 degrees of freedom)
class PathConstraint final : public TwoBodyConstraint
{
public:
	/// Construct point constraint
									PathConstraint(Body &inBody1, Body &inBody2, const PathConstraintSettings &inSettings);

	// Generic interface of a constraint
	virtual EConstraintType			GetType() const override								{ return EConstraintType::Path; }
	virtual void					SetupVelocityConstraint(float inDeltaTime) override;
	virtual void					WarmStartVelocityConstraint(float inWarmStartImpulseRatio) override;
	virtual bool					SolveVelocityConstraint(float inDeltaTime) override;
	virtual bool					SolvePositionConstraint(float inDeltaTime, float inBaumgarte) override;
#ifdef JPH_STAT_COLLECTOR
	virtual void					CollectStats() const override;
#endif // JPH_STAT_COLLECTOR
#ifdef JPH_DEBUG_RENDERER
	virtual void					DrawConstraint(DebugRenderer *inRenderer) const override;
#endif // JPH_DEBUG_RENDERER
	virtual void					SaveState(StateRecorder &inStream) const override;
	virtual void					RestoreState(StateRecorder &inStream) override;
	virtual bool					IsActive() const override								{ return TwoBodyConstraint::IsActive() && mPath != nullptr; }

	// See: TwoBodyConstraint
	virtual Mat44					GetConstraintToBody1Matrix() const override				{ return mPathToBody1; }
	virtual Mat44					GetConstraintToBody2Matrix() const override				{ return mPathToBody2; }

	/// Update the path for this constraint
	void							SetPath(const PathConstraintPath *inPath, float inPathFraction);

	/// Access to the current path
	const PathConstraintPath *		GetPath() const											{ return mPath; }

	/// Access to the current fraction along the path e [0, GetPath()->GetMaxPathFraction()]
	float							GetPathFraction() const									{ return mPathFraction; }
	
	/// Friction control
	void							SetMaxFrictionForce(float inFrictionForce)				{ mMaxFrictionForce = inFrictionForce; }
	float							GetMaxFrictionForce() const								{ return mMaxFrictionForce; }

	/// Position motor settings
	MotorSettings &					GetPositionMotorSettings()								{ return mPositionMotorSettings; }
	const MotorSettings &			GetPositionMotorSettings() const						{ return mPositionMotorSettings; }

	// Position motor controls (drives body 2 along the path)
	void							SetPositionMotorState(EMotorState inState)				{ JPH_ASSERT(inState == EMotorState::Off || mPositionMotorSettings.IsValid()); mPositionMotorState = inState; }
	EMotorState						GetPositionMotorState() const							{ return mPositionMotorState; }
	void							SetTargetVelocity(float inVelocity)						{ mTargetVelocity = inVelocity; }
	float							GetTargetVelocity() const								{ return mTargetVelocity; }
	void							SetTargetPathFraction(float inFraction)					{ JPH_ASSERT(mPath->IsLooping() || (inFraction >= 0.0f && inFraction <= mPath->GetPathMaxFraction())); mTargetPathFraction = inFraction; }
	float							GetTargetPathFraction() const							{ return mTargetPathFraction; }

private:
	// Internal helper function to calculate the values below
	void							CalculateConstraintProperties(float inDeltaTime);

	// CONFIGURATION PROPERTIES FOLLOW

	RefConst<PathConstraintPath>	mPath;													///< The path that attaches the two bodies
	Mat44							mPathToBody1;											///< Transform that takes a quantity from path space to body 1 center of mass space
	Mat44							mPathToBody2;											///< Transform that takes a quantity from path space to body 2 center of mass space
	EPathRotationConstraintType		mRotationConstraintType;								///< How to constrain the rotation of the path

	// Friction
	float							mMaxFrictionForce;

	// Motor controls
	MotorSettings					mPositionMotorSettings;
	EMotorState						mPositionMotorState = EMotorState::Off;
	float							mTargetVelocity = 0.0f;
	float							mTargetPathFraction = 0.0f;

	// RUN TIME PROPERTIES FOLLOW

	// Positions where the point constraint acts on in world space
	Vec3							mR1;
	Vec3							mR2;

	// X2 + R2 - X1 - R1
	Vec3							mU;

	// World space path tangent
	Vec3							mPathTangent;

	// Normals to the path tangent
	Vec3							mPathNormal;
	Vec3							mPathBinormal;

	// Inverse of initial rotation from body 1 to body 2 in body 1 space (only used when rotation constraint type is FullyConstrained)
	Quat							mInvInitialOrientation;

	// Current fraction along the path where body 2 is attached
	float							mPathFraction = 0.0f;

	// Translation constraint parts
	DualAxisConstraintPart			mPositionConstraintPart;								///< Constraint part that keeps the movement along the tangent of the path
	AxisConstraintPart				mPositionLimitsConstraintPart;							///< Constraint part that prevents movement beyond the beginning and end of the path
	AxisConstraintPart				mPositionMotorConstraintPart;							///< Constraint to drive the object along the path or to apply friction

	// Rotation constraint parts
	HingeRotationConstraintPart		mHingeConstraintPart;									///< Constraint part that removes 2 degrees of rotation freedom
	RotationQuatConstraintPart		mRotationConstraintPart;								///< Constraint part that removes all rotational freedom
};

} // JPH