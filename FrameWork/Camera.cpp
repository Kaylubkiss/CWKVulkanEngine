#include "Camera.h"


Camera::Camera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up) :mEye(eye), mCenter(center), mUp(up) {}

const glm::mat4& Camera::LookAt() 
{
	return glm::lookAt(-mEye, mCenter, mUp);
}

const glm::vec3& Camera::Position() 
{
	return mEye;
}
