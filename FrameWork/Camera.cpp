#include "Camera.h"
#include "Application.h"
#include <glm/gtx/rotate_vector.hpp>

static float temp_cameraSpeed = 30.0f;

Camera::Camera(const glm::vec3& eye, const glm::vec3& lookDirection, const glm::vec3& up) : mEye(eye), mLookDir(lookDirection), mUpVector(up) 
{


}

const glm::mat4& Camera::LookAt() 
{
	return glm::lookAt(mEye, mEye + mLookDir, mUpVector);
}

const glm::vec3& Camera::Position() 
{
	return -mEye;
}

void Camera::MoveLeft() 
{
	//TODO
	float dT = _Application->GetDeltaTime();
	mEye -= glm::cross(mLookDir, mUpVector) * temp_cameraSpeed * dT;
}

void Camera::MoveRight() 
{
	//TODO
	float dT = _Application->GetDeltaTime();
	mEye += glm::cross(mLookDir, mUpVector) * temp_cameraSpeed * dT;
}

void Camera::MoveForward() 
{
	//TODO
	float dT = _Application->GetDeltaTime();
	mEye += mLookDir * temp_cameraSpeed * dT;
}

void Camera::MoveBack() 
{
	//TODO
	float dT = _Application->GetDeltaTime();
	mEye -= mLookDir * temp_cameraSpeed * dT;
}

void Camera::Rotate(const int& mouseX, const int& mouseY)
{
	glm::vec2 currentMousePos(mouseX, mouseY);


	static bool firstLook = true;
	if (firstLook) 
	{
		mOldMousePos = currentMousePos;
		firstLook = false;
	}

	glm::vec2 delta = currentMousePos - mOldMousePos;
	
	mPitch -= delta.y;
	mYaw += delta.x;
	
	if (mPitch > 89)
	{

		mPitch = 89.f;
	}
	else if (mPitch < -89.f) 
	{
		mPitch = -89.f;
	}


	//std::cout << "angular displacement (Y): " << mBeta << std::endl;
	//std::cout << "angular displacement (X): " << mAlpha << std::endl;
	//std::cout << delta.x << std::endl;
	//std::cout << delta.y << std::endl;

	//TODO
	/*mLookDir = glm::rotate(mLookDir, delta.y, glm::vec3(1, 0, 0));
	mLookDir = glm::rotate(mLookDir, delta.x, mUpVector);*/

	mLookDir.x = glm::cos(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));
	mLookDir.y = glm::sin(glm::radians(mPitch));
	mLookDir.z = glm::sin(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));

	mLookDir /= glm::length(mLookDir);

	mOldMousePos = currentMousePos;

}
