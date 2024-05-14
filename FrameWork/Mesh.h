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
	int numVertices = 0;
	int numTris = 0;
	int numIndices = 0;

	std::vector<Vertex> vertexBuffer;
	std::vector<int> indexBuffer;

};

struct Object 
{
	glm::mat4 m_ModelTransform;

	Mesh m_Mesh;
};

void LoadMeshOBJ(const char* fileName, Mesh& mesh);