#pragma once
#include "Common.h"


//this class more works like a line debug drawer.
struct DebugDrawObject 
{
private:
	std::vector<Vertex> debugVertexData;
	glm::mat4 modelTransforms;
	uint32_t offsetInLinesArrary = 0;
	Buffer debugVertexBuffer;
	bool debugBufferAllocated = false;
	bool isDebugEnabled = false;

public:
	void SetArrayOffset(uint32_t offsetIntoLinesArray);
	void Draw(VkCommandBuffer cmdBuffer);
	void Update();
	void WillDraw(bool set);
	void DestroyResources();
	void AddModelTransform(const glm::mat4& model);
};
