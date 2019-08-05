
#version 150

#ifndef AO_BLUR
#define AO_BLUR 1
#endif

uniform sampler2DArray texResultsArray;

out vec4 out_Color;

//----------------------------------------------------------------------------------

void main() {
  ivec2 FullResPos = ivec2(gl_FragCoord.xy);
  ivec2 Offset = FullResPos & 3;
  int SliceId = Offset.y * 4 + Offset.x;
  ivec2 QuarterResPos = FullResPos >> 2;
  
#if AO_BLUR
  out_Color = vec4(texelFetch( texResultsArray, ivec3(QuarterResPos, SliceId), 0).xy,0,0);
#else
  out_Color = vec4(texelFetch( texResultsArray, ivec3(QuarterResPos, SliceId), 0).x);
#endif
  
}
