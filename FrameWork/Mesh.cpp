#include "Mesh.h"
#include <iostream>
#include <algorithm>

Object::Object(const char* fileName) 
{
    LoadMeshOBJ("cube.obj", this->mMesh);

    glm::vec3 min_points(0.f);
    glm::vec3 max_points(0.f);

    for (unsigned i = 0; i < this->mMesh.vertexBufferData.size(); ++i)
    {
        min_points.x = std::min(min_points.x, mMesh.vertexBufferData[i].pos.x);
        min_points.y = std::min(min_points.y, mMesh.vertexBufferData[i].pos.y);
        min_points.z = std::min(min_points.z, mMesh.vertexBufferData[i].pos.z);

        max_points.x = std::max(max_points.x, mMesh.vertexBufferData[i].pos.x);
        max_points.y = std::max(max_points.y, mMesh.vertexBufferData[i].pos.y);
        max_points.z = std::max(max_points.z, mMesh.vertexBufferData[i].pos.z);
    }

    mCenter = (max_points + min_points) / 2.f;
    float unitScale = std::max({ glm::length(max_points.x - min_points.x), glm::length(max_points.y - min_points.y), glm::length(max_points.z - min_points.z) });

    for (size_t i = 0; i < this->mMesh.vertexBufferData.size(); ++i)
    {
        this->mMesh.vertexBufferData[i].pos = (this->mMesh.vertexBufferData[i].pos - mCenter) / unitScale;
    }

  
    //back face
    mMesh.vertexBufferData[0].uv = { 1,0 };
    mMesh.vertexBufferData[2].uv = { 1,1 };
    mMesh.vertexBufferData[6].uv = { 0,1 };
    mMesh.vertexBufferData[4].uv = { 0,0 };


    //front face
    mMesh.vertexBufferData[1].uv = { 0,0 };
    mMesh.vertexBufferData[3].uv = { 0,1 };
    mMesh.vertexBufferData[5].uv = { 1,0 };
    mMesh.vertexBufferData[7].uv = { 1,1 };

    size_t sizeOfVertexBuffer = (sizeof(Vertex) * this->mMesh.vertexBufferData.size());
    this->vertex = Buffer(sizeOfVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->mMesh.vertexBufferData.data());

    size_t sizeOfIndexBuffer = (sizeof(int) * this->mMesh.indexBufferData.size());
    this->index = Buffer(sizeOfIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->mMesh.indexBufferData.data());



}


static bool readDataLine(char* lineBuf, int& lineNum, FILE* fp, int MAX_LINE_LEN)
{
	while (!feof(fp)) 
	{
		++lineNum;

		char* line = fgets(lineBuf, MAX_LINE_LEN + 1, fp);

		int lineLen = strlen(lineBuf);
		if (lineLen == MAX_LINE_LEN && lineBuf[MAX_LINE_LEN - 1] != '\n') 
		{
			continue;
		}

		if (lineLen > 1 && lineBuf[0] == '#') 
		{
			continue;
		}

		if (lineLen == 1) 
		{
			continue;
		}

		for (unsigned i = 0; i < lineLen; ++i) 
		{
			if (!isspace(lineBuf[i])) 
			{
				return true;
			}
		}
	}

	return false;

}

//boiler plate from framework of graphics class.
void LoadMeshOBJ(const char* fileName, Mesh& mesh) 
{
    const int MAX_LINE_LEN = 1024;
    char lineBuf[MAX_LINE_LEN + 1];
    int numLines;

    FILE* fp;
    if (fopen_s(&fp, fileName, "r") != 0)
    {
        std::cerr << "Failed to open " << fileName << "\n";
        exit(1);
    }

    int posID = 0;

    while (!feof(fp))
    {
        if (readDataLine(lineBuf, numLines, fp, MAX_LINE_LEN))
        {
            if (lineBuf[0] == 'v')
            {
                char dataType[MAX_LINE_LEN + 1];
                float x, y, z;
                sscanf_s(lineBuf, "%s %f %f %f", dataType, sizeof(dataType), &x, &y, &z);

                Vertex v;
                if (!strcmp(dataType, "v"))
                {
                    v.pos = glm::vec3(x, y, z);
                    if (posID >= mesh.numVertices)
                    {
                        mesh.vertexBufferData.push_back(v);
                        ++mesh.numVertices;
                    }
                    else 
                    {
                        mesh.vertexBufferData[posID].pos = v.pos;
                    }

                    ++posID;
                }
            }
            else if (lineBuf[0] == 'f')
            {
                ++mesh.numTris;

                std::vector<char*> faceData;
                char* tokWS, * ptrFront, * ptrRear;
                char* ct;

                tokWS = strtok_s(lineBuf, " ", &ct);
                tokWS = strtok_s(NULL, " ", &ct);
                while (tokWS != NULL)
                {
                    faceData.push_back(tokWS);
                    tokWS = strtok_s(NULL, " ", &ct);
                }

                if (faceData.size() > 3) 
                {
                    std::cerr << "Only triangulated mesh is accepted.\n";
                    exit(1);
                }

                for (int i = 0; i < (int)faceData.size(); i++)
                {
                    int vertNum;

                    ptrFront = strchr(faceData[i], '/');
                    if (ptrFront == NULL)
                    {
                        vertNum = atoi(faceData[i]) - 1;
                        mesh.indexBufferData.push_back(vertNum);
                        ++mesh.numIndices;
                    }
                    else
                    {
                        char* tokFront, * tokRear, * cF;
                        ptrRear = strrchr(faceData[i], '/');
                        tokFront = strtok_s(faceData[i], "/", &cF);
                        vertNum = atoi(tokFront) - 1;

                        if (ptrRear == ptrFront)
                        {
                            mesh.indexBufferData.push_back(vertNum);
                            ++mesh.numIndices;
                        }
                        else
                        {
                            if (ptrRear != ptrFront + 1)
                            {
                                tokRear = strtok_s(NULL, "/", &cF);
                            }

                            tokRear = strtok_s(NULL, "/", &cF);
                            mesh.indexBufferData.push_back(vertNum);
                            ++mesh.numIndices;
                        }
                    }
                }
            }
        }
    }

    if (fp) 
    {
        if (fclose(fp))
        {
            std::cerr << "Failed to close " << fileName << "\n";
            exit(1);
        }
    }


}

