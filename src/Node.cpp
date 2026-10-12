//
// Created by cwkmi on 10/12/2026.
//
#include "Node.h"

glm::mat4 Node::GetNodeMatrix() const
{
    glm::mat4 nodeMatrix = GetLocalTransformMatrix();

    Node* parentNode = this->parent;
    while (parentNode)
    {
        nodeMatrix = parentNode->GetLocalTransformMatrix() * nodeMatrix;
        parentNode = parentNode->parent;
    }

    return nodeMatrix;
}

glm::mat4 Node::GetLocalTransformMatrix() const
{
    return glm::translate(glm::mat4(1.f), translation) *
        glm::mat4(rotation) *
        glm::scale(glm::mat4(1.f), scale) * transform;
}
