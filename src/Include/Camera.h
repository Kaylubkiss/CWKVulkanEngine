#pragma once
#include "Common.h"
#include "Physics.h"


struct Capsule 
{
	float mRadius;
	float mHeight;
};

class CamRayCastCallBack : public RaycastCallback {};

class Camera 
{
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
	/*glm::vec3 mCenter;*/
	glm::vec3 mUpVector;
	glm::vec3 mLookDir = glm::vec3(0, 0, -1);
	glm::vec2 mOldMousePos = glm::vec3(0.f);
	float mPitch = 0.f;
	float mYaw = 0.f;
	bool isUpdate = false;
	bool flyCam = false;
	bool isGrounded = false;


	Capsule mCapsule;
	PhysicsComponent mPhysicsComponent; 
	CamRayCastCallback mCamRayCast;
	reactphysics3d::Transform mMovementTransform;
	reactphysics3d::Vector3 accumulatedVelocity = Vector3::zero();

	void UpdatePosition(reactphysics3d::Vector3& velocity);

public:
	Camera() : mEye(0.f), mUpVector(0.f), mCapsule()
	{
	}

	Camera(const glm::vec3& eye, const glm::vec3& lookDirection, const glm::vec3& up);
	
	void MoveLeft();
	void MoveRight();
	void MoveBack();
	void MoveForward();
	void MoveDown();

	void Update(float interpFactor);
	void InitPhysics(BodyType bType = BodyType::DYNAMIC);
	void Rotate(const int& mouseX, const int& mouseY);

	//void SetOldMousePos(const glm::vec2& pos);
	//used for function parameters, this computes the lookat matrix.
	glm::mat4 LookAt(); 
	//returns world position.
	glm::vec3 Position();

	glm::vec3 ViewDirection();

	bool isUpdated();

	void setIsGrounded(bool set);

};


