#include "Debug.h"
#include "Application.h"

void DebugDrawObject::Draw(VkCommandBuffer cmdBuffer) 
{
	if (_Application == NULL) 
	{
		return;
	}

	if (this->isDebugEnabled && this->debugBufferAllocated)
	{
		
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _Application->GetLinePipeline());

		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &this->debugVertexBuffer.handle, offsets);
		vkCmdDraw(cmdBuffer, static_cast<uint32_t>(debugVertexData.size()), 1, 0, 0);


		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _Application->GetTrianglePipeline());

		
	}

}

void DebugDrawObject::Update() 
{
	reactphysics3d::DebugRenderer& debugRenderer = _Application->GetPhysicsWorld()->getDebugRenderer();

	uint32_t sizeOfLinesArray = debugRenderer.getNbLines();

	if (!this->debugBufferAllocated && sizeOfLinesArray > 0)
	{
		debugBufferAllocated = true;
		for (size_t i = 0; i < 12; ++i)
		{
			const reactphysics3d::DebugRenderer::DebugLine& tri = debugRenderer.getLinesArray()[i + offsetInLinesArrary];

			Vertex vert;
			vert.pos = glm::inverse(this->modelTransforms) * glm::vec4(tri.point1.x, tri.point1.y, tri.point1.z, 1);

			debugVertexData.push_back(vert);

			vert.pos = glm::inverse(this->modelTransforms) * glm::vec4(tri.point2.x, tri.point2.y, tri.point2.z, 1);

			debugVertexData.push_back(vert);
		}

		this->debugVertexBuffer = Buffer(sizeof(std::vector<Vertex>) + (sizeof(Vertex) * this->debugVertexData.size()), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->debugVertexData.data());
	}

}


void DebugDrawObject::WillDraw(bool set) 
{
	this->isDebugEnabled = set;
}

void DebugDrawObject::DestroyResources() 
{
	vkFreeMemory(_Application->LogicalDevice(), this->debugVertexBuffer.memory, nullptr);
	vkDestroyBuffer(_Application->LogicalDevice(), this->debugVertexBuffer.handle, nullptr);
}

void DebugDrawObject::AddModelTransform(const glm::mat4& model) 
{
	this->modelTransforms = model;
}

void DebugDrawObject::SetArrayOffset(uint32_t offsetIntoLinesArray) 
{
	this->offsetInLinesArrary = offsetIntoLinesArray;
}