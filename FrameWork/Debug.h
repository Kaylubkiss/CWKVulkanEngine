#pragma once
#include "Common.h"
#include <SDL2/SDL.h>


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
	bool isOnlyDrawing = false;
	bool isDrawing = false;

public:
	void SetArrayOffset(uint32_t offsetIntoLinesArray);
	void ToggleVisibility(SDL_Keycode symbol, Uint8 lshift);
	bool onlyVisible();
	bool isVisible();
	void Draw(VkCommandBuffer cmdBuffer);
	void Update();
	void WillDraw(bool set);
	void DestroyResources();
	void AddModelTransform(const glm::mat4& model);
};
