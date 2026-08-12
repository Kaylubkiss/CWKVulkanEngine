#include "Object.h"
#include "GLTFModel.h"
#include "OBJModel.h"

//normally I'd say ObjectManager should take care of these paths, but since loading was already
//implemented with checking the extensions, they'll be placed here. Perhaps change this later.
static constexpr std::string_view GLTF_OBJECT_PATH = "art/gltf/";
static constexpr std::string_view OBJ_PATH = "art/obj/";

Object::Object( const ObjectCreateInfo& objectCI, vk::TextureManager& textureManager )
{
    assert(objectCI.devicePtr != nullptr);
    
    std::filesystem::path filePath(objectCI.objName);

    std::string ext = filePath.extension().string();
    std::ranges::transform(ext, ext.begin(), ::tolower);

    if (ext == ".gltf")
    {
        filePath = std::string(GLTF_OBJECT_PATH) + filePath.string();

        if (std::filesystem::exists(filePath) == false)
        {
            std::cerr << filePath.string() + " doesn't exist!\n";
            throw std::runtime_error("Object() Failed!");
        }

        if (objectCI.textureFileNames.empty() == false)
        {
            std::cout << "\033[33m [WARNING] .gltf will not use specified texture name in ObjectCreateInfo \033[0m\n";
        }

        m_model = std::make_unique<GLTFModel>(objectCI.devicePtr, filePath);

        std::vector<std::string> gltf_fileNames = m_model->GetTextureNames();

        for ( auto& fileName : gltf_fileNames )
        {
            fileName = filePath.parent_path().string() + "/" + fileName;
        }

        m_model->LoadTextures(textureManager, gltf_fileNames);
    }
    else if (ext == ".obj")
    {
        filePath = std::string(OBJ_PATH) + filePath.string();

        if (std::filesystem::exists(filePath) == false)
        {
            std::cerr << filePath.string() + " doesn't exist!\n";
            throw std::runtime_error("Object() Failed!");
        }

        m_model = std::make_unique<OBJModel>(objectCI.devicePtr, filePath);

        if ( objectCI.textureFileNames.empty() == false )
        {
            m_model->LoadTextures(textureManager, objectCI.textureFileNames);
        }
    }
    else
    {
        throw std::runtime_error("Extension not supported. Object::CreateModelBasedOnFilePath() Failed!");
    }

    m_model->UpdateModelTransform(objectCI.modelTransform);
}

void Object::LoadTextures( vk::TextureManager& textureManager, const std::vector<std::string>& fileNames )
{
    m_model->LoadTextures(textureManager, fileNames);
}

void Object::InitPhysics( const PhysicsInitInfo& physicsInfo )
{
    auto& appPhysics = app.GetPhysics();

    m_physicsComponent = PhysicsComponent();

    const glm::mat4& modelTransform = m_model->GetModelTransform();

    glm::vec4 worldMinPoints = glm::vec4(m_model->GetMinPoint(), 1);
    glm::vec4 worldMaxPoints = glm::vec4(m_model->GetMaxPoint(), 1);

    const glm::vec4 center = modelTransform * (.5f * (worldMinPoints + worldMaxPoints));

    glm::vec3 scale(glm::length(
           glm::vec3(modelTransform[0])),
           glm::length(glm::vec3(modelTransform[1])),
           glm::length(glm::vec3(modelTransform[2]))
           );

    m_physicsComponent.value().scale = scale;

    reactphysics3d::Vector3 position(center.x, center.y, center.z);
    reactphysics3d::Quaternion orientation = reactphysics3d::Quaternion::identity();
    reactphysics3d::Transform transform(position, orientation);

   
    m_physicsComponent.value().rigidBody = appPhysics.AddRigidBody(transform);

    //setting the body type of the rigidbody
    if (physicsInfo.bodyType != reactphysics3d::BodyType::DYNAMIC)
    {
        m_physicsComponent.value().rigidBody->setType(physicsInfo.bodyType);
    }
    
    //creating a collision shape
    if (physicsInfo.colliderType == PhysicsInitInfo::ColliderType::CUBE)
    {
        glm::vec3 worldHalfExtent = scale * glm::vec3((worldMaxPoints - worldMinPoints) * .5f);

        m_physicsComponent.value().shape =
            appPhysics.CreateBoxShape({ worldHalfExtent.x, worldHalfExtent.y, worldHalfExtent.z });
    }

    //the collider transform is relative to the rigidbody origin.
    if (m_physicsComponent.value().shape != nullptr)
    {
        m_physicsComponent.value().collider = m_physicsComponent.value().rigidBody->addCollider(
            m_physicsComponent.value().shape,
            reactphysics3d::Transform::identity());
    }

    m_physicsComponent.value().prevTransform = m_physicsComponent.value().rigidBody->getTransform();
}


void Object::Update(const float& interpFactor)
{
    if (m_physicsComponent.has_value())
    {
        reactphysics3d::Transform uninterpolatedTransform =
            m_physicsComponent.value().rigidBody->getTransform();

        m_physicsComponent.value().currTransform =
            reactphysics3d::Transform::interpolateTransforms(
                m_physicsComponent.value().prevTransform,
                uninterpolatedTransform, interpFactor);

        m_physicsComponent.value().prevTransform = m_physicsComponent.value().currTransform;

        float matrix[16];

        m_physicsComponent.value().currTransform.getOpenGLMatrix(matrix);

        glm::mat4 nModel = glm::mat4(matrix[0], matrix[1], matrix[2], matrix[3],
                                     matrix[4], matrix[5], matrix[6], matrix[7],
                                     matrix[8], matrix[9], matrix[10], matrix[11],
                                     matrix[12], matrix[13], matrix[14], matrix[15]);

        nModel = nModel * glm::scale(glm::mat4(1.f), m_physicsComponent.value().scale);

        m_model->UpdateModelTransform( nModel );
    }
}

void Object::Draw( const vk::DrawInfo& drawInfo ) const
{  
    m_model->Draw(drawInfo);
}


