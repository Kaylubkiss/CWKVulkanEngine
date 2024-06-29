#include "Object.h"
#include "Application.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Object::Object(const char* fileName, const char* textureName, VkPipelineLayout* pipelineLayout)
{
    assert(_Application != NULL);

    LoadMeshOBJ(fileName, *this);

    this->numVertices = static_cast<int>(this->vertexBufferData.size());

    glm::vec3 min_points(0.f);
    glm::vec3 max_points(0.f);

    for (unsigned i = 0; i < this->vertexBufferData.size(); ++i)
    {

        min_points.x = std::min(min_points.x, vertexBufferData[i].pos.x);
        min_points.y = std::min(min_points.y, vertexBufferData[i].pos.y);
        min_points.z = std::min(min_points.z, vertexBufferData[i].pos.z);

        max_points.x = std::max(max_points.x, vertexBufferData[i].pos.x);
        max_points.y = std::max(max_points.y, vertexBufferData[i].pos.y);
        max_points.z = std::max(max_points.z, vertexBufferData[i].pos.z);

        this->vertexBufferData[i].nrm = glm::vec3(2.f, .5f, 0.f);

        mCenter += vertexBufferData[i].pos;
    }

    mCenter /= this->vertexBufferData.size();
   /* mCenter = (max_points + min_points) * 0.5f;*/

    float unitScale = std::max({ glm::length(max_points.x - min_points.x), glm::length(max_points.y - min_points.y), glm::length(max_points.z - min_points.z) });

    max_points = { -std::numeric_limits<float>::min(),  -std::numeric_limits<float>::min() , -std::numeric_limits<float>::min() };

    for (size_t i = 0; i < this->vertexBufferData.size(); ++i)
    {
        this->vertexBufferData[i].pos = (this->vertexBufferData[i].pos - mCenter) / unitScale;
        
        max_points.x = std::max(max_points.x, vertexBufferData[i].pos.x);
        max_points.y = std::max(max_points.y, vertexBufferData[i].pos.y);
        max_points.z = std::max(max_points.z, vertexBufferData[i].pos.z);
    }

    glm::vec3 halfExtent = ((max_points - mCenter));

    halfExtent.x = std::abs(halfExtent.x);
    halfExtent.y = std::abs(halfExtent.y);
    halfExtent.z = std::abs(halfExtent.z);

    mHalfExtent = reactphysics3d::Vector3(halfExtent.x, halfExtent.y, halfExtent.z);


    mMaxLocalPoints = max_points;
    //mHalfExtent.normalize();


    size_t sizeOfVertexBuffer = sizeof(std::vector<Vertex>) + (sizeof(Vertex) * this->vertexBufferData.size());
    this->vertexBuffer = Buffer(sizeOfVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->vertexBufferData.data());
    size_t sizeOfIndexBuffer = sizeof(std::vector<uint16_t>) + (sizeof(uint16_t) * this->indexBufferData.size());
    this->indexBuffer = Buffer(sizeOfIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->indexBufferData.data());

    std::cout << std::endl;
    std::cout << "loaded in " + std::string(fileName) << std::endl;
    std::cout << this->numVertices << " vertices loaded in." << std::endl << std::endl;

    if (textureName != nullptr) 
    {
        this->textureIndex = _Application->GetTexture(textureName);
    }

    if (pipelineLayout != nullptr) 
    {
        this->mPipelineLayout = _Application->GetPipelineLayout();
    }
}


void Object::InitPhysics(ColliderType cType, BodyType bType)
{
    assert(_Application != NULL);

    const glm::vec4& dc2Position = this->mModelTransform[3] + glm::vec4(this->mCenter, 1);
    reactphysics3d::Vector3 position(dc2Position.x, dc2Position.y, dc2Position.z);
    reactphysics3d::Quaternion orientation = Quaternion::identity();
    reactphysics3d::Transform transform(position, orientation);

   
    this->mPhysics.rigidBody = _Application->GetPhysicsWorld()->createRigidBody(transform);

    if (bType != BodyType::DYNAMIC)
    {
       this->mPhysics.rigidBody->setType(bType);
    }

    switch (cType) 
    {
        case ColliderType::CUBE:
            glm::vec3 worldHalfExtent =  glm::vec3(mModelTransform * glm::vec4(mMaxLocalPoints, 1)) - glm::vec3(dc2Position);
            this->mPhysics.shape = _Application->GetPhysicsCommon().createBoxShape({ worldHalfExtent.x, worldHalfExtent.y, worldHalfExtent.z });
    }


    //the collider transform is relative to the rigidbody origin.

    if (this->mPhysics.shape != nullptr)
    {
        this->mPhysics.collider = this->mPhysics.rigidBody->addCollider(this->mPhysics.shape, Transform::identity());
    }
    
    this->mPhysics.prevTransform = this->mPhysics.rigidBody->getTransform();
}

void Object::DestroyResources()
{
    vkFreeMemory(_Application->LogicalDevice(), this->vertexBuffer.memory, nullptr);
    vkDestroyBuffer(_Application->LogicalDevice(), this->vertexBuffer.handle, nullptr);
    
    vkFreeMemory(_Application->LogicalDevice(), this->indexBuffer.memory, nullptr);
    vkDestroyBuffer(_Application->LogicalDevice(), this->indexBuffer.handle, nullptr);
}

void Object::Update(const float& interpFactor)
{
    Transform uninterpolatedTransform = this->mPhysics.rigidBody->getTransform();

    this->mPhysics.currTransform = Transform::interpolateTransforms(this->mPhysics.prevTransform, uninterpolatedTransform, interpFactor);


    this->mPhysics.prevTransform = this->mPhysics.currTransform;


    const reactphysics3d::Vector3& rpnPosition = this->mPhysics.currTransform.getPosition();
    glm::vec3 nPosition = { rpnPosition.x, rpnPosition.y, rpnPosition.z };

    this->mModelTransform[3] = glm::vec4(nPosition, 1);

}

void Object::Draw(VkCommandBuffer cmdBuffer) 
{
    const Texture& texture = _Application->Textures()[this->textureIndex];

    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *this->mPipelineLayout, 0, 1, &texture.mDescriptor, 0, nullptr);

    VkDeviceSize offsets[1] = { 0 };
    VkBuffer  vBuffers[] = { this->vertexBuffer.handle };

    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.handle, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, indexBuffer.handle, 0, VK_INDEX_TYPE_UINT16);

    vkCmdPushConstants(cmdBuffer, *this->mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), (void*)(&mModelTransform));

    vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(indexBufferData.size()), 1, 0, 0, 0);


}

void LoadMeshOBJ(const std::string& path, Object& obj)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    if (!warn.empty()) 
    {
        std::cout << warn << std::endl << std::endl;
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};


    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vert = {};

            vert.pos =
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (index.texcoord_index > 0) 
            {
                vert.uv =
                {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1 - attrib.texcoords[2 * index.texcoord_index + 1] //vulkan is upside down.
                };

            }

            if (uniqueVertices.count(vert) == 0)
            {
                uniqueVertices[vert] = static_cast<uint32_t>(obj.vertexBufferData.size());
                obj.vertexBufferData.push_back(vert);
            }

            obj.indexBufferData.push_back(uniqueVertices[vert]);

        }
    }
}


