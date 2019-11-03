#include "ParticleIncludes.hlsli"

StructuredBuffer<Particle> ParticlePool	: register(t0);
StructuredBuffer<ParticleDraw> DrawList	: register(t1);

struct VStoGS
{
	float3 position: POSITION;
	float  size: SIZE;
	float4 color: COLOR;
};

VStoGS main(uint id : SV_VertexID)
{
	//Look up the draw info, then this particle.
	ParticleDraw draw = DrawList.Load(id);
	Particle particle = ParticlePool.Load(draw.Index);

	//Pass through.
	VStoGS output;
	output.position = particle.Position;
	output.size = particle.Size;
	output.color = particle.Color;

	return output;
}