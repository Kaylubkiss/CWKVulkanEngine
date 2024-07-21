#include "Camera.h"
#include "Application.h"
#include <glm/gtx/rotate_vector.hpp>

static float temp_cameraSpeed = 20.0f;

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

void Camera::InitPhysics(BodyType bType)
{
	assert(_Application != NULL);

	const glm::vec4& dc2Position = glm::vec4(mEye, 1);
	reactphysics3d::Vector3 position(dc2Position.x, dc2Position.y, dc2Position.z);
	reactphysics3d::Quaternion orientation = Quaternion::identity();
	reactphysics3d::Transform transform(position, orientation);

	this->mPhysicsComponent.rigidBody = _Application->PhysicsSystem().AddRigidBody(transform);

	mPhysicsComponent.rigidBody->setAngularLockAxisFactor(reactphysics3d::Vector3(0,0,0));
	mPhysicsComponent.rigidBody->setLinearDamping(1.f);
	mPhysicsComponent.rigidBody->setMass(100.f);

	if (bType != BodyType::DYNAMIC)
	{
		this->mPhysicsComponent.rigidBody->setType(bType);
		this->mPhysicsComponent.bodyType = bType;
	}
	this->mPhysicsComponent.shape = _Application->PhysicsSystem().CreateCapsuleShape(1.f, mEye.length());

	if (this->mPhysicsComponent.shape != nullptr)
	{
		this->mPhysicsComponent.collider = this->mPhysicsComponent.rigidBody->addCollider(this->mPhysicsComponent.shape, Transform::identity());
	}

	reactphysics3d::Material& colliderMat = this->mPhysicsComponent.collider->getMaterial();

	colliderMat.setBounciness(0.f);
	colliderMat.setFrictionCoefficient(0.f);

	/*_Application->PhysicsSystem().GetPhysicsWorld()->getGravity*/
	this->mPhysicsComponent.prevTransform = this->mPhysicsComponent.rigidBody->getTransform();
}

void Camera::MoveLeft() 
{
	//TODO
	isUpdate = true;
	float dT = _Application->GetTime().DeltaTime();
	reactphysics3d::Vector3 velocity = -reactphysics3d::Vector3(mLookDir.x, 0, mLookDir.z).cross({ mUpVector.x, mUpVector.y, mUpVector.z }) * temp_cameraSpeed * dT;

	mPhysicsComponent.rigidBody->setLinearVelocity(velocity + this->mPhysicsComponent.rigidBody->getLinearVelocity());
}

void Camera::Update(float interpFactor) 
{
	//only update the y if we aren't colliding with something below us. raycast from feet to check for ground.
	isUpdate = true;
	if (this->mPhysicsComponent.bodyType != BodyType::STATIC)
	{
		Transform uninterpolatedTransform = this->mPhysicsComponent.rigidBody->getTransform();

		this->mPhysicsComponent.currTransform = Transform::interpolateTransforms(this->mPhysicsComponent.prevTransform, uninterpolatedTransform, interpFactor);

		this->mPhysicsComponent.prevTransform = this->mPhysicsComponent.currTransform;

		reactphysics3d::Vector3 nTransform = this->mPhysicsComponent.currTransform.getPosition();

		
		this->mEye = glm::vec3(nTransform.x, nTransform.y, nTransform.z);
	}

	/*this->mPhysicsComponent.rigidBody->get*/
}

void Camera::MoveRight() 
{
	//TODO
	isUpdate = true;
	float dT = _Application->GetTime().DeltaTime();
	//mEye += glm::cross(mLookDir, mUpVector) * temp_cameraSpeed * dT;
	reactphysics3d::Vector3 velocity = reactphysics3d::Vector3(mLookDir.x, 0, mLookDir.z).cross({mUpVector.x, mUpVector.y, mUpVector.z}) * temp_cameraSpeed * dT;

	mPhysicsComponent.rigidBody->setLinearVelocity(velocity + this->mPhysicsComponent.rigidBody->getLinearVelocity());
}

void Camera::MoveForward() 
{
	//
	isUpdate = true;
	float dT = _Application->GetTime().DeltaTime();
	/*mEye += mLookDir * temp_cameraSpeed * dT;*/
	reactphysics3d::Vector3 velocity = reactphysics3d::Vector3(mLookDir.x, 0, mLookDir.z) * temp_cameraSpeed * dT;
	
	mPhysicsComponent.rigidBody->setLinearVelocity(velocity + this->mPhysicsComponent.rigidBody->getLinearVelocity());
}

void Camera::MoveBack() 
{
	//TODO
	isUpdate = true;
	float dT = _Application->GetTime().DeltaTime();
	//mEye -= mLookDir * temp_cameraSpeed * dT;

	reactphysics3d::Vector3 velocity = -reactphysics3d::Vector3(mLookDir.x, 0, mLookDir.z) * temp_cameraSpeed * dT;
	/*mEye += glm::vec3(0, 0, 1) * temp_cameraSpeed * dT;*/
	/*mPhysicsComponent.rigidBody->applyLocalForceAtWorldPosition(reactphysics3d::Vector3(0, 0, 1),reactphysics3d::Vector3(-mEye.x, -mEye.y, -mEye.z));*/
	/*mPhysicsComponent.rigidBody->setLinearVelocity(velocity);
	mPhysicsComponent.rigidBody->setLinearDamping(velocity.length() * 2.f);*/

	mPhysicsComponent.rigidBody->setLinearVelocity(velocity + this->mPhysicsComponent.rigidBody->getLinearVelocity());
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
