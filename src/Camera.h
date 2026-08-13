#ifndef CAMERA_HPP
#define CAMERA_HPP

class Camera final
{
public:
	Camera() = default;
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
	glm::mat4 LookAt() const;
	glm::vec3 Position() const;
	glm::vec3 ViewDirection() const;

	float GetFOV() const;
private:
	bool isUpdate = false;

	glm::vec3 mEye = glm::vec3(0);
	glm::vec3 mUpVector = glm::vec3(0);
	glm::vec3 mLookDir = glm::vec3(0, 0, -1);

	float mFOV = 45.0f;
	float mPitch = 0.f;
	float mYaw = 0.f;

	float constant_velocity = 5.f;

	reactphysics3d::Transform mMovementTransform;
	reactphysics3d::Vector3 accumulatedVelocity = reactphysics3d::Vector3::zero();

	void UpdatePosition( reactphysics3d::Vector3& velocity, float dt );
};

#endif


