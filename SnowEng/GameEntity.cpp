#include "GameEntity.h"

GameEntity::GameEntity(Mesh* mesh, Material* newMat)
{
  meshObject = mesh;
  materialObject = newMat;
  SetTranslation(XMFLOAT3(0.0f, 0.0f, 0.0f));
  SetScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
  //rotation = XMFLOAT3(1.0f, 1.0f, 1.0f);
  //Rotate(1.0f, 1.0f, 1.0f);
  SetRotation(0.0f, 0.0f, 0.0f);
  GetMatrix();
}

GameEntity::~GameEntity() {}

void GameEntity::SetTranslation(XMFLOAT3 setPos) {position = setPos;}

void GameEntity::SetScale(XMFLOAT3 setScale) {scale = setScale;}

void GameEntity::SetRotation(float x, float y, float z) {rotation = XMFLOAT3(x, y, z);}

void GameEntity::Move(float x, float y, float z)
{
  position.x += x;
  position.y += y;
  position.z += z;
}

void GameEntity::Rotate(float x, float y, float z)
{
  rotation.x += x;
  rotation.y += y;
  rotation.z += z;
}

Mesh* GameEntity::GetMesh() {return meshObject;}

Material* GameEntity::GetMaterial() {return materialObject;}

XMFLOAT4X4 GameEntity::GetMatrix() //Returns the world matrix. 
{
  //Coverting to vectors:
  XMVECTOR vPosition = XMLoadFloat3(&position);
  XMVECTOR vRotation = XMLoadFloat3(&rotation);
  XMVECTOR vScale = XMLoadFloat3(&scale);

  //Converting to matrices:
  XMMATRIX mPosition = XMMatrixTranslationFromVector(vPosition);
  XMMATRIX mRotation = XMMatrixRotationRollPitchYawFromVector(vRotation);
  XMMATRIX mScale = XMMatrixScalingFromVector(vScale);

  XMMATRIX wake = XMMatrixIdentity();

  //Calculate world matrix.
  XMMATRIX world = mScale * mRotation * mPosition;

  //Store the world matrix.
  XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(world));

  return worldMatrix;
}

void GameEntity::UpdateWorldMatrix()
{
  XMMATRIX trans = XMMatrixTranslation(position.x, position.y, position.z);
  XMMATRIX rotX = XMMatrixRotationX(rotation.x);
  XMMATRIX rotY = XMMatrixRotationY(rotation.y);
  XMMATRIX rotZ = XMMatrixRotationZ(rotation.z);
  XMMATRIX sc = XMMatrixScaling(scale.x, scale.y, scale.z);

  XMMATRIX total = sc * rotZ * rotY * rotX * trans;
  XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(total));
}

void GameEntity::PrepareMaterial(XMFLOAT4X4 viewMatrix, XMFLOAT4X4 projMatrix, ID3D11SamplerState* objSampler, ID3D11ShaderResourceView* skyIR, 
                                 ID3D11ShaderResourceView* skyPrefilter, ID3D11ShaderResourceView* brdf)
{
  auto vertexShader = materialObject->GetVertexShader();
  auto pixelShader = materialObject->GetPixelShader();

  //First SRV, then sample it:
  pixelShader->SetShaderResourceView("AlbedoTexture", materialObject->GetAlbedoSRV());
  pixelShader->SetShaderResourceView("NormalTexture", materialObject->GetNormalSRV());
  pixelShader->SetShaderResourceView("RoughnessTexture", materialObject->GetRoughnessSRV());
  pixelShader->SetShaderResourceView("MetalTexture", materialObject->GetMetalSRV());

  pixelShader->SetShaderResourceView("skyIrradianceTexture", skyIR);
  pixelShader->SetShaderResourceView("skyPrefilterTexture", skyPrefilter);
  pixelShader->SetShaderResourceView("brdfLookUpTexture", brdf);

  pixelShader->SetSamplerState("BasicSampler", objSampler);

  vertexShader->SetMatrix4x4("World", GetMatrix());
  vertexShader->SetMatrix4x4("View", viewMatrix);
  vertexShader->SetMatrix4x4("Proj", projMatrix);

  vertexShader->CopyAllBufferData();
  pixelShader->CopyAllBufferData();

  vertexShader->SetShader();
  pixelShader->SetShader();
}