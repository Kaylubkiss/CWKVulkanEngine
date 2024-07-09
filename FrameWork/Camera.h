#pragma once
#include "Common.h"

class Camera 
{
	glm::vec3 mEye;
	glm::vec3 mCenter;
	glm::vec3 mUp;
	PhysicsComponent mPhysics;

public:
	Camera() : mEye(0.f), mCenter(0.f), mUp(0.f) {}
	Camera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);
	//used for function parameters, this computes the lookat matrix.
	const glm::mat4& LookAt(); 
	//returns world position.
	const glm::vec3& Position();

};