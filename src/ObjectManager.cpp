#include "ObjectManager.h"
#include "ThreadPool.h"
#include "vkInit.h"
#include "vkUtility.h"

void ObjectManager::LoadObjParallel(const VkPhysicalDevice p_device, const VkDevice l_device, const char* name, const char* filename, bool willDebugDraw, const glm::mat4& modelTransform)
{
	objects[name] = new Object(p_device, l_device, filename, willDebugDraw, modelTransform);
}

void ObjectManager::LoadObject(const VkPhysicalDevice p_device, const VkDevice l_device, const char* name, const char* filename, bool willDebugDraw, const glm::mat4& modelTransform)
{
	std::function<void()> func = [this, p_device, l_device, modelTransform, name, filename, willDebugDraw] { ObjectManager::LoadObjParallel(p_device, l_device, name, (PathToObjects() + std::string(filename)).c_str(), willDebugDraw, modelTransform); };

	mThreadWorkers.EnqueueTask(func);
	
}

void ObjectManager::Init(const VkDevice l_device, VkCommandPool cmdPool) 
{
	this->secondaryCmdBuffer = vk::init::CommandBuffer(l_device, cmdPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY);

	this->mThreadWorkers.Init(1);
}

void ObjectManager::Draw() 
{

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(this->secondaryCmdBuffer, &beginInfo))

	for (auto& object : objects) 
	{
		object.second->Draw(this->secondaryCmdBuffer);
	}

	//sync this up with primary command buffer in graphics system...
	VK_CHECK_RESULT(vkEndCommandBuffer(this->secondaryCmdBuffer))

}


ObjectManager::ObjectManager() 
{
	//keep it to one thread.
	
}