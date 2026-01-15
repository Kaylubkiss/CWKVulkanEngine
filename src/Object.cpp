#include "Object.h"

Object::Object(const VkPhysicalDevice p_device, const VkDevice l_device, 
                const char* fileName, bool willDebugDraw)
{
    (void)(willDebugDraw);
    
    bool result = this->mMesh.LoadOBJMesh((OBJECT_PATH + std::string(fileName)).c_str());
    
    if (!result) 
    {
        throw std::runtime_error("could not load requested object " + std::string(fileName));
    }
}


Object::Object(vk::Device* device) 
{
    assert(device);

    devicePtr = device;
}

void Object::InitPhysics(PhysicsSystem& appPhysics)
{

    glm::vec4 worldMinPoints = modelTransform * glm::vec4(mMesh.minLocalPoints, 1);
    glm::vec4 worldMaxPoints = modelTransform * glm::vec4(mMesh.maxLocalPoints, 1);

    const glm::vec4 dc2Position = .5f * (worldMinPoints + worldMaxPoints);
    reactphysics3d::Vector3 position(dc2Position.x, dc2Position.y, dc2Position.z);
    reactphysics3d::Quaternion orientation = Quaternion::identity();
    reactphysics3d::Transform transform(position, orientation);

   
    this->mPhysicsComponent.rigidBody = appPhysics.AddRigidBody(transform);

    //setting the body type of the rigidbody
    if (this->mPhysicsComponent.bodyType != BodyType::DYNAMIC)
    {
        this->mPhysicsComponent.rigidBody->setType(this->mPhysicsComponent.bodyType);
    }
    
    //creating a collision shape
    if (this->mPhysicsComponent.colliderType == PhysicsComponent::ColliderType::CUBE) 
    {
        glm::vec3 worldHalfExtent = glm::vec3((worldMaxPoints - worldMinPoints) * .5f);
        this->mPhysicsComponent.shape = appPhysics.CreateBoxShape({ std::abs(worldHalfExtent.x), std::abs(worldHalfExtent.y), std::abs(worldHalfExtent.z) });
    }
    else if (this->mPhysicsComponent.colliderType == PhysicsComponent::ColliderType::PLANE) 
    {
        glm::vec3 worldHalfExtent2D = glm::vec3((worldMaxPoints - worldMinPoints) * .5f);
        this->mPhysicsComponent.shape = appPhysics.CreatePlaneShape({ std::abs(worldHalfExtent2D.x), std::abs(worldHalfExtent2D.z) });
    }

    //the collider transform is relative to the rigidbody origin.
    if (this->mPhysicsComponent.shape != nullptr)
    {
        this->mPhysicsComponent.collider = this->mPhysicsComponent.rigidBody->addCollider(this->mPhysicsComponent.shape, Transform::identity());
    }

    this->mPhysicsComponent.prevTransform = this->mPhysicsComponent.rigidBody->getTransform();
}

uint32_t Object::TextureIndex() 
{
    return this->textureIndex;
}

void Object::Destroy(const VkDevice l_device) 
{
    this->mMesh.Destroy(l_device);
}

void Object::Update(const float& interpFactor)
{

    if (this->mPhysicsComponent.bodyType != BodyType::STATIC)
    {
        Transform uninterpolatedTransform = this->mPhysicsComponent.rigidBody->getTransform();

        this->mPhysicsComponent.currTransform = Transform::interpolateTransforms(this->mPhysicsComponent.prevTransform, uninterpolatedTransform, interpFactor);

        this->mPhysicsComponent.prevTransform = this->mPhysicsComponent.currTransform;

        float matrix[16];

        this->mPhysicsComponent.currTransform.getOpenGLMatrix(matrix);

       
        //this makes this stuff too dang easy.
        glm::mat4 nModel = glm::mat4(matrix[0], matrix[1], matrix[2], matrix[3], 
                                     matrix[4], matrix[5], matrix[6], matrix[7],
                                     matrix[8], matrix[9], matrix[10], matrix[11],
                                     matrix[12], matrix[13], matrix[14], matrix[15]);

        this->modelTransform = nModel;
    }


}

void Object::Draw(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout)
{  
    if (pipelineLayout != VK_NULL_HANDLE)
    {
        vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), (void*)(&this->modelTransform));
    }

    VkDeviceSize offsets[1] = { 0 };
    
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &mMesh.buffer.vertex.handle, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, mMesh.buffer.index.handle, 0, VK_INDEX_TYPE_UINT16);    
    vkCmdDrawIndexed(cmdBuffer, mMesh.buffer.indexCount, 1, 0, 0, 0);
}

void Object::UpdatePhysicsComponent(const PhysicsComponent* physComp)
{
    if (physComp != nullptr) 
    {
        mPhysicsComponent = *physComp;

        PhysicsSystem& appPhysics = _Application->GetPhysics();

        Object::InitPhysics(appPhysics);
    }
}

void Object::UpdateModelTransform(const glm::mat4* modelTransform) 
{
    if (modelTransform) 
    {
        this->modelTransform = *modelTransform;
    }
}

void Object::UpdateMesh(const Mesh* mesh) 
{
    if (mesh) 
    {
        this->mMesh = *mesh;
    }
}

void Object::UpdateTextureDescriptorOffset(uint32_t offset)
{
    this->textureIndex = offset;
}


