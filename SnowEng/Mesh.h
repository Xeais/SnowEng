#pragma once
#include <DirectXMath.h>
#include <d3d11.h>
#include "Vertex.h"
#include "DXCore.h"
#include <fstream>
#include <vector>

class Mesh
{
  public:
  Mesh();
  Mesh(Vertex* vertices, UINT vertexCount, UINT* indices, UINT indexCount, ID3D11Device* device);
  Mesh(const char* file, ID3D11Device* device);
  ~Mesh();

  ID3D11Buffer* GetVertexBuffer();
  ID3D11Buffer* GetIndexBuffer();
  int GetIndexCount();

  void CreateBasicGeometry(Vertex* vertices, UINT vertexCount, UINT* indices, UINT indexCount, ID3D11Device* device);
  void CalculateTangents(Vertex* vertices, UINT vertexCount, UINT* indices, UINT indexCount);

  private:
  ID3D11Buffer* vertexBuffer;
  ID3D11Buffer* indexBuffer;
  int indexCount;
};