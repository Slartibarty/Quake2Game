
#include "phys_local.h"

#include "phys_system.h"

extern csurface_t s_nullsurface;

namespace PhysicsPrivate {

static constexpr float kCollisionTolerance = 1.0e-3f;
static constexpr float kCharacterPadding = 0.02f;
static constexpr float kMinRequiredPenetration = 0.005f + kCharacterPadding;

void CPhysicsSystem::Trace( const rayCast_t &rayCast, const IPhysicsShape *shapeHandle, const vec3_t shapeOrigin, const vec3_t shapeAngles, trace_t &trace )
{
	memset( &trace, 0, sizeof( trace ) );
	trace.surface = &s_nullsurface;
	trace.fraction = 1.0f;
	trace.contents = CONTENTS_SOLID;

	JPH::Vec3 origin = QuakePositionToJolt( rayCast.start );
	JPH::Vec3 direction = QuakePositionToJolt( rayCast.direction );
	JPH::Vec3 halfExtent = QuakePositionToJolt( rayCast.halfExtent );

	bool isRay = halfExtent.ReduceMin() < JPH::cDefaultConvexRadius;
	bool isCollide = direction.IsNearZero();

	const JPH::Shape *shape = reinterpret_cast<const JPH::Shape *>( shapeHandle );

	JPH::Vec3 position = QuakePositionToJolt( shapeOrigin );
	JPH::Quat rotation = QuakeEulerToJoltQuat( shapeAngles );

	JPH::Mat44 queryTransform = JPH::Mat44::sRotationTranslation( rotation, position + rotation * shape->GetCenterOfMass() );

	if ( isRay )
	{
		// Create ray
		JPH::RayCast joltRay = { origin, direction };
		// Transform our ray by the inverse of the collideOrigin/collideAngles
		joltRay = joltRay.Transformed( queryTransform.InversedRotationTranslation() );

		JPH::RayCastResult hit;

		// Cast away!
		bool bHit = shape->CastRay( joltRay, JPH::SubShapeIDCreator(), hit );

		trace.fraction = bHit ? hit.mFraction : 1.0f;

		// Compute trace.endpos
		vec3_t distance;
		VectorScale( rayCast.direction, trace.fraction, distance );
		VectorAdd( rayCast.start, distance, trace.endpos );

		trace.allsolid = trace.fraction == 0.0f;
		trace.startsolid = trace.fraction == 0.0f;

		// If we hit, we fill in the plane too
		if ( bHit )
		{
			// Set up the trace plane
			JPH::Vec3 normal = queryTransform.GetRotation() * shape->GetSurfaceNormal( hit.mSubShapeID2, joltRay.GetPointOnRay( hit.mFraction ) );

			VectorSet( trace.plane.normal, normal.GetX(), normal.GetY(), normal.GetZ() );
			trace.plane.dist = DotProduct( trace.endpos, trace.plane.normal );

			trace.contents = CONTENTS_SOLID;
		}
	}
	else
	{
		JPH::BoxShape boxShape( halfExtent );
		JPH::ShapeCast shapeCast( &boxShape, JPH::Vec3::sReplicate( 1.0f ), JPH::Mat44::sTranslation( origin ), direction );

		JPH::ShapeCastSettings settings;
		settings.mBackFaceModeTriangles = JPH::EBackFaceMode::CollideWithBackFaces;
		settings.mBackFaceModeConvex = JPH::EBackFaceMode::CollideWithBackFaces;
		settings.mUseShrunkenShapeAndConvexRadius = true;
		settings.mReturnDeepestPoint = true;

		JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
		JPH::CollisionDispatch::sCastShapeVsShapeWorldSpace( shapeCast, settings, shape, JPH::Vec3::sReplicate( 1.0f ), JPH::ShapeFilter(), queryTransform, JPH::SubShapeIDCreator(), JPH::SubShapeIDCreator(), collector );

		if ( collector.HadHit() )
		{
			// Get hit normal
			JPH::Vec3 normal = -( collector.mHit.mPenetrationAxis.Normalized() );
			VectorSet( trace.plane.normal, normal.GetX(), normal.GetY(), normal.GetZ() );

			// Compute fraction
			trace.fraction = Max( 0.0f, collector.mHit.mFraction + kCharacterPadding / normal.Dot( direction ) );

			// Compute endpos
			vec3_t distance;
			VectorScale( rayCast.direction, trace.fraction, distance );
			VectorAdd( rayCast.start, distance, trace.endpos );

			trace.plane.dist = DotProduct( trace.endpos, trace.plane.normal );
			trace.contents = CONTENTS_SOLID;

			trace.allsolid = collector.mHit.mPenetrationDepth > kMinRequiredPenetration && trace.fraction == 0.0f;
			trace.startsolid = collector.mHit.mPenetrationDepth > kMinRequiredPenetration && trace.fraction == 0.0f;
		}
		else
		{
			trace.fraction = 1.0f;
			vec3_t distance;
			VectorScale( rayCast.direction, trace.fraction, distance );
			VectorAdd( rayCast.start, distance, trace.endpos );

			// We didn't hit anything, so we must be completely free right?
			trace.allsolid = false;
			trace.startsolid = false;
		}
	}
}

} // namespace PhysicsPrivate
