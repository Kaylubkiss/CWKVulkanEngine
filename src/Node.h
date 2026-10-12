//
// Created by cwkmi on 10/11/2026.
//

#ifndef NODE_HPP
#define NODE_HPP


struct Node //notice: no pointing to parents. Might be something for later.
{
    glm::mat4 transform; //TODO: remove
    std::optional<size_t> meshIndex;
    std::optional<size_t> skinIndex = 0;
    std::vector<size_t> childrenIndices;

    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::quat rotation{};
    glm::vec3 translation{};

    std::string name;

    Node* parent = nullptr;

    [[nodiscard]] glm::mat4 GetNodeMatrix() const;
    [[nodiscard]] glm::mat4 GetLocalTransformMatrix() const;
};


#endif //NODE_HPP
