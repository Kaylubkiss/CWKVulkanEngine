#pragma once
#include <reactphysics3d/reactphysics3d.h>

#include "Physics.h"
using namespace reactphysics3d;

class Camera final
{
public:
	Camera() : mEye(0.f), mUpVector(0.f) {}

	Camera( const glm::vec3& eye, const glm::vec3& lookDirection, const	glm::vec3& up );

	bool IsUpdated() const;

	void Update( float dt );
	void MoveLeft();
	void MoveRight();
	void MoveBack();
	void MoveForward();
	void MoveDown();
	void Rotate( const int& mouseX, const int& mouseY );

	//getter functions.
	glm::mat4 LookAt();
	glm::vec3 Position();
	glm::vec3 ViewDirection();
private:
	bool isUpdate = false;
	bool isInterpolating = false;

	glm::vec3 mEye;
	glm::vec3 mUpVector;
	glm::vec3 mLookDir = glm::vec3(0, 0, -1);

	float mPitch = 0.f;
	float mYaw = 0.f;

	float constant_velocity = 0.3f;

	reactphysics3d::Transform mMovementTransform;
	reactphysics3d::Vector3 accumulatedVelocity = Vector3::zero();

	void UpdatePosition( reactphysics3d::Vector3& velocity, float dt );
};


