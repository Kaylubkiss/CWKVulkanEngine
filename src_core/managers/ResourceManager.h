#pragma once

#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <future>

#include "DescriptorManager.h"
#include "Resource.h"
#include "vkDevice.h"
#include "vkTexture.h"
#include "ThreadPool.h"

struct TransferSubmissionInfo
{
    std::mutex* pTransferMutex = nullptr;
    VkQueue transferQueue = VK_NULL_HANDLE;
    VkCommandBuffer transferCommand = VK_NULL_HANDLE;
    uint32_t sourceQueueFamily = 0;
    uint32_t destinationQueueFamily = 0;
};

template<typename T>
class ResourceHandle;

class ResourceManager
{
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    void Init( vk::Device* devicePtr, uint32_t workerThreadCount )
    {
        assert(workerThreadCount > 0);
        assert(devicePtr != nullptr);

        m_devicePtr = devicePtr;
        m_transferCommandPools.resize(workerThreadCount);
        m_transferCommandBuffers.resize(workerThreadCount);

        VkDevice device = m_devicePtr->GetDevice();
        uint32_t transferQueueFamily = m_devicePtr->GetQueue(DeviceQueue::TRANSFER).family;
        for (size_t i = 0; i < m_transferCommandPools.size(); ++i)
        {
            VkCommandPoolCreateInfo commandPoolCreateInfo = {};
            commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            commandPoolCreateInfo.queueFamilyIndex = transferQueueFamily;

            VK_CHECK_RESULT(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr,
                &m_transferCommandPools[i]));

            VkCommandBufferAllocateInfo commandBufferAI = {};
            commandBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAI.commandPool = m_transferCommandPools[i];
            commandBufferAI.commandBufferCount = 1;

            VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAI, &m_transferCommandBuffers[i]));
        }

        m_threadPool.Init(workerThreadCount);
    }

    void Destroy()
    {
        m_threadPool.Terminate();
    }

    template<typename T>
    std::future<ResourceHandle<T>> AsyncLoad( const std::string& resourceId )
    {
        {
            std::lock_guard lock(m_resourceMapMutex);
            auto& resourceTypeMap = m_resources[std::type_index(typeid(T))];
            auto& refCountTypeMap = m_refCounts[std::type_index(typeid(T))];

            if (resourceTypeMap.contains(resourceId))
            {
                ++refCountTypeMap[resourceId].refCount;

                std::promise<ResourceHandle<T>> readyPromise;

                readyPromise.set_value(ResourceHandle<T>(resourceId, this));

                return readyPromise.get_future();
            }
        }

        auto task = std::make_shared<std::packaged_task<ResourceHandle<T>()>>(
            [this, resourceId]()
        {
            return Load<T>(resourceId);
        });

        auto future = task->get_future();

        m_threadPool.EnqueueTask([task] {
            (*task)();
        });

        return future;
    }

    template<typename T>
    ResourceHandle<T> Load(const std::string& resourceId)
    {
        static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

        {
            std::lock_guard lock(m_resourceMapMutex);
            auto& resourceTypeMap = m_resources[std::type_index(typeid(T))];
            auto& refCountTypeMap = m_refCounts[std::type_index(typeid(T))];

            if (resourceTypeMap.contains(resourceId))
            {
                ++refCountTypeMap[resourceId].refCount;
                return ResourceHandle<T>(resourceId, this);
            }
        }

        std::shared_ptr<Resource> newResource = std::make_shared<T>(resourceId);

        if (!newResource->Load(m_devicePtr, *this))
        {
            return ResourceHandle<T>();
        }

        if (newResource->NeedsTransferSubmission())
        {
            size_t cmdBufferIndex = 0;
            size_t cmdCount = m_transferCommandBuffers.size();
            uint16_t fullMask = (1ULL << cmdCount) - 1;
            {
                std::unique_lock lock(m_commandBufferMutex);
                m_conditionVariable.wait(lock, [this, &cmdCount, fullMask]
                {
                    return transferIsBusyMask != fullMask;
                });

                for (size_t i = 0; i < cmdCount; ++i)
                {
                    if ((transferIsBusyMask & (1 << i)) == 0)
                    {
                        cmdBufferIndex = i;
                        transferIsBusyMask |= (1 << i);
                        break;
                    }
                }
            }

            TransferSubmissionInfo transferInfo = {};
            transferInfo.pTransferMutex = &m_transferMutex;
            transferInfo.transferCommand = m_transferCommandBuffers[cmdBufferIndex];
            transferInfo.destinationQueueFamily = m_devicePtr->GetQueue(DeviceQueue::GRAPHICS).family;
            transferInfo.sourceQueueFamily = m_devicePtr->GetQueue(DeviceQueue::TRANSFER).family;
            transferInfo.transferQueue = m_devicePtr->GetQueue(DeviceQueue::TRANSFER).handle;

            bool successfulLoad = newResource->SubmitDataToTransferQueue(m_devicePtr, transferInfo);

            {
                std::unique_lock lock(m_commandBufferMutex);
                transferIsBusyMask &= ~(1 << cmdBufferIndex);
                m_conditionVariable.notify_one();
            }

            if (successfulLoad == false)
            {
                return ResourceHandle<T>();
            }
        }

        if (newResource->NeedsDescriptor())
        {
            //TODO: request from descriptorManager;
        }

        {
            std::lock_guard lock(m_resourceMapMutex);
            auto& resourceTypeMap = m_resources[std::type_index(typeid(T))];
            auto& refCountTypeMap = m_refCounts[std::type_index(typeid(T))];

            //maybe another thread beat us to the punch.
            if (resourceTypeMap.contains(resourceId))
            {
                ++refCountTypeMap[resourceId].refCount;
                return ResourceHandle<T>(resourceId, this);
            }

            resourceTypeMap[resourceId] = newResource;
            refCountTypeMap[resourceId].resource = newResource;
            refCountTypeMap[resourceId].refCount = 1;

            return ResourceHandle<T>(resourceId, this);
        }
    }

    template<typename T>
    T* GetResource(const std::string& resourceId)
    {
        std::lock_guard lock(m_resourceMapMutex);
        auto& resourceTypeMap = m_resources[std::type_index(typeid(T))];
        auto iterator = resourceTypeMap.find(resourceId);

        if (iterator != resourceTypeMap.end())
        {
            return static_cast<T*>(iterator->second.get());
        }

        return nullptr;
    }

    template<typename T>
    bool HasResource( const std::string& resourceId )
    {
        std::lock_guard lock(m_resourceMapMutex);
        auto& resourceTypeMap = m_resources[std::type_index(typeid(T))];
        return resourceTypeMap.contains(resourceId);
    }

    template<typename T>
    void ReleaseResource(const std::string& resourceId)
    {
        std::lock_guard lock(m_resourceMapMutex);
        auto& resourceTypeMap = m_resources[std::type_index(typeid(T))];
        auto& refCountTypeMap = m_refCounts[std::type_index(typeid(T))];

        if (refCountTypeMap.contains(resourceId))
        {
            --refCountTypeMap[resourceId].refCount;
            if (refCountTypeMap[resourceId].refCount <= 0)
            {
                resourceTypeMap[resourceId]->Unload(m_devicePtr);

                resourceTypeMap.erase(resourceId);
                refCountTypeMap.erase(resourceId);
            }
        }
    }

    //for emergency shutdowns or exiting the program in general.
    void UnloadAll()
    {
        std::lock_guard lock(m_resourceMapMutex);
        for (auto& [type, typeResources] : m_resources)
        {
            for (auto& [resourceId, resource] : typeResources)
            {
                resource->Unload(m_devicePtr);
            }

            typeResources.clear();
        }

        m_refCounts.clear();
    }
private:
    //sort by type first, then by id -> type-safe, efficient resource lookup
    //this prevents collisions when resources have the same name but are of different types (via std::type_index)
    std::unordered_map<std::type_index,
        std::unordered_map<std::string, std::shared_ptr<Resource>>> m_resources;

    //TODO: lookup refcounts for resources
    struct ResourceData
    {
        std::shared_ptr<Resource> resource;
        size_t refCount = 0;
    };
    std::unordered_map<std::type_index,
        std::unordered_map<std::string, ResourceData>> m_refCounts;

    std::mutex m_resourceMapMutex; //don't want multiple threads to access the map at the same time.
    std::mutex m_commandBufferMutex;
    std::condition_variable m_conditionVariable;
    std::mutex m_transferMutex;

    ThreadPool m_threadPool; //async core system

    uint16_t transferIsBusyMask = 0;
    std::vector<VkCommandPool> m_transferCommandPools;
    std::vector<VkCommandBuffer> m_transferCommandBuffers;


    vk::Device* m_devicePtr = nullptr; //TODO: move vk::Device to inherit from a generic device class.
};





