#include "Camera.h"

Camera::Camera(const glm::vec3& eye, const glm::vec3& lookDirection, const glm::vec3& up) :
	mEye(eye), mLookDir(lookDirection), mUpVector(up)
{

	glm::mat4 lookAt = Camera::LookAt();

	this->mMovementTransform.setFromOpenGL(&lookAt[0].x);

	reactphysics3d::Vector3 nPosition = this->mMovementTransform.getPosition();
	
	this->mMovementTransform.setPosition({ nPosition.x, nPosition.y, nPosition.z });

}

glm::mat4 Camera::LookAt() 
{
	return glm::lookAt(mEye, mEye + mLookDir, mUpVector);
}

glm::vec3 Camera::Position() 
{
	return mEye;
}

void Camera::MoveDown() 
{
	//TODO
}

void Camera::UpdatePosition( reactphysics3d::Vector3& direction, float dt )
{
	if (direction.isZero() == false)
	{
		direction.normalize();

		mMovementTransform.setPosition(mMovementTransform.getPosition() + direction * constant_velocity * dt);

		reactphysics3d::Vector3 currTransform = mMovementTransform.getPosition();
		this->mEye = glm::vec3(-currTransform.x, -currTransform.y, -currTransform.z);
	}
}

glm::vec3 Camera::ViewDirection() 
{
	return this->mLookDir;
}

bool Camera::IsUpdated() const
{
	return isUpdate;
}

void Camera::Update( float dt )
{
	if (isUpdate)
	{
		Camera::UpdatePosition(accumulatedVelocity, dt);

		accumulatedVelocity = reactphysics3d::Vector3::zero();

		isUpdate = false;
	}
}

void Camera::MoveLeft() 
{
	isUpdate = true;
	
	reactphysics3d::Vector3 velocity =
		reactphysics3d::Vector3(mLookDir.x,0, mLookDir.z).cross({ mUpVector.x, mUpVector.y, mUpVector.z });
	
	this->accumulatedVelocity += velocity;
}

void Camera::MoveRight() 
{
	isUpdate = true;
	
	reactphysics3d::Vector3 velocity =
		-reactphysics3d::Vector3(mLookDir.x, 0, mLookDir.z).cross({mUpVector.x, mUpVector.y, mUpVector.z});

	this->accumulatedVelocity += velocity;
}

void Camera::MoveForward() 
{
	isUpdate = true;
	
	reactphysics3d::Vector3 velocity =
		-reactphysics3d::Vector3(mLookDir.x, mLookDir.y, mLookDir.z);
	
	this->accumulatedVelocity += velocity;
}

void Camera::MoveBack() 
{
	isUpdate = true;

	reactphysics3d::Vector3 velocity =
		reactphysics3d::Vector3(mLookDir.x, mLookDir.y, mLookDir.z);

	this->accumulatedVelocity += velocity;

}

void Camera::Rotate(const int& mouseX, const int& mouseY)
{
	isUpdate = true;

	mPitch -= static_cast<float>(mouseY);
	mYaw += static_cast<float>(mouseX);
	
	if (mPitch >= 89.f)
	{
		mPitch = 89.f;
	}
	else if (mPitch <= -89.f)
	{
		mPitch = -89.f;
	}

	mLookDir.x = glm::cos(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));
	mLookDir.y = glm::sin(glm::radians(mPitch));
	mLookDir.z = glm::sin(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));

	mLookDir = glm::normalize(mLookDir);
}
