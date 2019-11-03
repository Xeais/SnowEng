struct VertexToPixel
{
  float4 position: SV_POSITION;
  float3 sampleDir: TEXCOORD;
};

//Texture-related variables:
TextureCube SkyTexture : register(t0);
SamplerState BasicSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
  return SkyTexture.Sample(BasicSampler, input.sampleDir);
}