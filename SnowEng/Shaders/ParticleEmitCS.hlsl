#include "ParticleIncludes.hlsli"
#include "SimplexNoise.hlsli"

cbuffer ExternalData : register(b0)
{
  float TotalTime;
  int EmitCount;
  int MaxParticles;
  int GridSize;
}

RWStructuredBuffer<Particle> ParticlePool : register(u0);
ConsumeStructuredBuffer<uint> DeadList : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
  if(id.x >= (uint)EmitCount)
    return;

  uint emitIndex = DeadList.Consume();

  //Replace grid with sphere.
  float3 gridPos;
  uint gridIndex = emitIndex;
  gridPos.x = gridIndex % (GridSize + 1);
  gridIndex /= (GridSize + 1);
  gridPos.y = gridIndex % (GridSize + 1);
  gridIndex /= (GridSize + 1);
  gridPos.z = gridIndex;

  Particle emitParticle = ParticlePool.Load(emitIndex);

  //All of these magical numbers (obtained: See "SimplexNoise.hlsli") combined in their correspondent formulae, give a fairly convincing snow flurry.
  float3 noise = (frac(sin(dot(gridPos + id, float3(12.9898f, 78.233f, 45.7785f) * 2.0f)) * 43758.5453f));
  emitParticle.Color = float4(1.1f, 1.1f, 1.1f, 1.1f);
  emitParticle.Age = 0.0f;
  emitParticle.Position = /* float3(noise.x * 30.0f + 1.1f, noise.y * 10.0f + 5.0f, noise.z * 100.0f + 2.0f); */ gridPos / 10.0f - float3(GridSize / 20.0f, GridSize / 20.0f, -GridSize / 10.0f);
  emitParticle.Size = 0.05f;
  emitParticle.Velocity = float3(0.0f, 0.0f, 0.0f);
  emitParticle.Alive = 1.0f;

  ParticlePool[emitIndex] = emitParticle;
}