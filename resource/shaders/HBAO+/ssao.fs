#version 150

in vec2 texCoord;

uniform sampler2D texLinearDepth;
uniform sampler2D texRandom;

uniform vec3 samples[64];
uniform mat4 projection;
uniform vec4 projInfo;
uniform int  projOrtho;
uniform vec2 InvFullResolution;
uniform vec2 viewerSize;

const int kernelSize = 64;
uniform float radius;
uniform float bias;

out vec4 out_Color;


vec3 UVToView(vec2 uv, float eye_z)
{
    return vec3((uv * projInfo.xy + projInfo.zw) * (projOrtho != 0 ? 1. : eye_z), eye_z);
}


vec3 FetchViewPos(vec2 UV)
{
  float ViewDepth = textureLod(texLinearDepth,UV,0).x;
  return UVToView(UV, ViewDepth);
}


vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
{
  vec3 V1 = Pr - P;
  vec3 V2 = P - Pl;
  return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

vec3 ReconstructNormal(vec2 UV, vec3 P)
{
  vec3 Pr = FetchViewPos(UV + vec2(InvFullResolution.x, 0));
  vec3 Pl = FetchViewPos(UV + vec2(-InvFullResolution.x, 0));
  vec3 Pt = FetchViewPos(UV + vec2(0, InvFullResolution.y));
  vec3 Pb = FetchViewPos(UV + vec2(0, -InvFullResolution.y));
  return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
}



void main()
{	
	vec2 uv = texCoord;
        vec3 fragPos = FetchViewPos(uv);
	vec3 normal = -ReconstructNormal(uv, fragPos);
        vec3 randomVec = texture(texRandom, uv * viewerSize / 4.0).xyz; //* 2.0 - 1.0;
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));	
	vec3 bitangent = cross(tangent, normal);
	mat3 TBN = mat3(tangent, bitangent, normal);

	float occlusion = 0.0;
	for(int i = 0; i < kernelSize; ++i)
	{
                vec3 samp = TBN * samples[i];
                samp = fragPos + samp * radius;

                // project sample position (to sample texture) (to get position on screen/texture)
                vec4 offset = vec4(samp, 1.0);
                offset = projection * offset; // from view to clip-space
                offset.xyz /= offset.w; // perspective divide
                offset.xyz = (projOrtho != 0 ? 1. : -1.)  * offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

                float sampleDepth = texture(texLinearDepth, offset.xy).r;

                // range check & accumulate:
                float rangeCheck= abs(fragPos.z - sampleDepth) < radius ? 1.0 : 0.0;
                occlusion += (sampleDepth <= samp.z - bias ? 1.0 : 0.0) * rangeCheck;
	}
	occlusion = 1.0 - (occlusion / kernelSize);
        out_Color = vec4(occlusion);
}
