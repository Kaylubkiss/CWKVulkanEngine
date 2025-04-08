#pragma once
#include <reactphysics3d/reactphysics3d.h>
using namespace reactphysics3d;


class RayCastObject : public reactphysics3d::RaycastCallback {
public:
	virtual decimal notifyRaycastHit(const RaycastInfo& info)
	{
		// Display the world hit point coordinates
		std::cout << " Hit point : " <<
			info.worldPoint.x << " " <<
			info.worldPoint.y << " " << 
			info.worldPoint.z << " " << 
			'\n';

		std::cout << "broad phase ID: " << info.collider->getBroadPhaseId();

		// Return a fraction of 1.0 to gather all hits
		return decimal(1.0);
	}
};

//for general objects.
struct PhysicsComponent
{
	reactphysics3d::RigidBody* rigidBody = nullptr;
	reactphysics3d::Collider* collider = nullptr;
	reactphysics3d::CollisionShape* shape = nullptr;

	reactphysics3d::Transform currTransform;
	reactphysics3d::Transform prevTransform;

	reactphysics3d::BodyType bodyType;
	enum ColliderType
	{
		NONE = 0,
		CUBE,
	};
	ColliderType colliderType;

	bool rayCastHit = false;

	PhysicsComponent() : rigidBody(nullptr), collider(nullptr), shape(nullptr),
		currTransform(reactphysics3d::Vector3::zero(), reactphysics3d::Quaternion::identity()),
		prevTransform(currTransform), bodyType(BodyType::STATIC), colliderType(ColliderType::NONE) {};

	
	void SetRayCastHit(bool set);

	PhysicsComponent(const PhysicsComponent& rhs) = default;
	const PhysicsComponent& operator=(const PhysicsComponent& rhs) 
	{
		if (this != &rhs) 
		{
			this->rigidBody = rhs.rigidBody;
			this->collider = rhs.collider;
			this->shape = rhs.shape;

			this->currTransform = rhs.currTransform;
			this->prevTransform = rhs.prevTransform;

			this->bodyType = rhs.bodyType;
			this->colliderType = rhs.colliderType;

			this->rayCastHit = rhs.rayCastHit;
		}

		return *this;
	}
};


//for managing the state of the physics simulation.
class PhysicsSystem 
{
	float mAccumulator = 0.f; //for updating the physics world.
	float interpFactor = 0.f; //for updating the objects for rendering.
	const double timeStep = 1/60.f; //for how fast the physics simulation should be.

	reactphysics3d::PhysicsCommon mPhysicsCommon;
	reactphysics3d::PhysicsWorld* mPhysicsWorld = nullptr;

public:
	PhysicsSystem() = default;
	void Init();
	float InterpFactor();
	void Update(float dt);
	reactphysics3d::PhysicsWorld* World();
	reactphysics3d::RigidBody* AddRigidBody(const reactphysics3d::Transform& transform);
	reactphysics3d::BoxShape* CreateBoxShape(const reactphysics3d::Vector3& extent);
	reactphysics3d::CapsuleShape* CreateCapsuleShape(float radius, float height);
	~PhysicsSystem();

};