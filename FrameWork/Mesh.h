#pragma once
#include "Common.h"
#include <iostream>
#include <vector>
#include <string>

enum MeshType
{
	M_NONE = 0,
	M_CUBE,
};

struct Vertex 
{
	glm::vec3 pos = {0,0,0};
	glm::vec3 nrm = {.2f,.5f,0};
	glm::vec2 uv = {-1.f,-1.f};
};

struct Mesh
{
	int numVertices = 0;
	int numTris = 0;
	int numIndices = 0;

	std::vector<Vertex> vertexBufferData;
	std::vector<uint16_t> indexBufferData; //setting this to uint16_t fixed the issue. INVESTIGATE THIS.

	

};

struct Object 
{
	glm::vec3 mCenter = glm::vec3(0.f);
	glm::mat4 mTransform = glm::mat4(1.f);
	
	Mesh mMesh;
	
	Buffer vertex;
	Buffer index;

	

	Object(const char* fileName, MeshType type = M_NONE);
	Object() : mMesh(), vertex(), index() {};
};

void LoadMeshOBJ(const std::string& path, Mesh& mesh);