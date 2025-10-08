#include "Object.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include "vkGlobal.h"

Object::Object(const VkPhysicalDevice p_device, const VkDevice l_device, 
                const char* fileName, bool willDebugDraw)
{
    (void)(willDebugDraw);
    
    bool result = this->mMesh.LoadOBJMesh((OBJECT_PATH + std::string(fileName)).c_str());
    
    if (!result) 
    {
        throw std::runtime_error("could not load requested object " + std::string(fileName));
    }
    
    /*int numVertices = static_cast<int>(this->mMesh.data.vertices.size());
    
    glm::vec3 min_points(0.f);
    glm::vec3 max_points(0.f);
    
    for (unsigned i = 0; i < this->mMesh.data.vertices.size(); ++i)
    {
    
        min_points.x = std::min(min_points.x, this->mMesh.data.vertices[i].pos.x);
        min_points.y = std::min(min_points.y, this->mMesh.data.vertices[i].pos.y);
        min_points.z = std::min(min_points.z, this->mMesh.data.vertices[i].pos.z);
    
        max_points.x = std::max(max_points.x, this->mMesh.data.vertices[i].pos.x);
        max_points.y = std::max(max_points.y, this->mMesh.data.vertices[i].pos.y);
        max_points.z = std::max(max_points.z, this->mMesh.data.vertices[i].pos.z);
    
        this->mMesh.data.vertices[i].nrm = glm::vec3(0, 0, 0.f);
    
        this->mMesh.center += this->mMesh.data.vertices[i].pos;
    }
    
    this->mMesh.center /= this->mMesh.data.vertices.size();
    
    float unitScale = std::max({ glm::length(max_points.x - min_points.x), glm::length(max_points.y - min_points.y), glm::length(max_points.z - min_points.z) });
    
    max_points = { -std::numeric_limits<float>::min(),  -std::numeric_limits<float>::min() , -std::numeric_limits<float>::min() };
    min_points = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    
    for (size_t i = 0; i < this->mMesh.data.vertices.size(); ++i)
    {
        this->mMesh.data.vertices[i].pos = (this->mMesh.data.vertices[i].pos - this->mMesh.center) / unitScale;
    
        max_points.x = std::max(max_points.x, this->mMesh.data.vertices[i].pos.x);
        max_points.y = std::max(max_points.y, this->mMesh.data.vertices[i].pos.y);
        max_points.z = std::max(max_points.z, this->mMesh.data.vertices[i].pos.z);
    
        min_points.x = std::min(min_points.x, this->mMesh.data.vertices[i].pos.x);
        min_points.y = std::min(min_points.y, this->mMesh.data.vertices[i].pos.y);
        min_points.z = std::min(min_points.z, this->mMesh.data.vertices[i].pos.z);
    }
    
    this->mMesh.maxLocalPoints = max_points;
    this->mMesh.minLocalPoints = min_points;
    
    Object::ComputeVertexNormals();
    
    size_t sizeOfVertexBuffer = sizeof(std::vector<Vertex>) + (sizeof(Vertex) * this->mMesh.data.vertices.size());
    this->mMesh.buffer.vertex = vk::Buffer(p_device, l_device, sizeOfVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->mMesh.data.vertices.data());
    
    size_t sizeOfIndexBuffer = sizeof(std::vector<uint16_t>) + (sizeof(uint16_t) * this->mMesh.data.indices.size());
    this->mMesh.buffer.index = vk::Buffer(p_device, l_device, sizeOfIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->mMesh.data.indices.data());
    
    std::cout << std::endl;
    std::cout << "loaded in " + std::string(fileName) << std::endl;
    std::cout << numVertices << " vertices loaded in." << std::endl << std::endl;*/
}

void Object::SetDebugDraw(bool option) 
{
    this->debugDraw = option;
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
    if (debugDraw == false)
    {
        if (pipelineLayout != VK_NULL_HANDLE)
        {
            vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), (void*)(&this->modelTransform));
        }

        if (this->textureDescriptorSet != VK_NULL_HANDLE) 
        {
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &this->textureDescriptorSet, 0, nullptr);
        }

        VkDeviceSize offsets[1] = { 0 };

        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &mMesh.buffer.vertex.handle, offsets);

        vkCmdBindIndexBuffer(cmdBuffer, mMesh.buffer.index.handle, 0, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(this->mMesh.data.indices.size()), 1, 0, 0, 0);

    }
    else {

        //..draw debug shape!
    }
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

void Object::AddTextureDescriptorSet(VkDescriptorSet textureDscSet)
{
    textureDescriptorSet = textureDscSet;
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



