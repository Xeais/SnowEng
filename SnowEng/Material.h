#pragma once
#include "SimpleShader.h"

class Material
{
  public:
  Material(SimpleVertexShader* vertexShader, SimplePixelShader* pixelShader, ID3D11ShaderResourceView* albedo, ID3D11ShaderResourceView* normal, 
           ID3D11ShaderResourceView* roughness, ID3D11ShaderResourceView* metal, ID3D11SamplerState* sampler);
  ~Material();

  SimpleVertexShader* GetVertexShader();
  SimplePixelShader* GetPixelShader();

  ID3D11ShaderResourceView* GetAlbedoSRV();
  ID3D11ShaderResourceView* GetNormalSRV();
  ID3D11ShaderResourceView* GetRoughnessSRV();
  ID3D11ShaderResourceView* GetMetalSRV();
  ID3D11SamplerState* GetSampler();

  private:
  SimpleVertexShader* vertexShader;
  SimplePixelShader* pixelShader;

  //ID3D11ShaderResourceView* textureShader;
  ID3D11ShaderResourceView* albedoSRV;
  ID3D11ShaderResourceView* normalSRV;
  ID3D11ShaderResourceView* roughnessSRV;
  ID3D11ShaderResourceView* metalSRV;
  ID3D11SamplerState* sampler;
};