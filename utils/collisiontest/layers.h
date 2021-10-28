// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#pragma once

#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"

/// Layer that objects can be in, determines which other objects it can collide with
namespace Layers
{
	static constexpr uint8 UNUSED1 = 0; // 3 unused values so that broadphase layers values don't match with object layer values (for testing purposes)
	static constexpr uint8 UNUSED2 = 1;
	static constexpr uint8 UNUSED3 = 2;
	static constexpr uint8 NON_MOVING = 3;
	static constexpr uint8 MOVING = 4;
	static constexpr uint8 NUM_LAYERS = 5;
};

/// Function that determines if two object layers can collide
inline bool ObjectCanCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2)
{
	switch (inObject1)
	{
	case Layers::UNUSED1:
	case Layers::UNUSED2:
	case Layers::UNUSED3:
		return false;
	case Layers::NON_MOVING:
		return inObject2 == Layers::MOVING;
	case Layers::MOVING:
		return inObject2 == Layers::NON_MOVING || inObject2 == Layers::MOVING;
	default:
		assert(0);
		return false;
	}
};

/// Broadphase layers
namespace BroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	static constexpr JPH::BroadPhaseLayer MOVING(1);
	static constexpr JPH::BroadPhaseLayer UNUSED(2);
};

/// Function that determines if two broadphase layers can collide
inline bool BroadPhaseCanCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2)
{
	switch (inLayer1)
	{
	case Layers::NON_MOVING:
		return inLayer2 == BroadPhaseLayers::MOVING;
	case Layers::MOVING:
		return inLayer2 == BroadPhaseLayers::NON_MOVING || inLayer2 == BroadPhaseLayers::MOVING;
	case Layers::UNUSED1:
	case Layers::UNUSED2:
	case Layers::UNUSED3:
		return false;			
	default:
		assert(0);
		return false;
	}
}

/// Create mapping table from layer to broadphase layer
inline JPH::ObjectToBroadPhaseLayer GetObjectToBroadPhaseLayer()
{
	JPH::ObjectToBroadPhaseLayer object_to_broadphase;
	object_to_broadphase.resize(Layers::NUM_LAYERS);
	object_to_broadphase[Layers::UNUSED1] = BroadPhaseLayers::UNUSED;
	object_to_broadphase[Layers::UNUSED2] = BroadPhaseLayers::UNUSED;
	object_to_broadphase[Layers::UNUSED3] = BroadPhaseLayers::UNUSED;
	object_to_broadphase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
	object_to_broadphase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	return object_to_broadphase;
}

/// Get name of broadphase layer for debugging purposes
inline const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer)
{
	switch ((JPH::BroadPhaseLayer::Type)inLayer)
	{
	case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
	case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
	case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::UNUSED:		return "UNUSED";
	default:													assert(0); return "INVALID";
	}
}