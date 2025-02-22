#include "Object.h"
#include "ApplicationGlobal.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


Object::Object(const VkPhysicalDevice p_device, const VkDevice l_device, 
                const char* fileName, bool willDebugDraw, 
                const glm::mat4& modelTransform)
{

    (void)(willDebugDraw);

    LoadMeshOBJ((OBJECT_PATH + std::string(fileName)).c_str(), *this);

    this->mMesh.modelTransform = modelTransform;
    int numVertices = static_cast<int>(this->mMesh.data.vertices.size());

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
    std::cout << numVertices << " vertices loaded in." << std::endl << std::endl;    
}

void Object::UpdateTexture(const VkDescriptorSet textureDescriptor)
{
    this->mTextureDescriptor = textureDescriptor;
}

void Object::UpdatePipelineLayout(const VkPipelineLayout pipelineLayout)
{
    this->mPipelineLayout = pipelineLayout;
}

void Object::UpdatePhysicsComponent(PhysicsComponent* physComp) 
{
   
    if (physComp != nullptr) 
    {

        mPhysicsComponent = *physComp;
        
        assert(_Application != NULL);
        PhysicsSystem& appPhysics = _Application->GetPhysics();

        Object::InitPhysics(appPhysics);        
    }

}

void Object::InitPhysics(PhysicsSystem& appPhysics)
{

    glm::vec4 worldMinPoints = mMesh.modelTransform * glm::vec4(mMesh.minLocalPoints, 1);
    glm::vec4 worldMaxPoints = mMesh.modelTransform * glm::vec4(mMesh.maxLocalPoints, 1);

    const glm::vec4& dc2Position = .5f * (worldMinPoints + worldMaxPoints);
    reactphysics3d::Vector3 position(dc2Position.x, dc2Position.y, dc2Position.z);
    reactphysics3d::Quaternion orientation = Quaternion::identity();
    reactphysics3d::Transform transform(position, orientation);

   
    this->mPhysicsComponent.rigidBody = appPhysics.AddRigidBody(transform);

    if (this->mPhysicsComponent.bodyType != BodyType::DYNAMIC)
    {
        this->mPhysicsComponent.rigidBody->setType(this->mPhysicsComponent.bodyType);
    }

    switch (this->mPhysicsComponent.colliderType)
    {
        case PhysicsComponent::ColliderType::CUBE:
            glm::vec3 worldHalfExtent = glm::vec3((worldMaxPoints - worldMinPoints) * .5f);

            this->mPhysicsComponent.shape = appPhysics.CreateBoxShape({ std::abs(worldHalfExtent.x), std::abs(worldHalfExtent.y), std::abs(worldHalfExtent.z) });
            break;
        case PhysicsComponent::ColliderType::NONE:
            break;
        default:
            break;
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
   //this->debugDrawObject.DestroyResources();

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

        this->mMesh.modelTransform = nModel;
    }

    //this->debugDrawObject.Update();

}

//void Object::SetLinesArrayOffset(uint32_t index) 
//{
//    this->debugDrawObject.SetArrayOffset(index);
//}

void Object::Draw(VkCommandBuffer cmdBuffer) 
{  
   
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->mPipelineLayout, 0, 1, &this->mTextureDescriptor, 0, nullptr);
    
    VkDeviceSize offsets[1] = { 0 };
    VkBuffer  vBuffers[] = { this->mMesh.buffer.vertex.handle };
    
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &mMesh.buffer.vertex.handle, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, mMesh.buffer.index.handle, 0, VK_INDEX_TYPE_UINT16);
    
    vkCmdPushConstants(cmdBuffer, this->mPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), (void*)(&this->mMesh.modelTransform));
    
    vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(this->mMesh.data.indices.size()), 1, 0, 0, 0);
}

void Object::ComputeVertexNormals()
{
    std::vector<Vertex>& vertexBufferData = this->mMesh.data.vertices;
    std::vector<uint16_t>& indexBufferData = this->mMesh.data.indices;

    for (int i = 0; i < vertexBufferData.size(); ++i)
    {
        glm::vec3 total_vec(0.0f);

        for (int j = 0; j < indexBufferData.size(); ++j)
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
                uniqueVertices[vert] = static_cast<uint32_t>(obj.mMesh.data.vertices.size());
                obj.mMesh.data.vertices.push_back(vert);
            }

            obj.mMesh.data.indices.push_back(uniqueVertices[vert]);

        }
    }
}



