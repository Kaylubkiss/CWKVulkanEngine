#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <reactphysics3d/reactphysics3d.h>
using namespace reactphysics3d;

struct Capsule 
{
	float mRadius;
	float mHeight;
};

class CamRayCastCallBack : public RaycastCallback {};

struct CamControllerInfo 
{
	bool firstLook = true;
	glm::vec2 mMousePos_0 = { 0,0 };
};

class Camera 
{
	protected:
		class CamRayCastCallback : public RaycastCallback {

	private:
		Camera* mCamera = nullptr;

	public:

		void SetParent(Camera* camera)
		{
			mCamera = camera;
		}

		virtual decimal notifyRaycastHit(const RaycastInfo& info) {

			assert(mCamera != NULL);

			// Display the world hit point coordinates
		/*	std::cout << "Hit point : " <<
				info.worldPoint.x <<
				info.worldPoint.y <<
				info.worldPoint.z <<
				std::endl;*/

			glm::vec3 cmPosition = mCamera->Position();
			reactphysics3d::Vector3 cam_position = mCamera->mPhysicsComponent.rigidBody->getTransform().getPosition();

			cam_position.y -= mCamera->mCapsule.mHeight * .5f;
			
			if ((cam_position - info.worldPoint).length() <= 0.1f)
			{
				mCamera->mPhysicsComponent.rayCastHit = true;
				std::cout << "grounded.\n";
			}
			// Return a fraction of 1.0 to gather all hits
			return decimal(0.0);
		}
	};
	
		glm::vec3 mEye;
		glm::vec3 mUpVector;
		glm::vec3 mLookDir = glm::vec3(0, 0, -1);
	
		float mPitch = 0.f;
		float mYaw = 0.f;
	
		bool isUpdate = false;
		
		
		float constant_velocity = 15.f;


	
	
		Capsule mCapsule;
		PhysicsComponent mPhysicsComponent; 
		CamRayCastCallback mCamRayCast;
		reactphysics3d::Transform mMovementTransform;
		reactphysics3d::Vector3 accumulatedVelocity = Vector3::zero();
	
		void UpdatePosition(reactphysics3d::Vector3& velocity, const float& dt);
	
	private:
		CamControllerInfo controlInfo;
	
	public:
		Camera() : mEye(0.f), mUpVector(0.f), mCapsule()
		{
		}
	
		Camera(const glm::vec3& eye, const glm::vec3& lookDirection, const	glm::vec3& up);
		
		void MoveLeft();
		void MoveRight();
		void MoveBack();
		void MoveForward();
		void MoveDown();
	
		void Update(const float& dt);
		void Rotate(const int& mouseX, const int& mouseY);
		
		//getter functions.
		glm::mat4 LookAt(); 
		glm::vec3 Position();
		glm::vec3 ViewDirection();
	
	
		CamControllerInfo& ControllerInfo();
};


