#pragma once
#include "Common.h"
#include <vector>

struct Vertex 
{
	glm::vec3 pos;
	glm::vec3 nrm;
	glm::vec3 uv;
};

struct Mesh
{
	int numVertices = 0;
	int numTris = 0;
	int numIndices = 0;

	std::vector<Vertex> vertexBufferData;
	std::vector<int> indexBufferData;

};

struct Object 
{
	glm::mat4 mTransform = glm::mat4(1.f);
	
	Mesh mMesh;
	
	Buffer vertexBuffer;
	Buffer indexBuffer;
	

	Object(const char* fileName);
	Object() : mMesh(), vertexBuffer(), indexBuffer() {};
};

void LoadMeshOBJ(const char* fileName, Mesh& mesh);