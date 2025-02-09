#include "BlinnPhong.h"
#include "Application.h"

void LightInfoObject::AddPosition(const glm::vec3& pos)
{
	if (curr_index[POS_IND] < 0 && curr_index[POS_IND] != MaxLights)
	{
		const short arr_size = sizeof(glm::vec3);
		*(lightPos + arr_size * curr_index[POS_IND]) = pos;
		++curr_index[POS_IND];
	}

}

void LightInfoObject::Create(const glm::vec3& pos, const glm::vec3& dir)
{
	if (!mBuffer.isAllocated) 
	{
		this->mBuffer = Buffer(sizeof(LightInfoObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, (void*)this);
		mBuffer.isAllocated = true;
	}

	this->AddPosition(pos);
	isUpdated = true;
}

void LightInfoObject::Update() 
{
	if (isUpdated) 
	{
		memcpy(mBuffer.mappedMemory, (void*)this, (size_t)(sizeof(LightInfoObject)));
		isUpdated = false;
	}

}

void LightInfoObject::Deallocate() 
{
	vkFreeMemory(_Application->LogicalDevice(), this->mBuffer.memory, nullptr);
	vkDestroyBuffer(_Application->LogicalDevice(), this->mBuffer.handle, nullptr);
}