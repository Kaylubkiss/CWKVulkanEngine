#include "Physics.h"

PhysicsSystem::PhysicsSystem()
{
	this->mPhysicsWorld = this->mPhysicsCommon.createPhysicsWorld();
}

PhysicsSystem::~PhysicsSystem()
{
	this->mPhysicsCommon.destroyPhysicsWorld(this->mPhysicsWorld);
}

void PhysicsSystem::Update(float dt)
{
	this->mAccumulator += dt;

	while (this->mAccumulator >= this->timeStep)
	{
		this->mPhysicsWorld->update(this->timeStep);

		this->mAccumulator -= this->timeStep;
	}

	this->interpFactor = this->mAccumulator / this->timeStep;

}

PhysicsWorld* PhysicsSystem::GetPhysicsWorld()
{
	return this->mPhysicsWorld;

}

float PhysicsSystem::InterpFactor()
{
	return this->interpFactor;
}

reactphysics3d::RigidBody* PhysicsSystem::AddRigidBody(const reactphysics3d::Transform& transform)
{
	return mPhysicsWorld->createRigidBody(transform);
}

reactphysics3d::BoxShape* PhysicsSystem::CreateBoxShape(const reactphysics3d::Vector3& extent) 
{
	return mPhysicsCommon.createBoxShape({extent.x, extent.y, extent.z});
}

reactphysics3d::CapsuleShape* PhysicsSystem::CreateCapsuleShape(float radius, float height)
{
	return mPhysicsCommon.createCapsuleShape(radius, height);
}

void PhysicsComponent::SetRayCastHit(bool set) 
{
	this->rayCastHit = set;	
}
