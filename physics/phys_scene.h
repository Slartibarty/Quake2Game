
#pragma once

namespace PhysicsPrivate {

class CPhysicsScene final : public IPhysicsScene
{
public:
	CPhysicsScene();
	~CPhysicsScene();

	void Simulate( float deltaTime ) override;

	virtual IPhysicsBody *CreateAndAddBody( const bodyCreationSettings_t &settings, IPhysicsShape *pShape, void *pUserData ) override;
	virtual void CreateAndAddBody_World( void *vertexList, void *indexList ) override; // HACK
	virtual void SetWorldBodyUserData( void *pUserData ) override; // HACK
	virtual void RemoveAndDestroyBody( IPhysicsBody *pBody ) override;

	virtual void *GetInternalStructure() override;

private:
	// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
	static constexpr uint cMaxBodies = 16384;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	static constexpr uint cNumBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	static constexpr uint cMaxBodyPairs = cMaxBodies;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	static constexpr uint cMaxContactConstraints = cMaxBodies;

	JPH::PhysicsSystem m_physicsSystem;
	JPH::Body *m_pWorldBody = nullptr; // HACK

};

} // namespace PhysicsPrivate
