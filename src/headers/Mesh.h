#pragma once

#include <optional>
#include <string>
#include <vector>

struct Primitive
{
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;

    uint32_t firstVertex = 0;
    uint32_t vertexCount = 0;

    uint32_t textureSetLayoutIndex = 0;
};

struct Mesh
{
    std::string m_name;
    std::vector<Primitive> m_primitives;
    Mesh() = default;
    Mesh( const std::string& name, const std::vector<Primitive>& primitives )
    {
        m_name = name;
        m_primitives = primitives;
    }
};