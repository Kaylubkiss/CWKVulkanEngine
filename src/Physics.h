#ifndef PHYSICS_HPP
#define PHYSICS_HPP

//for general objects.
struct PhysicsComponent
{
	reactphysics3d::Transform currTransform;
	reactphysics3d::Transform prevTransform;

	reactphysics3d::RigidBody* rigidBody = nullptr;
	reactphysics3d::Collider* collider = nullptr;
	reactphysics3d::CollisionShape* shape = nullptr;

	glm::vec3 scale = glm::vec3(0);
};


//for managing the state of the physics simulation.
class PhysicsSystem 
{
public:
	PhysicsSystem();
	~PhysicsSystem();

	[[nodiscard]] float InterpFactor(float dt);
	[[nodiscard]] reactphysics3d::PhysicsWorld* World() const;
	[[nodiscard]] reactphysics3d::RigidBody* AddRigidBody(const reactphysics3d::Transform& transform);
	[[nodiscard]] reactphysics3d::BoxShape* CreateBoxShape(const reactphysics3d::Vector3& extent);
	[[nodiscard]] reactphysics3d::BoxShape* CreatePlaneShape(reactphysics3d::Vector2 extent);
	[[nodiscard]] reactphysics3d::CapsuleShape* CreateCapsuleShape(float radius, float height);
private:
	reactphysics3d::PhysicsCommon mPhysicsCommon;
	reactphysics3d::PhysicsWorld* mPhysicsWorld = nullptr;
	float mAccumulator = 0.f; //for updating the physics world.
	float interpFactor = 0.f; //for updating the objects for rendering.
	const reactphysics3d::decimal timeStep = 1/60.f; //for how fast the physics simulation should be.
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