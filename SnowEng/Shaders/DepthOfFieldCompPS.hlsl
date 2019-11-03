cbuffer Data : register(b0) {}

struct VertexToPixel
{
  float4 position: SV_POSITION;
  float2 uv: TEXCOORD0;
};

Texture2D Pixels: register(t0);
Texture2D BlurTexture: register(t1);
Texture2D Radius: register(t2);
SamplerState Sampler: register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
  float4 pack = Pixels.Sample(Sampler, input.uv);
  float3 sharp = pack.rgb;
  float radius = Radius.Sample(Sampler, input.uv).r;
  //Bilinear interpolate when reading the blur texture.
  float3 blurred = BlurTexture.Sample(Sampler, input.uv).rgb;

  //Normalize radius.
  float normRadius = (radius * 2.0f - 1.0f); //* 0.25f;

  //Boost the blur factor.
  normRadius = clamp(normRadius * 2.0f, -1.0f, 1.0f);

  float3 result = lerp(sharp, blurred, abs(normRadius));

  return float4(result, 1.0f);
}