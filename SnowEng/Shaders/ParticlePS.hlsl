#include "ParticleIncludes.hlsli"

struct GStoPS
{
  float4 position : SV_POSITION;
  float4 color : COLOR;
  float2 uv	: TEXCOORD0;
};

float4 main(GStoPS input) : SV_TARGET
{
  //Convert uv to -1 and subsequently to 1.
  input.uv = input.uv * 2.0f - 1.0f;

  //Distance from center:
  float fade = saturate(distance(float2(0.0f, 0.0f), input.uv));
  float3 color = lerp(input.color.rgb, float3(0.0f, 0.0f, 0.0f), fade * fade);

  return float4(color, 1.0f);
}