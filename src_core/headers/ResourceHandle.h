#pragma once

#include "ResourceManager.h"

template<typename T>
class ResourceHandle
{
public:
    ResourceHandle()
    {
        m_resourceManagerPtr = nullptr;
    }

    ResourceHandle( const std::string& id, ResourceManager* resourceManager )
    {
        m_id = id;
        m_resourceManagerPtr = resourceManager;
    }

    T* Get() const
    {
        if (m_resourceManagerPtr == nullptr)
        {
            return nullptr;
        }

        return m_resourceManagerPtr->GetResource<T>(m_id);
    }

    [[nodiscard]] bool IsValid() const
    {
        return m_resourceManagerPtr != nullptr && m_resourceManagerPtr->HasResource<T>(m_id);
    }

    T* operator->() const
    {
        return Get();
    }

    T& operator*() const
    {
        return *Get();
    }

    operator bool() const
    {
        return IsValid();
    }
private:
    std::string m_id;
    ResourceManager* m_resourceManagerPtr = nullptr;
};






