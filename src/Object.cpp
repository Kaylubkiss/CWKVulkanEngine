#include "Object.h"
#include "Application.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Object::Object(const char* fileName, bool willDebugDraw, const glm::mat4& modelTransform, const char* textureName, VkPipelineLayout* pipelineLayout)
{

   // LoadMeshOBJ(fileName, *this);


   // this->mModelTransform = modelTransform;

   // this->numVertices = static_cast<int>(this->vertexBufferData.size());

   // glm::vec3 min_points(0.f);
   // glm::vec3 max_points(0.f);

   // for (unsigned i = 0; i < this->vertexBufferData.size(); ++i)
   // {

   //     min_points.x = std::min(min_points.x, vertexBufferData[i].pos.x);
   //     min_points.y = std::min(min_points.y, vertexBufferData[i].pos.y);
   //     min_points.z = std::min(min_points.z, vertexBufferData[i].pos.z);

   //     max_points.x = std::max(max_points.x, vertexBufferData[i].pos.x);
   //     max_points.y = std::max(max_points.y, vertexBufferData[i].pos.y);
   //     max_points.z = std::max(max_points.z, vertexBufferData[i].pos.z);

   //     this->vertexBufferData[i].nrm = glm::vec3(0, 0, 0.f);

   //     mCenter += vertexBufferData[i].pos;
   // }

   // mCenter /= this->vertexBufferData.size();
   ///* mCenter = (max_points + min_points) * 0.5f;*/

   // float unitScale = std::max({ glm::length(max_points.x - min_points.x), glm::length(max_points.y - min_points.y), glm::length(max_points.z - min_points.z) });

   // max_points = { -std::numeric_limits<float>::min(),  -std::numeric_limits<float>::min() , -std::numeric_limits<float>::min() };
   // min_points = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };

   // for (size_t i = 0; i < this->vertexBufferData.size(); ++i)
   // {
   //     this->vertexBufferData[i].pos = (this->vertexBufferData[i].pos - mCenter) / unitScale;

   //     max_points.x = std::max(max_points.x, vertexBufferData[i].pos.x);
   //     max_points.y = std::max(max_points.y, vertexBufferData[i].pos.y);
   //     max_points.z = std::max(max_points.z, vertexBufferData[i].pos.z);

   //     min_points.x = std::min(min_points.x, vertexBufferData[i].pos.x);
   //     min_points.y = std::min(min_points.y, vertexBufferData[i].pos.y);
   //     min_points.z = std::min(min_points.z, vertexBufferData[i].pos.z);
   // }

   // mMaxLocalPoints = max_points;
   // mMinLocalPoints = min_points;
   // //mHalfExtent.normalize();

   // this->ComputeVertexNormals();

   // size_t sizeOfVertexBuffer = sizeof(std::vector<Vertex>) + (sizeof(Vertex) * this->vertexBufferData.size());
   // this->vertexBuffer = vk::Buffer(sizeOfVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->vertexBufferData.data());
   // size_t sizeOfIndexBuffer = sizeof(std::vector<uint16_t>) + (sizeof(uint16_t) * this->indexBufferData.size());
   // this->indexBuffer = Buffer(sizeOfIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->indexBufferData.data());

   // std::cout << std::endl;
   // std::cout << "loaded in " + std::string(fileName) << std::endl;
   // std::cout << this->numVertices << " vertices loaded in." << std::endl << std::endl;


   // this->debugDrawObject.WillDraw(willDebugDraw);

   // if (textureName != nullptr) 
   // {
   //     assert(_Application != NULL);

   //     this->textureIndex = _Application->GetTexture(textureName);
   // }

   // if (pipelineLayout != nullptr) 
   // {
   //     assert(_Application != NULL);

   //     this->mPipelineLayout = _Application->GetPipelineLayout();
   // }

    
}

void Object::UpdateTexture(const char* textureName) 
{
   /* if (textureName != nullptr)
    {
        this->textureIndex = _Application->GetTexture(textureName);
    }*/
}

void Object::UpdatePipelineLayout(VkPipelineLayout* pipelineLayout) 
{
    /*if (pipelineLayout != nullptr)
    {
        this->mPipelineLayout = _Application->GetPipelineLayout();
    }*/
}

void Object::InitPhysics(ColliderType cType, BodyType bType)
{
    //glm::vec4 worldMinPoints = mModelTransform * glm::vec4(mMinLocalPoints, 1);
    //glm::vec4 worldMaxPoints = mModelTransform * glm::vec4(mMaxLocalPoints, 1);

    //const glm::vec4& dc2Position = .5f * (worldMinPoints + worldMaxPoints);
    //reactphysics3d::Vector3 position(dc2Position.x, dc2Position.y, dc2Position.z);
    //reactphysics3d::Quaternion orientation = Quaternion::identity();
    //reactphysics3d::Transform transform(position, orientation);

   
    //this->mPhysicsComponent.rigidBody = _Application->PhysicsSystem().AddRigidBody(transform);

    //if (bType != BodyType::DYNAMIC)
    //{
    //   this->mPhysicsComponent.rigidBody->setType(bType);
    //   this->mPhysicsComponent.bodyType = bType;
    //}

    //switch (cType) 
    //{
    //    case ColliderType::CUBE:
    //        glm::vec3 worldHalfExtent = glm::vec3((worldMaxPoints - worldMinPoints) * .5f);

    //        this->mPhysicsComponent.shape = _Application->PhysicsSystem().CreateBoxShape({ std::abs(worldHalfExtent.x), std::abs(worldHalfExtent.y), std::abs(worldHalfExtent.z) });
    //        break;
    //    case ColliderType::NONE:
    //        break;
    //    default:
    //        break;
    //}


    ////the collider transform is relative to the rigidbody origin.
    //if (this->mPhysicsComponent.shape != nullptr)
    //{
    //    this->mPhysicsComponent.collider = this->mPhysicsComponent.rigidBody->addCollider(this->mPhysicsComponent.shape, Transform::identity());
    //}


    //this->mPhysicsComponent.prevTransform = this->mPhysicsComponent.rigidBody->getTransform();
}

void Object::DestroyResources()
{
    /*assert(_Application != NULL);

    vkFreeMemory(_Application->LogicalDevice(), this->vertexBuffer.memory, nullptr);
    vkDestroyBuffer(_Application->LogicalDevice(), this->vertexBuffer.handle, nullptr);
    
    vkFreeMemory(_Application->LogicalDevice(), this->indexBuffer.memory, nullptr);
    vkDestroyBuffer(_Application->LogicalDevice(), this->indexBuffer.handle, nullptr);

    this->debugDrawObject.DestroyResources();*/
}

void Object::willDebugDraw(bool option) 
{
    //this->debugDrawObject.WillDraw(option);
    ////not memory efficient.
    //this->debugDrawObject.AddModelTransform(this->mModelTransform);
}

void Object::Update(const float& interpFactor)
{

    //if (this->mPhysicsComponent.bodyType != BodyType::STATIC)
    //{
    //    Transform uninterpolatedTransform = this->mPhysicsComponent.rigidBody->getTransform();

    //    this->mPhysicsComponent.currTransform = Transform::interpolateTransforms(this->mPhysicsComponent.prevTransform, uninterpolatedTransform, interpFactor);

    //    this->mPhysicsComponent.prevTransform = this->mPhysicsComponent.currTransform;

    //    float matrix[16];

    //    this->mPhysicsComponent.currTransform.getOpenGLMatrix(matrix);

    //   
    //    //this makes this stuff too dang easy.
    //    glm::mat4 nModel = glm::mat4(matrix[0], matrix[1], matrix[2], matrix[3], 
    //                                 matrix[4], matrix[5], matrix[6], matrix[7],
    //                                 matrix[8], matrix[9], matrix[10], matrix[11],
    //                                 matrix[12], matrix[13], matrix[14], matrix[15]);

    //    this->mModelTransform = nModel;
    //}

    //this->debugDrawObject.Update();

}

void Object::SetLinesArrayOffset(uint32_t index) 
{
    this->debugDrawObject.SetArrayOffset(index);
}

void Object::Draw(VkCommandBuffer cmdBuffer) 
{
   /* assert(_Application != NULL);

    if (!this->debugDrawObject.onlyVisible()) 
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
    else 
    {
        vkCmdPushConstants(cmdBuffer, *this->mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), (void*)(&mModelTransform));
    }

    if (this->debugDrawObject.isVisible() || this->debugDrawObject.onlyVisible())
    {
        this->debugDrawObject.Draw(cmdBuffer);
    }*/

}

static void LoadMeshOBJ(const std::string& path, Object& obj)
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


void Object::ComputeVertexNormals() 
{
    
        for (int i = 0; i < this->vertexBufferData.size(); ++i)
        {
            glm::vec3 total_vec(0.0f);

            for (int j = 0; j < this->indexBufferData.size(); ++j)
            {
                //total_vec
                if (indexBufferData[j] == i)
                {
                    //angle = glm::angle(q -p, r - p)
                    //total_vec += angle * cross(q - r, r - p)
                    float angle;
                    glm::vec3 normal;
                    glm::vec3 orientation_QP;
                    glm::vec3 orientation_RP;
                    glm::vec3 edge_1;
                    glm::vec3 edge_2;

                    if (j % 3 == 0) //beginning of the face index
                    {
                        orientation_QP = vertexBufferData[indexBufferData[j + 1]].pos - vertexBufferData[indexBufferData[j]].pos;
                        orientation_RP = vertexBufferData[indexBufferData[j + 2]].pos - vertexBufferData[indexBufferData[j]].pos;

                        edge_1 = orientation_QP;
                        edge_2 = orientation_RP;
                    }
                    else if (j % 3 == 1) //middle of the face index
                    {
                        orientation_QP = vertexBufferData[indexBufferData[j]].pos - vertexBufferData[indexBufferData[j - 1]].pos;
                        orientation_RP = vertexBufferData[indexBufferData[j + 1]].pos - vertexBufferData[indexBufferData[j - 1]].pos;

                        edge_1 = vertexBufferData[indexBufferData[j - 1]].pos - vertexBufferData[indexBufferData[j]].pos;
                        edge_2 = vertexBufferData[indexBufferData[j + 1]].pos - vertexBufferData[indexBufferData[j]].pos;

                    }
                    else if (j % 3 == 2) //end of face index sequence
                    {
                        orientation_QP = vertexBufferData[indexBufferData[j - 1]].pos - vertexBufferData[indexBufferData[j - 2]].pos;
                        orientation_RP = vertexBufferData[indexBufferData[j]].pos - vertexBufferData[indexBufferData[j - 2]].pos;

                        edge_1 = vertexBufferData[indexBufferData[j - 2]].pos - vertexBufferData[indexBufferData[j]].pos;
                        edge_2 = vertexBufferData[indexBufferData[j - 1]].pos - vertexBufferData[indexBufferData[j]].pos;
                    }


                    angle = glm::degrees(acos((abs(glm::dot(edge_1, edge_2)) /
                        (glm::length(edge_1) * glm::length(edge_2)))));
                    normal = glm::cross(orientation_QP, orientation_RP);
                    //The angle needs to be between the edges that *SHARE* the vertex.
                    total_vec += (angle * normal);
                }
            }

            vertexBufferData[i].nrm = glm::normalize(total_vec); //point + vector equals another point

        } //calculate the normals
}
