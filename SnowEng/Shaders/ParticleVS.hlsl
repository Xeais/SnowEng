#include "ParticleIncludes.hlsli"

cbuffer externalData : register(b0)
{
  matrix world;
  matrix view;
  matrix projection;
  float aspectRatio;
  float2 oneOverWidthHeight;
};

StructuredBuffer<Particle> ParticlePool	: register(t0);
StructuredBuffer<ParticleDraw> DrawList	: register(t1);

struct VStoPS
{
  float4 position : SV_POSITION;
  float4 color : COLOR;
  float2 uv	: TEXCOORD;
};

VStoPS main(uint id : SV_VertexID)
{
  //Get id info.
  uint particleId = id / 4;
  uint cornerId = id % 4;

  //Look up the draw info, then this particle.
  ParticleDraw draw = DrawList.Load(particleId);
  Particle particle = ParticlePool.Load(draw.Index);

  //Offsets for smaller triangles:
  float2 offsets[4];
  offsets[0] = float2(-1.0f, -1.0f);  //BL
  offsets[1] = float2(-1.0f, +1.0f);  //TL
  offsets[2] = float2(+1.0f, -1.0f);  //BR
  offsets[3] = float2(+1.0f, +1.0f);  //TR

  //Output struct:
  VStoPS output;

  //Calculate world view and projection matrix.
  matrix wvp = mul(mul(world, view), projection);
  output.position = mul(float4(particle.Position, 1.0f), wvp);

  //Depth (change this to 1.0 for no size change when getting close)
  float depthChange = output.position.z / output.position.w;

  //Adjust based on depth:
  offsets[cornerId].y *= aspectRatio;
  output.position.xy += offsets[cornerId] * depthChange * particle.Size;
  output.color = particle.Color;
  output.uv = saturate(offsets[cornerId]);

  return output;
}