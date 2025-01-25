#pragma once
#include <reactphysics3d/reactphysics3d.h>
using namespace reactphysics3d;

//for general objects.
struct PhysicsComponent
{
	reactphysics3d::RigidBody* rigidBody = nullptr;
	reactphysics3d::Collider* collider = nullptr;
	reactphysics3d::CollisionShape* shape = nullptr;

	reactphysics3d::Transform currTransform;
	reactphysics3d::Transform prevTransform;

	reactphysics3d::BodyType bodyType;

	bool rayCastHit = false;

	PhysicsComponent() : rigidBody(nullptr), collider(nullptr), shape(nullptr),
		currTransform(reactphysics3d::Vector3::zero(), reactphysics3d::Quaternion::identity()),
		prevTransform(currTransform) {};

	void SetRayCastHit(bool set);


	//void operator=(const PhysicsComponent& rhs);
};


//for managing the state of the physics simulation.
class Physics 
{
	float mAccumulator = 0.f; //for updating the physics world.
	float interpFactor = 0.f; //for updating the objects for rendering.
	const float timeStep = 1.f / 60; //for how fast the physics simulation should be.

	reactphysics3d::PhysicsCommon mPhysicsCommon;
	reactphysics3d::PhysicsWorld* mPhysicsWorld = nullptr;

public:
	Physics();
	float InterpFactor();
	void Update(float dt);
	reactphysics3d::PhysicsWorld* GetPhysicsWorld();
	reactphysics3d::RigidBody* AddRigidBody(const reactphysics3d::Transform& transform);
	reactphysics3d::BoxShape* CreateBoxShape(const reactphysics3d::Vector3& extent);
	reactphysics3d::CapsuleShape* CreateCapsuleShape(float radius, float height);
	~Physics();

};