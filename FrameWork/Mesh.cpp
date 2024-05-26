#include "Mesh.h"
#include <fstream>
#include <sstream>
#include <algorithm>

Object::Object(const char* fileName, MeshType type)
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

  
    if (type == M_CUBE) 
    {
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

    }

    size_t sizeOfVertexBuffer = (sizeof(Vertex) * this->mMesh.vertexBufferData.size());
    this->vertex = Buffer(sizeOfVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->mMesh.vertexBufferData.data());
   /* this->vertex.RecordData();
    this->vertex.CopyData();
    this->vertex.StopRecordData();*/

    size_t sizeOfIndexBuffer = (sizeof(int) * this->mMesh.indexBufferData.size());
    this->index = Buffer(sizeOfIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, this->mMesh.indexBufferData.data());
  /*  this->index.RecordData();
    this->index.CopyData();
    this->index.StopRecordData();*/



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

//boiler plate from framework of graphics class
void LoadMeshOBJ(const std::string& path, Mesh& mesh) 
{

    std::ifstream file;

    file.open(path, std::ios::in);

    if (!file) {
        std::cout << "had an error opening file!\n";
        return;
    }
    std::string line;

    while (std::getline(file, line))
    {
        std::istringstream lineSStream(line);
        std::string type;
        lineSStream >> type;

        if (type == "v") {

            float x, y, z;

            lineSStream >> x >> y >> z;

            Vertex vert = { glm::vec3(x,y,z), glm::vec3(0), glm::vec2(0) };

            mesh.vertexBufferData.push_back(vert);
        }
        else if (type == "f") //this will only work for files that have delimiters '/' for their faces.
        {
            std::string str_f;
            while (lineSStream >> str_f)
            {
                std::istringstream ref(str_f);
                std::string vStr;
                std::getline(ref, vStr, '/');
                int v = atoi(vStr.c_str()) - 1;
                mesh.indexBufferData.push_back(v);
                std::getline(ref, vStr, '/');
                std::getline(ref, vStr, '/');
            }

        }
    }

    file.close();
}


