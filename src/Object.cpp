#include "Object.h"
#include "GLTFModel.h"
#include "OBJModel.h"

//normally I'd say ObjectManager should take care of these paths, but since loading was already
//implemented with checking the extensions, they'll be placed here. Perhaps change this later.
#define GLTF_OBJECT_PATH "art/objects/gltf/"
#define OBJECT_PATH "art/objects/"

Object::Object( const ObjectCreateInfo& objectCI, TextureManager& textureManager )
{
    assert(objectCI.devicePtr != nullptr);
    
    std::filesystem::path filePath(std::string(objectCI.objName));

    if (filePath.extension() == ".gltf")
    {
        filePath = GLTF_OBJECT_PATH + filePath.string();

        if (std::filesystem::exists(filePath) == false)
        {
            std::cerr << filePath.string() + " doesn't exist!\n";
            throw std::runtime_error("Object() Failed!");
        }

        if (objectCI.textureFileName != nullptr)
        {
            std::cout << "\033[33m[WARNING] .gltf will not use specified texture name in ObjectCreateInfo\033[0m\n";
        }

        m_model = std::make_unique<GLTFModel>(objectCI.devicePtr, filePath);

        std::vector<std::string> gltf_fileNames = dynamic_cast<GLTFModel*>(m_model.get())->GetTextureFileNames();
        m_model->LoadTextures(textureManager, gltf_fileNames);
    }
    else if (filePath.extension() == ".obj")
    {
        filePath = OBJECT_PATH + filePath.string();

        if (std::filesystem::exists(filePath) == false)
        {
            std::cerr << filePath.string() + " doesn't exist!\n";
            throw std::runtime_error("Object() Failed!");
        }

        m_model = std::make_unique<OBJModel>(objectCI.devicePtr, filePath);

        if (objectCI.textureFileName != nullptr)
        {
            m_model->LoadTextures(textureManager, {objectCI.textureFileName});
        }
    }
    else
    {
        throw std::runtime_error("Extension not supported. Object::CreateModelBasedOnFilePath() Failed!");
    }

    m_model->UpdateModelTransform(objectCI.modelTransform);

    if (objectCI.hasPhysicsComponent) 
    {
        m_physicsComponent = objectCI.physicsComponent;
        InitPhysics();
    }
}

void Object::LoadTextures( TextureManager& textureManager, const std::vector<std::string>& fileNames )
{
    m_model->LoadTextures(textureManager, fileNames);
}

void Object::InitPhysics()
{
    PhysicsSystem& appPhysics = _Application->GetPhysics();

    glm::vec4 worldMinPoints = glm::vec4(m_model->GetMinPoint(), 1);
    glm::vec4 worldMaxPoints = glm::vec4(m_model->GetMaxPoint(), 1);

    const glm::vec4 dc2Position = .5f * (worldMinPoints + worldMaxPoints);
    reactphysics3d::Vector3 position(dc2Position.x, dc2Position.y, dc2Position.z);
    reactphysics3d::Quaternion orientation = Quaternion::identity();
    reactphysics3d::Transform transform(position, orientation);

   
    m_physicsComponent.rigidBody = appPhysics.AddRigidBody(transform);

    //setting the body type of the rigidbody
    if (m_physicsComponent.bodyType != BodyType::DYNAMIC)
    {
        m_physicsComponent.rigidBody->setType(m_physicsComponent.bodyType);
    }
    
    //creating a collision shape
    if (m_physicsComponent.colliderType == PhysicsComponent::ColliderType::CUBE) 
    {
        glm::vec3 worldHalfExtent = glm::vec3((worldMaxPoints - worldMinPoints) * .5f);
        m_physicsComponent.shape = appPhysics.CreateBoxShape({ std::abs(worldHalfExtent.x), std::abs(worldHalfExtent.y), std::abs(worldHalfExtent.z) });
    }

    //the collider transform is relative to the rigidbody origin.
    if (m_physicsComponent.shape != nullptr)
    {
        m_physicsComponent.collider = m_physicsComponent.rigidBody->addCollider(m_physicsComponent.shape, Transform::identity());
    }

    m_physicsComponent.prevTransform = m_physicsComponent.rigidBody->getTransform();

    m_physicsComponent.isInitialized = true;
}


void Object::Update(const float& interpFactor)
{
    if (m_physicsComponent.isInitialized == true && 
        m_physicsComponent.bodyType != BodyType::STATIC)
    {
        Transform uninterpolatedTransform = m_physicsComponent.rigidBody->getTransform();

        m_physicsComponent.currTransform = Transform::interpolateTransforms(m_physicsComponent.prevTransform, uninterpolatedTransform, interpFactor);

        m_physicsComponent.prevTransform = m_physicsComponent.currTransform;

        float matrix[16];

        m_physicsComponent.currTransform.getOpenGLMatrix(matrix);

        //this makes this stuff too dang easy.
        glm::mat4 nModel = glm::mat4(matrix[0], matrix[1], matrix[2], matrix[3], 
                                     matrix[4], matrix[5], matrix[6], matrix[7],
                                     matrix[8], matrix[9], matrix[10], matrix[11],
                                     matrix[12], matrix[13], matrix[14], matrix[15]);

        m_model->UpdateModelTransform(nModel);
    }


}

void Object::Draw( const vk::DrawInfo& drawInfo ) const
{  
    m_model->Draw(drawInfo);
}


