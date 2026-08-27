#ifndef OBJECTPARSER_H
#define OBJECTPARSER_H

#include <nlohmann/json.hpp>
#include <fstream>
#include <istream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using json = nlohmann::json;
using namespace std;

static_assert(sizeof(array<array<float, 4>, 4>) == sizeof(glm::mat4));

inline void _TEST_ReadObject(const std::string& fileName, ObjectCreateInfo& objCI)
{

    std::ifstream i(fileName);
    if (i.is_open() == false)
    {
        std::cerr << "Couldn't read file " << fileName << std::endl;
        return;
    }

    json data;
    i >> data;

    objCI = data.get<ObjectCreateInfo>();
}

inline void _TEST_WriteObject(const std::string& fileName, const ObjectCreateInfo& objCI)
{
    json data = objCI;

    std::ofstream output(fileName);

    if (output.is_open() == false)
    {
        std::cerr << "couldn't open ofstream for scene: " << fileName << std::endl;
        return;
    }

    output << std::setw(4) << data << '\n';
}


#endif