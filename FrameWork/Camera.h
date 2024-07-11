#pragma once
#include "Common.h"

class Camera 
{
	glm::vec3 mEye;
	/*glm::vec3 mCenter;*/
	glm::vec3 mUpVector;
	glm::vec3 mLookDir = glm::vec3(0, 0, -1);
	glm::vec2 mOldMousePos;
	float mPitch = 0.f;
	float mYaw = 0.f;
	bool isUpdate = false;
	//PhysicsComponent mPhysics;

public:
	Camera() : mEye(0.f), mUpVector(0.f) {}
	Camera(const glm::vec3& eye, const glm::vec3& lookDirection, const glm::vec3& up);
	
	void MoveLeft();
	void MoveRight();
	void MoveBack();
	void MoveForward();

	void Rotate(const int& mouseX, const int& mouseY);

	//void SetOldMousePos(const glm::vec2& pos);
	//used for function parameters, this computes the lookat matrix.
	const glm::mat4& LookAt(); 
	//returns world position.
	const glm::vec3& Position();

	bool isUpdated();

};