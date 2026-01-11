#version 450

// layout binding means that this uniform comes binding 0 of descriptor set
//
//  VkDescriptorSetLayoutBinding{
//      .binding = 0,
//      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//  }
//
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// layout location - vertex data
//  VkVertexInputAttributeDescription{
//    .location = 0,
//    .binding  = 0,
//    .format   = VK_FORMAT_R32G32_SFLOAT,
//    .offset   = offsetof(Vertex, pos)
//  }
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}