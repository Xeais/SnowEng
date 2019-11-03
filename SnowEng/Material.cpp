#include "Material.h"

Material::Material(SimpleVertexShader* VertexShader, SimplePixelShader* PixelShader, ID3D11ShaderResourceView* Albedo, ID3D11ShaderResourceView* Normal, 
                   ID3D11ShaderResourceView* Roughness, ID3D11ShaderResourceView* Metal, ID3D11SamplerState* Sampler)
{
  vertexShader = VertexShader;
  pixelShader = PixelShader;
  albedoSRV = Albedo;
  normalSRV = Normal;
  roughnessSRV = Roughness;
  metalSRV = Metal;
  sampler = Sampler;
}

Material::~Material() {}

SimpleVertexShader* Material::GetVertexShader() {return vertexShader;}

SimplePixelShader* Material::GetPixelShader() {return pixelShader;}

ID3D11ShaderResourceView* Material::GetAlbedoSRV() {return albedoSRV;}

ID3D11SamplerState* Material::GetSampler() {return sampler;}

ID3D11ShaderResourceView* Material::GetNormalSRV() {return normalSRV;}

ID3D11ShaderResourceView* Material::GetRoughnessSRV() {return roughnessSRV;}

ID3D11ShaderResourceView* Material::GetMetalSRV() {return metalSRV;}