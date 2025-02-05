#include "Camera.h"
#include "Application.h"
#include <glm/gtx/rotate_vector.hpp>

namespace Global 
{
	static float temp_cameraSpeed = 15.0f;

}
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

void Camera::setIsGrounded(bool set) 
{
	isGrounded = set;
}

void Camera::MoveDown() 
{
	assert(_Application != NULL);

	float dT = _Application->GetTime().DeltaTime();
	reactphysics3d::Vector3 gravity = _Application->PhysicsSystem().GetPhysicsWorld()->getGravity();
	
	reactphysics3d::Transform nTransform = mPhysicsComponent.rigidBody->getTransform();

	reactphysics3d::Vector3 position = nTransform.getPosition();

	position.y -= gravity.length() * dT;

	nTransform.setPosition(position);

	mPhysicsComponent.rigidBody->setTransform(nTransform);
}

void Camera::InitPhysics(BodyType bType)
{
	assert(_Application != NULL);

	this->mCamRayCast.SetParent(this);

	glm::vec3 colliderPosition = -mEye;
	colliderPosition.y -= mCapsule.mHeight * .5f;
	const glm::vec4& dc2Position = glm::vec4(colliderPosition, 1);
	reactphysics3d::Vector3 position(dc2Position.x, dc2Position.y, dc2Position.z);
	reactphysics3d::Quaternion orientation = Quaternion::identity();
	reactphysics3d::Transform transform(position, orientation);

	this->mPhysicsComponent.rigidBody = _Application->PhysicsSystem().AddRigidBody(transform);

	mPhysicsComponent.rigidBody->setAngularLockAxisFactor(reactphysics3d::Vector3(0,0,0));
	
	mPhysicsComponent.rigidBody->enableGravity(false);

	if (bType != BodyType::DYNAMIC)
	{
		this->mPhysicsComponent.rigidBody->setType(bType);
		this->mPhysicsComponent.bodyType = bType;
	}

	this->mPhysicsComponent.shape = _Application->PhysicsSystem().CreateCapsuleShape(mCapsule.mRadius, mCapsule.mHeight);

	if (this->mPhysicsComponent.shape != nullptr)
	{
		this->mPhysicsComponent.collider = this->mPhysicsComponent.rigidBody->addCollider(this->mPhysicsComponent.shape, Transform::identity());
	}

	reactphysics3d::Material& colliderMat = this->mPhysicsComponent.collider->getMaterial();

	colliderMat.setBounciness(0.f);
	colliderMat.setFrictionCoefficient(0.f);

	this->mPhysicsComponent.rigidBody->updateMassFromColliders();

	this->mPhysicsComponent.prevTransform = this->mPhysicsComponent.rigidBody->getTransform();
}

void Camera::UpdatePosition(reactphysics3d::Vector3& velocity) 
{

	float dT = _Application->GetTime().DeltaTime();

	velocity.normalize();

	velocity *= Global::temp_cameraSpeed * dT;

	reactphysics3d::Vector3 nPosition = this->mMovementTransform.getPosition() + velocity;

	this->mMovementTransform.setPosition(nPosition);

}

glm::vec3 Camera::ViewDirection() 
{
	return this->mLookDir;
}

void Camera::Update(float interpFactor) 
{
	if (this->isUpdate)
	{

		this->UpdatePosition(this->accumulatedVelocity);

		reactphysics3d::Vector3 currTransform = this->mMovementTransform.getPosition();
		this->mEye = glm::vec3(-currTransform.x, -(currTransform.y + .5f * mCapsule.mHeight), -currTransform.z);

	/*	this->mPhysicsComponent.rigidBody->setTransform(this->mMovementTransform);*/
		
	/*	this->mMovementTransform = reactphysics3d::Transform::identity();*/
		this->accumulatedVelocity = reactphysics3d::Vector3::zero();

		_Application->UpdateUniformViewMatrix();

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
	
	//mEye += glm::cross(mLookDir, mUpVector) * temp_cameraSpeed * dT;
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
	//TODO
	isUpdate = true;
	//mEye -= mLookDir * temp_cameraSpeed * dT;

	reactphysics3d::Vector3 velocity = reactphysics3d::Vector3(mLookDir.x, mLookDir.y, mLookDir.z);

	this->accumulatedVelocity += velocity;

}

bool Camera::isUpdated() 
{
	return this->isUpdate;
}

void Camera::Rotate(const int& mouseX, const int& mouseY)
{
	isUpdate = true;

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

	mLookDir.x = glm::cos(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));
	mLookDir.y = glm::sin(glm::radians(mPitch));
	mLookDir.z = glm::sin(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));

	mLookDir /= glm::length(mLookDir);

	mOldMousePos = currentMousePos;

}
