#pragma once
#include "Common.h"
#include "Physics.h"

struct AABB 
{
	glm::vec3 mMin = glm::vec3(0.f);
	glm::vec3 mMax = glm::vec3(0.f);
};

struct Capsule 
{
	float mRadius;
	float mHeight;
};

class Camera 
{
	glm::vec3 mEye;
	/*glm::vec3 mCenter;*/
	glm::vec3 mUpVector;
	glm::vec3 mLookDir = glm::vec3(0, 0, -1);
	glm::vec2 mOldMousePos = glm::vec3(0.f);
	float mPitch = 0.f;
	float mYaw = 0.f;
	bool isUpdate = false;
	bool flyCam = false;


	Capsule mCapsule = { 0.5f, 1.f };
	PhysicsComponent mPhysicsComponent;

public:
	Camera() : mEye(0.f), mUpVector(0.f) 
	{
		mCapsule.mRadius = .5f;
		mCapsule.mHeight = 1.f;	
	}

	Camera(const glm::vec3& eye, const glm::vec3& lookDirection, const glm::vec3& up);
	
	void MoveLeft();
	void MoveRight();
	void MoveBack();
	void MoveForward();

	void Update(float interpFactor);
	void InitPhysics(BodyType bType = BodyType::DYNAMIC);
	void Rotate(const int& mouseX, const int& mouseY);

	//void SetOldMousePos(const glm::vec2& pos);
	//used for function parameters, this computes the lookat matrix.
	const glm::mat4& LookAt(); 
	//returns world position.
	const glm::vec3& Position();

	bool isUpdated();

};