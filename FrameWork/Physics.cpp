#include "Physics.h"

Physics::Physics() 
{
	this->mPhysicsWorld = this->mPhysicsCommon.createPhysicsWorld();
}


Physics::~Physics() 
{
	this->mPhysicsCommon.destroyPhysicsWorld(this->mPhysicsWorld);
}

void Physics::Update(float dt) 
{
	this->mAccumulator += dt;

	while (this->mAccumulator >= this->timeStep)
	{
		this->mPhysicsWorld->update(this->timeStep);

		this->mAccumulator -= this->timeStep;
	}

	this->interpFactor = this->mAccumulator / this->timeStep;

}

PhysicsWorld* Physics::GetPhysicsWorld() 
{
	return this->mPhysicsWorld;
}
float Physics::InterpFactor() 
{
	return this->interpFactor;
}

reactphysics3d::RigidBody* Physics::AddRigidBody(const reactphysics3d::Transform& transform) 
{
	return mPhysicsWorld->createRigidBody(transform);
}

reactphysics3d::BoxShape* Physics::CreateBoxShape(const reactphysics3d::Vector3& extent) 
{
	return mPhysicsCommon.createBoxShape({extent.x, extent.y, extent.z});

}