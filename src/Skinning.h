//
// Created by cwkmi on 10/6/2026.
//

#ifndef ANIMATION_HPP
#define ANIMATION_HPP

struct Skin
{
    std::string name;
    std::vector<glm::mat4> inverseBindMatrices;
    std::vector<Node*> joints;
    std::array<vk::Buffer, gMaxFramesInFlight> storageBuffers;
    uint32_t descriptorIndex; //index into descriptorManager
};

struct AnimationSampler
{
    fastgltf::AnimationInterpolation interpolation = fastgltf::AnimationInterpolation::Linear;
    std::vector<float> inputs; //assuming linear time for input (according to gltf spec of what this data means)
    std::vector<glm::vec4> outputsVec4; //assuming animated property (according to gltf spec of what this data means)
};

struct AnimationChannel
{
    //kind of incorrect to assume translation by default, but the initialization will be overrided anyway
    fastgltf::AnimationPath path = fastgltf::AnimationPath::Translation; //property to animate: translation, rotation, scale, or weights (morph targets, not vertex weights [NOT IMPLEMENTED])
    Node* node = nullptr;
    uint32_t samplerIndex = 0;

};

struct Animation
{
    std::string name;
    std::vector<AnimationSampler> samplers;
    std::vector<AnimationChannel> channels;
    float start = std::numeric_limits<float>::max();
    float end = std::numeric_limits<float>::min();
    float currentTime = 0.0f;
};


#endif //ANIMATION_HPP
