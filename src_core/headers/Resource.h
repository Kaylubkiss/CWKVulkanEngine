#pragma once

struct TransferSubmissionInfo;
class ResourceManager;

class Resource
{
public:
    Resource() = default;

    explicit Resource(const std::string& id)
    {
        m_id = id;
    }

    virtual ~Resource() = default;

    [[nodiscard]] const std::string& GetId() const
    {
        return m_id;
    }

    [[nodiscard]] bool IsLoaded() const
    {
        return isLoaded;
    }

    bool Load( vk::Device* devicePtr, ResourceManager& resourceManager )
    {
        isLoaded = doLoad(devicePtr, resourceManager);
        return isLoaded;
    }

    bool SubmitDataToTransferQueue( vk::Device* devicePtr, TransferSubmissionInfo& transferInfo )
    {
        isLoaded = doTransferSubmission(devicePtr, transferInfo);
        return isLoaded;
    }

    [[nodiscard]] virtual bool NeedsTransferSubmission() const
    {
        return false;
    }

    [[nodiscard]] virtual bool NeedsDescriptor() const
    {
        return false;
    }

    void Unload(vk::Device* devicePtr)
    {
        doUnload(devicePtr);
        isLoaded = false;
    }
protected:
    //derived classes will have to figure out how to implement these
    virtual bool doLoad( vk::Device* devicePtr, ResourceManager& resourceManager ) = 0;
    virtual bool doTransferSubmission( vk::Device* devicePtr, TransferSubmissionInfo& transferInfo )
    {
        (void)(devicePtr);
        (void)(transferInfo);
        return false;
    }
    virtual void doUnload( vk::Device* devicePtr ) = 0;
private:
    std::string m_id; //usually a URL or name of the resource
    bool isLoaded = false;
};