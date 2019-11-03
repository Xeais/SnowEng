#include "Lighting.hlsli"

/* 
struct DirectionalLight
{
	float4 AmbientColor;
	float4 DiffuseColor;
	float3 Direction;
	float Intensity;
}; 
*/

cbuffer externalData : register(b0)
{
  DirectionalLight light1;
  DirectionalLight light2;
  PointLight light3;
  SpotLight light4;

  float3 CameraPosition;
};

//Resources for the texture:
Texture2D AlbedoTexture	: register(t0);
Texture2D NormalTexture	: register(t1);
Texture2D RoughnessTexture : register(t2);
Texture2D MetalTexture : register(t3);

TextureCube skyIrradianceTexture : register(t4);
Texture2D brdfLookUpTexture : register(t5);
TextureCube skyPrefilterTexture : register(t6);

SamplerState BasicSampler	: register(s0);

struct VertexToPixel
{
  float4 position: SV_POSITION;
  float3 normal: NORMAL;
  float2 uv: TEXCOORD;
  float3 tangent: TANGENT;
  float3 worldPos: POSITION;
  float linearZ: LINEARZ;
};

float PackFloat(float linearZ, float farZ) {return linearZ / farZ;}

float3 calculateNormalFromMap(float2 uv, float3 normal, float3 tangent)
{
  float3 normalFromTexture = NormalTexture.Sample(BasicSampler, uv).xyz;
  float3 unpackedNormal = normalFromTexture * 2.0f - 1.0f;
  float3 N = normal;
  float3 T = normalize(tangent - N * dot(tangent, N));
  float3 B = cross(N, T);
  float3x3 TBN = float3x3(T, B, N);
  return normalize(mul(unpackedNormal, TBN));
}

float3 PrefilteredColor(float3 viewDir, float3 normal, float roughness)
{
  const float MAX_REF_LOD = 8.0f;
  float3 R = reflect(-viewDir, normal);
  return skyPrefilterTexture.SampleLevel(BasicSampler, R, roughness * MAX_REF_LOD).rgb;
}

float2 BrdfLUT(float3 normal, float3 viewDir, float roughness)
{
  float NdotV = dot(normal, viewDir);
  NdotV = max(NdotV, 0.0f);
  float2 uv = float2(NdotV, roughness);
  return brdfLookUpTexture.Sample(BasicSampler, uv).rg;
}

float4 main(VertexToPixel input) : SV_TARGET
{
  input.normal = normalize(input.normal);
  input.tangent = normalize(input.tangent);
  float4 surfaceColor = AlbedoTexture.Sample(BasicSampler, input.uv);

  input.normal = calculateNormalFromMap(input.uv, input.normal, input.tangent);

  //Sample the roughness map.
  float roughness = RoughnessTexture.Sample(BasicSampler, input.uv).r;
  //roughness = lerp(0.2f, roughness, 1.0f);

  //Sample the metal map.
  float metalness = MetalTexture.Sample(BasicSampler, input.uv).r;
  //metalness = lerp(0.0f, metalness, 1.0f);

  //Specular color: Assuming albedo texture is actually holding specular color if metal is exactly one.
  float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metalness);
  //float3 specColor = F0_NON_METAL.rrr;

  //float3 lightOne = surfaceColor.rgb * (light1.DiffuseColor * dirNdotL + light1.AmbientColor);
  //float3 lightTwo = surfaceColor.rgb * (light2.DiffuseColor * dirNdotL2 + light2.AmbientColor);
  //Total color for this pixel:
  float3 totalColor = float3(0.0f, 0.0f, 0.0f);

  //IBL calculations:
  float3 viewDir = normalize(CameraPosition - input.worldPos);
  float3 prefilter = PrefilteredColor(viewDir, input.normal, roughness);
  float2 brdf = BrdfLUT(input.normal, viewDir, roughness);
  float3 irradiance = skyIrradianceTexture.Sample(BasicSampler, input.normal).rgb;

  float3 dirPBR = DirLightPBR(light1, input.normal, input.worldPos, CameraPosition, roughness, metalness, surfaceColor.rgb, specColor, irradiance, prefilter, brdf);
  //float3 dirPBR = DirLightPBR(light1, input.normal, input.worldPos, CameraPosition, roughness, metalness, surfaceColor.rgb, specColor);
  float3 pointPBR = PointLightPBR(light3, input.normal, input.worldPos, CameraPosition, roughness, metalness, surfaceColor.rgb, specColor);
  float3 spotPBR = SpotLightPBR(light4, input.normal, input.worldPos, CameraPosition, roughness, metalness, surfaceColor.rgb, specColor);

  totalColor = dirPBR + pointPBR + spotPBR;

  float packedValue = PackFloat(input.linearZ, 100.0f);
  //return light2.DiffuseColor * dirNdotL2 + light.AmbientColor;
  float3 gammaCorrected = lerp(totalColor, pow(abs(totalColor), 1.0f / 2.2f), 0.4f);
  return float4(gammaCorrected, packedValue);
}