#pragma once
#include <DirectXMath.h>

using namespace DirectX;

struct DirectionalLight
{
  XMFLOAT4 AmbientColor;
  XMFLOAT4 DiffuseColor;
  XMFLOAT3 Direction;
  float Intensity;
};

struct PointLight
{
  XMFLOAT4 Color;
  XMFLOAT3 Position;
  float Range;
  float Intensity;
  XMFLOAT3 padding;
};

struct SpotLight
{
  XMFLOAT4 Color;
  XMFLOAT3 Position;
  float Intensity;
  XMFLOAT3 Direction;
  float Range;
  float SpotFalloff;
  XMFLOAT3 padding;
};