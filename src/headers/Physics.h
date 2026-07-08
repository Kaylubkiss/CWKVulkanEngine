#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include <reactphysics3d/reactphysics3d.h>

//for general objects.
struct alignas(16) PhysicsComponent
{
	reactphysics3d::Transform currTransform;
	reactphysics3d::Transform prevTransform;

	reactphysics3d::RigidBody* rigidBody = nullptr;
	reactphysics3d::Collider* collider = nullptr;
	reactphysics3d::CollisionShape* shape = nullptr;

	reactphysics3d::BodyType bodyType;
	enum ColliderType
	{
		NONE = 0,
		CUBE,
		PLANE,
	};
	ColliderType colliderType = NONE;

	bool isInitialized = false;
};


//for managing the state of the physics simulation.
class PhysicsSystem 
{
public:
	PhysicsSystem();
	~PhysicsSystem();

	float InterpFactor(float dt);
	reactphysics3d::PhysicsWorld* World() const;
	reactphysics3d::RigidBody* AddRigidBody(const reactphysics3d::Transform& transform);
	reactphysics3d::BoxShape* CreateBoxShape(const reactphysics3d::Vector3& extent);
	reactphysics3d::BoxShape* CreatePlaneShape(const reactphysics3d::Vector2 extent);
	reactphysics3d::CapsuleShape* CreateCapsuleShape(float radius, float height);
private:
	float mAccumulator = 0.f; //for updating the physics world.
	float interpFactor = 0.f; //for updating the objects for rendering.
	const reactphysics3d::decimal timeStep = 1/60.f; //for how fast the physics simulation should be.

	reactphysics3d::PhysicsCommon mPhysicsCommon;
	reactphysics3d::PhysicsWorld* mPhysicsWorld = nullptr;
};


class RayCastObject : public reactphysics3d::RaycastCallback {
public:
	reactphysics3d::decimal notifyRaycastHit(const reactphysics3d::RaycastInfo& info) override
	{
		// Display the world hit point coordinates
		std::cout << " Hit point : " <<
			info.worldPoint.x << " " <<
			info.worldPoint.y << " " <<
			info.worldPoint.z << " " <<
			'\n';

		std::cout << "broad phase ID: " << info.collider->getBroadPhaseId();

		// Return a fraction of 1.0 to gather all hits
		return 1.f;
	}
};

#endif