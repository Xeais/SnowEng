#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include "SimpleShader.h"
#include "Camera.h"

/* Struct with particle info 
 * Emitter with particle behavior options
 * Particle system - CPU or GPU */

struct Particle
{
  DirectX::XMFLOAT4 Color;
  float Age;
  DirectX::XMFLOAT3 Position;
  float Size;
  DirectX::XMFLOAT3 Velocity;
  float Alive;
  DirectX::XMFLOAT3 padding;
};

struct ParticleSort
{
  unsigned int index;
  float distanceSq;
  //float padding[2];
};

class Emitter
{
  public:
  Emitter(unsigned int maxParticles, float emissionRate, float lifetime, ID3D11Device* device, ID3D11DeviceContext* context, SimpleComputeShader* deadListInitCS, SimpleComputeShader* emitCS,
          SimpleComputeShader* updateCS, SimpleComputeShader* copyDrawCountCS, SimpleVertexShader* particleDefaultVS, SimpleVertexShader* particleVS, SimplePixelShader* particlePS);
  ~Emitter();

  void Update(float dt, float tt);
  void Draw(Camera* camera, float aspectRatio, float width, float height, bool additive);

  private:
  //Emitter settings:
  unsigned int maxParticles;
  float lifetime;
  float emissionRate;
  float timeBetweenEmit;
  float emitTimeCounter;

  ID3D11Buffer* indexBuffer;
  ID3D11BlendState* additiveBlend;
  ID3D11DepthStencilState* depthWriteOff;

  ID3D11DeviceContext* context;
  SimpleComputeShader* emitCS;
  SimpleComputeShader* updateCS;
  SimpleComputeShader* copyDrawCountCS;
  SimpleVertexShader* particleDefaultVS;
  SimpleVertexShader* particleVS;
  SimplePixelShader* particlePS;

  ID3D11Buffer* drawArgsBuffer;

  ID3D11UnorderedAccessView* particlePoolUAV;
  ID3D11UnorderedAccessView* particleDeadUAV;
  ID3D11UnorderedAccessView* particleDrawUAV;
  ID3D11UnorderedAccessView* drawArgsUAV;
  ID3D11ShaderResourceView* particlePoolSRV;
  ID3D11ShaderResourceView* particleDrawSRV;
};