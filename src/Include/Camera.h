#pragma once
#include "vkGlobal.h"
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

class Camera 
{

	public:
		bool isUpdate = false;

	private:
		
		glm::vec3 mEye;
		glm::vec3 mUpVector;
		glm::vec3 mLookDir = glm::vec3(0, 0, -1);
		
		float mPitch = 0.f;
		float mYaw = 0.f;
		
		float constant_velocity = 15.f;

		reactphysics3d::Transform mMovementTransform;
		reactphysics3d::Vector3 accumulatedVelocity = Vector3::zero();
	
		void UpdatePosition(reactphysics3d::Vector3& velocity, const float& dt);
	
	public:
		Camera() : mEye(0.f), mUpVector(0.f) {}
	
		Camera(const glm::vec3& eye, const glm::vec3& lookDirection, const	glm::vec3& up);

		void Update(const float& dt);
		void MoveLeft();
		void MoveRight();
		void MoveBack();
		void MoveForward();
		void MoveDown();
		void Rotate(const int& mouseX, const int& mouseY);
		
		//getter functions.
		glm::mat4 LookAt(); 
		glm::vec3 Position();
		glm::vec3 ViewDirection();
	
};


