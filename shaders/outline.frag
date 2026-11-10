#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform usampler2D textureID;

layout(push_constant) uniform Push {
  uint selectedID;
} push;

// just leaving these here in case i need to do edge detection

// Sobel X Kernel
const float sobelX[9] = float[9](
    -1.0, 0.0, 1.0,
    -2.0, 0.0, 2.0,
    -1.0, 0.0, 1.0
  );

// Sobel Y Kernel
const float sobelY[9] = float[9](
    -1.0, -2.0, -1.0,
    0.0, 0.0, 0.0,
    1.0, 2.0, 1.0
  );

const int line_thickness = 4;

void main() {
  vec4 uv_center = texelFetch(textureID, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);
  bool shouldBeColored = false;
  for (int i = -1; i < line_thickness; i++) {
    for (int j = -1; j < line_thickness; j++) {
      float x_coord = max(gl_FragCoord.x + i, 0);
      float y_coord = max(gl_FragCoord.y + j, 0);
      vec4 uv = texelFetch(textureID, ivec2(x_coord, y_coord), 0);
      if (uv.x != push.selectedID && uv_center.x == push.selectedID) {
        shouldBeColored = true;
        // leaving out break to prevent branch divergence
      }
    }
  }

  if (shouldBeColored) {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
  } else {
    discard;
  }
}
