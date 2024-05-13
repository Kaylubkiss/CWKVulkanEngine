#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Vertex 
{
	glm::vec3 pos;
	glm::vec3 nrm;
	glm::vec3 uv;
};

struct Mesh
{
	std::vector<Vertex> vertexBuffer;
	std::vector<int> indexBuffer;

	int numVertices = 0;
	int numTris = 0;
	int numIndices = 0;
};

void LoadMeshOBJ(char* fileName, Mesh& mesh);