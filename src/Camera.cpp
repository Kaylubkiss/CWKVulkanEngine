#include "Camera.h"
#include "Application.h"
#include <glm/gtx/rotate_vector.hpp>


Camera::Camera(const glm::vec3& eye, const glm::vec3& lookDirection, const glm::vec3& up) : mEye(eye), mLookDir(lookDirection), mUpVector(up) 
{

	this->mCapsule = { .5f, 5.f };

	glm::mat4 lookAt = Camera::LookAt();

	this->mMovementTransform.setFromOpenGL(&lookAt[0].x);

	reactphysics3d::Vector3 nPosition = this->mMovementTransform.getPosition();
	
	this->mMovementTransform.setPosition({ nPosition.x, nPosition.y - mCapsule.mHeight * .5f, nPosition.z });


}

glm::mat4 Camera::LookAt() 
{
	return glm::lookAt(mEye, mEye + mLookDir, mUpVector);
}

glm::vec3 Camera::Position() 
{
	return -mEye;
}


void Camera::MoveDown() 
{
	//TODO
}

void Camera::UpdatePosition(reactphysics3d::Vector3& velocity, const float& dt) 
{

	velocity.normalize();

	velocity *= this->constant_velocity * dt;

	reactphysics3d::Vector3 nPosition = this->mMovementTransform.getPosition() + velocity;

	this->mMovementTransform.setPosition(nPosition);

}

glm::vec3 Camera::ViewDirection() 
{
	return this->mLookDir;
}

void Camera::Update(const float& dt) 
{
	if (this->isUpdate)
	{

		this->UpdatePosition(this->accumulatedVelocity, dt);

		reactphysics3d::Vector3 currTransform = this->mMovementTransform.getPosition();
		this->mEye = glm::vec3(-currTransform.x, -(currTransform.y + .5f * mCapsule.mHeight), -currTransform.z);

		this->accumulatedVelocity = reactphysics3d::Vector3::zero();

		this->isUpdate = false;
	}

	
}

void Camera::MoveLeft() 
{
	//TODO
	isUpdate = true;
	
	reactphysics3d::Vector3 velocity = reactphysics3d::Vector3(mLookDir.x, 0, mLookDir.z).cross({ mUpVector.x, mUpVector.y, mUpVector.z });
	
	this->accumulatedVelocity += velocity;
}

void Camera::MoveRight() 
{
	//TODO
	isUpdate = true;
	
	reactphysics3d::Vector3 velocity = -reactphysics3d::Vector3(mLookDir.x, 0, mLookDir.z).cross({mUpVector.x, mUpVector.y, mUpVector.z});

	this->accumulatedVelocity += velocity;
}

void Camera::MoveForward() 
{
	//
	isUpdate = true;
	
	/*mEye += mLookDir * temp_cameraSpeed * dT;*/
	reactphysics3d::Vector3 velocity = -reactphysics3d::Vector3(mLookDir.x, mLookDir.y, mLookDir.z);
	
	this->accumulatedVelocity += velocity;
}

void Camera::MoveBack() 
{
	isUpdate = true;

	reactphysics3d::Vector3 velocity = reactphysics3d::Vector3(mLookDir.x, mLookDir.y, mLookDir.z);

	this->accumulatedVelocity += velocity;

}


void Camera::Rotate(const int& mouseX, const int& mouseY)
{
	isUpdate = true;

	/*glm::vec2 currentMousePos(mouseX, mouseY);

	if (this->controlInfo.firstLook)
	{
		this->controlInfo.mMousePos_0 = currentMousePos;
		this->controlInfo.firstLook = false;
	}

	glm::vec2 delta = currentMousePos - this->controlInfo.mMousePos_0;
	*/
	mPitch -= mouseY;
	mYaw += mouseX;
	
	if (mPitch > 89)
	{
		mPitch = 89.f;
	}
	else if (mPitch < -89.f) 
	{
		mPitch = -89.f;
	}

	mLookDir.x = glm::cos(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));
	mLookDir.y = glm::sin(glm::radians(mPitch));
	mLookDir.z = glm::sin(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));

	mLookDir /= glm::length(mLookDir);

	//this->controlInfo.mMousePos_0 = currentMousePos;

}
