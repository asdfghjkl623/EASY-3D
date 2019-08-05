#version 150

//Do NOT remove or edit this line
//INSERT

#ifndef DEPTHLINEARIZE_MSAA
#define DEPTHLINEARIZE_MSAA 0
#endif

uniform vec4 clipInfo; // z_n * z_f,  z_n - z_f,  z_f, perspective = 1 : 0

#if DEPTHLINEARIZE_MSAA
uniform int sampleIndex;
uniform sampler2DMS inputTexture;
#else
uniform sampler2D inputTexture;
#endif

out float out_Color;

float reconstructCSZ(float d, vec4 clipInfo) {
  if (clipInfo[3] != 0) {
    return (clipInfo[0] / (clipInfo[1] * d + clipInfo[2]));
  }
  else {
    return (clipInfo[1]+clipInfo[2] - d * clipInfo[1]);
  }
}

void main() {
#if DEPTHLINEARIZE_MSAA
  float depth = texelFetch(inputTexture, ivec2(gl_FragCoord.xy), sampleIndex).x;
#else
  float depth = texelFetch(inputTexture, ivec2(gl_FragCoord.xy), 0).x;
#endif

  out_Color = reconstructCSZ(depth, clipInfo);
}
