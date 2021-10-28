// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#pragma once

#include <Geometry/AABox.h>
#include <Physics/Collision/Shape/Shape.h>
#include <Physics/Collision/Shape/SubShapeID.h>
#include <Physics/Collision/Shape/ConvexShape.h>
#include <atomic>

namespace JPH {

class CollideShapeSettings;

/// Collision detection helper that collides a convex object vs one or more triangles
class CollideConvexVsTriangles
{
public:
	/// Constructor
	/// @param inShape1 The convex shape to collide against triangles
	/// @param inScale1 Local space scale for the convex object
	/// @param inScale2 Local space scale for the triangles
	/// @param inCenterOfMassTransform1 Transform that takes the center of mass of 1 into world space
	/// @param inCenterOfMassTransform2 Transform that takes the center of mass of 2 into world space
	/// @param inSubShapeID1 Sub shape ID of the convex object
	/// @param inCollideShapeSettings Settings for the collide shape query
	/// @param ioCollector The collector that will receive the results
									CollideConvexVsTriangles(const ConvexShape *inShape1, Vec3Arg inScale1, Vec3Arg inScale2, Mat44Arg inCenterOfMassTransform1, Mat44Arg inCenterOfMassTransform2, const SubShapeID &inSubShapeID1, const CollideShapeSettings &inCollideShapeSettings, CollideShapeCollector &ioCollector);

	/// Collide convex object with a single triangle
	/// @param inV0 , inV1 , inV2: CCW triangle vertices
	/// @param inActiveEdges bit 0 = edge v0..v1 is active, bit 1 = edge v1..v2 is active, bit 2 = edge v2..v0 is active
	/// An active edge is an edge that is not connected to another triangle in such a way that it is impossible to collide with the edge
	/// @param inSubShapeID2 The sub shape ID for the triangle
	void							Collide(Vec3Arg inV0, Vec3Arg inV1, Vec3Arg inV2, uint8 inActiveEdges, const SubShapeID &inSubShapeID2);

#ifdef JPH_STAT_COLLECTOR
	/// Collect stats of the previous time step
	static void						sResetStats();
	static void						sCollectStats();
#endif // JPH_STAT_COLLECTOR

protected:
#ifdef JPH_STAT_COLLECTOR
	// Statistics
	alignas(JPH_CACHE_LINE_SIZE) static atomic<int>	sNumCollideChecks;
	alignas(JPH_CACHE_LINE_SIZE) static atomic<int>	sNumGJKChecks;
	alignas(JPH_CACHE_LINE_SIZE) static atomic<int>	sNumEPAChecks;
	alignas(JPH_CACHE_LINE_SIZE) static atomic<int>	sNumCollisions;
#endif // JPH_STAT_COLLECTOR

	const CollideShapeSettings &	mCollideShapeSettings;					///< Settings for this collision operation
	CollideShapeCollector &			mCollector;								///< The collector that will receive the results
	const ConvexShape *				mShape1;								///< The shape that we're colliding with
	Vec3							mScale1;								///< The scale of the shape (in shape local space) of the shape we're colliding with
	Vec3							mScale2;								///< The scale of the shape (in shape local space) of the shape we're colliding against
	Mat44							mTransform1;							///< Transform of the shape we're colliding with
	Mat44							mTransform2To1;							///< Transform that takes a point in space of the colliding shape to the shape we're colliding with
	AABox							mBoundsOf1;								///< Bounds of the colliding shape in local space
	AABox							mBoundsOf1InSpaceOf2;					///< Bounds of the colliding shape in space of shape we're colliding with
	SubShapeID						mSubShapeID1;							///< Sub shape ID of colliding shape
	float							mScaleSign2;							///< Sign of the scale of object 2, -1 if object is inside out, 1 if not
	ConvexShape::SupportBuffer		mBufferExCvxRadius;						///< Buffer that holds the support function data excluding convex radius
	ConvexShape::SupportBuffer		mBufferIncCvxRadius;					///< Buffer that holds the support function data including convex radius
	const ConvexShape::Support *	mShape1ExCvxRadius = nullptr;			///< Actual support function object excluding convex radius
	const ConvexShape::Support *	mShape1IncCvxRadius = nullptr;			///< Actual support function object including convex radius
};

} // JPH