#include "Mesh.h"
#include <iostream>

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
                        mesh.vertexBuffer.push_back(v);
                        ++mesh.numVertices;
                    }
                    else 
                    {
                        mesh.vertexBuffer[posID].pos = v.pos;
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

                if (faceData.size() > 3) //assume last line is newline.
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
                        mesh.indexBuffer.push_back(vertNum);
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
                            mesh.indexBuffer.push_back(vertNum);
                            ++mesh.numIndices;
                        }
                        else
                        {
                            if (ptrRear != ptrFront + 1)
                            {
                                tokRear = strtok_s(NULL, "/", &cF);
                            }

                            tokRear = strtok_s(NULL, "/", &cF);
                            mesh.indexBuffer.push_back(vertNum);
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