#version 450

#extension GL_EXT_buffer_reference : require

layout(location = 0) out vec4 fragColor;

struct Vertex {
  vec3 position;
  float uv_x;
  vec3 normal;
  float uv_y;
  vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
  Vertex vertices[];
};

layout( push_constant ) uniform Push {
    mat4 modelMatrix;
    mat4 view; 
    mat4 projection;
    VertexBuffer vertexBuffer;
    vec4 id;
} push;

void main() {
    Vertex v = push.vertexBuffer.vertices[gl_VertexIndex];
    vec4 positionWorld = push.modelMatrix * vec4(v.position, 1.0);
    gl_Position = push.projection * push.view * positionWorld;
    
    fragColor = push.id;
}
