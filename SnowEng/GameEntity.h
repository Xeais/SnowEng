#pragma once
#include <DirectXMath.h>
#include "Mesh.h"
#include "Material.h"
#include "SimpleShader.h"

using namespace DirectX;

class GameEntity
{
  public:
  GameEntity(Mesh* mesh, Material* newMat);
  ~GameEntity();

  SimpleVertexShader* vertexShader;
  SimplePixelShader* pixelShader;

  void SetTranslation(XMFLOAT3 setPos);
  void SetScale(XMFLOAT3 setScale);
  void SetRotation(float x, float y, float z);

  void Move(float x, float y, float z);
  void Rotate(float x, float y, float z);

  Mesh* GetMesh();
  Material* GetMaterial();
  XMFLOAT4X4 GetMatrix();
  void UpdateWorldMatrix();
  void PrepareMaterial(XMFLOAT4X4 viewMatrix, XMFLOAT4X4 projMatrix, ID3D11SamplerState* objSampler, ID3D11ShaderResourceView* skyIR, 
                       ID3D11ShaderResourceView* skyPrefilter, ID3D11ShaderResourceView* brdf);

  private:
  XMFLOAT4X4 worldMatrix;
  XMFLOAT3 position;
  XMFLOAT3 rotation;
  XMFLOAT3 scale;
  Mesh* meshObject;
  Material* materialObject;
};