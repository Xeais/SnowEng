#pragma once
#include "Mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

using namespace DirectX;

Mesh::Mesh() : indexBuffer(0), indexCount(0), vertexBuffer(0) {}

Mesh::Mesh(const char* objFile, ID3D11Device* device)
{
  //File input object:
  //std::ifstream obj(objFile);

  /* Check for a successful opening.
   * if(!obj.is_open())
   *   return; */

  //Variables used while reading the file:
  std::vector<XMFLOAT3> positions; //Positions from the file
  std::vector<XMFLOAT3> normals;   //Normals from the file
  std::vector<XMFLOAT2> uvs;       //UVs from the file
  std::vector<Vertex> verts;       //Vertices, we're assembling.
  std::vector<UINT> indices;       //Indices of these vertices
  unsigned int vertCounter = 0;    //Count of vertices/indices

  std::vector<Vertex> vertices;    //Vertices, we're assembling.
  std::vector<UINT> indexVals;     //Indices of these vertices
  std::string warnings;
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;
  bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, &warnings, objFile);

  for(size_t s = 0; s < shapes.size(); ++s)
  {
    //Loop over faces (polygon):
    size_t indexOffset = 0;
    for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f)
    {
      int fv = shapes[s].mesh.num_face_vertices[f];

      //Loop over vertices in the face.
      for(size_t v = 0; v < fv; ++v)
      {
        //Access to vertex:
        tinyobj::index_t idx = shapes[s].mesh.indices[indexOffset + v];
        tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
        tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
        tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
        tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
        tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
        tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
        tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
        tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
        Vertex vertex;
        vertex.Position = XMFLOAT3(vx, vy, vz);
        positions.push_back(vertex.Position);
        vertex.Normal = XMFLOAT3(nx, ny, nz);
        vertex.UV = XMFLOAT2(tx, ty);
        vertices.push_back(vertex);

        indexVals.push_back(static_cast<UINT>(indexOffset) + static_cast<UINT>(v));
        /* Optional: vertex colors
         * tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
         * tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
         *  tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2]; */
      }

      indexOffset += fv;

      //Per-face material:
      shapes[s].mesh.material_ids[f];
    }
  }

  /* Still data left?
   * while(obj.good())
   * {
   *   //Get the line (100 characters should be more than enough!).
   *   obj.getline(chars, 100);
   *
   *   //Check the type of line.
   *   if(chars[0] == 'v' && chars[1] == 'n')
   *   {
   *     //Read the three numbers directly into an XMFLOAT3.
   * 	 	 XMFLOAT3 norm;
   * 	 	 sscanf_s(chars, "vn %f %f %f", &norm.x, &norm.y, &norm.z);
   *
   *     //Add to the list of normals.
   *     normals.push_back(norm);
  -*   }
   *   else if(chars[0] == 'v' && chars[1] == 't')
   *   {
   *     //Read the two numbers directly into an XMFLOAT2.
   *     XMFLOAT2 uv;
   * 		 sscanf_s(chars, "vt %f %f", &uv.x, &uv.y);
   *
   *		 //Add to the list of uv's.
   * 		 uvs.push_back(uv);
   *   }
   *   else if(chars[0] == 'v')
   *   {
   *     //Read the three numbers directly into an XMFLOAT3.
   *     XMFLOAT3 pos;
   *  	 sscanf_s(chars, "v %f %f %f", &pos.x, &pos.y, &pos.z);
   *
   *     //Add to the positions.
   *   	 positions.push_back(pos);
   *   }
   *   else if(chars[0] == 'f')
   *   {
   *     //Read the face indices into an array.
   *		 unsigned int i[12];
   *		 int facesRead = sscanf_s(chars, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",0&i[0], &i[1], &i[2], 
   *                              &i[3], &i[4], &i[5], &i[6], &i[7], &i[8], &i[9], &i[10], &i[11]);
   *
   *     //- Create the vertices by looking up corresponding data from vectors.
   *     //- Obj-File-indices are 1-based, so they need to be adusted.
   *		 Vertex v1;
   *		 v1.Position = positions[i[0] - 1];
   *		 v1.UV = uvs[i[1] - 1];
   *	   v1.Normal = normals[i[2] - 1];
   *
   *		 Vertex v2;
   *		 v2.Position = positions[i[3] - 1];
   *     v2.UV = uvs[i[4] - 1];
   *     v2.Normal = normals[i[5] - 1];
   *
   *		 Vertex v3;
   *		 v3.Position = positions[i[6] - 1];
   *		 v3.UV = uvs[i[7] - 1];
   *     v3.Normal = normals[i[8] - 1];
   */
   /*    The model is most likely in right-handed space, especially if it came from Maya.
   *     Convert it to left-handed space for DirectX. This means we need to:
   *     - Invert the z-position.
   *     - Invert the normal's z-part.
   *     - Flip the winding order.
   *     We also need to flip the UV-coordinate, since DirectX defines (0, 0) as the top left of the texture, whereas many
   *     3D-modeling-packages use the bottom left as origin.
   */
   /*    //Flip the UVs, since they're probably upside down.
   *		 v1.UV.y = 1.0f - v1.UV.y;
   *		 v2.UV.y = 1.0f - v2.UV.y;
   *		 v3.UV.y = 1.0f - v3.UV.y;
   *
   *		 //Flip z (LH vs. RH).
   *		 v1.Position.z *= -1.0f;
   *		 v2.Position.z *= -1.0f;
   *		 v3.Position.z *= -1.0f;
   *
   *		 //Flip z-normal.
   *		 v1.Normal.z *= -1.0f;
   *		 v2.Normal.z *= -1.0f;
   *		 v3.Normal.z *= -1.0f;
   *
   *		 //Add the vertices to the vector (flipping the winding order).
   *		 verts.push_back(v1);
   *		 verts.push_back(v3);
   *		 verts.push_back(v2);
   *
   *		 //Add three more indices.
   *		 indices.push_back(vertCounter); 
   *     ++vertCounter;
   *		 indices.push_back(vertCounter); 
   *     ++vertCounter;
   *		 indices.push_back(vertCounter);
   *     ++vertCounter;
   *
   *     //Was there a fourth face?
   *     if(facesRead == 12)
   *     {
   *			 //Create the last vertex.
   *			 Vertex v4;
   *			 v4.Position = positions[i[9] - 1];
   *			 v4.UV = uvs[i[10] - 1];
   *			 v4.Normal = normals[i[11] - 1];
   *
   *			 //Flip the UV, z-pos and normal.
   *			 v4.UV.y = 1.0f - v4.UV.y;
   *			 v4.Position.z *= -1.0f;
   *			 v4.Normal.z *= -1.0f;
   *
   *			 //Add a whole triangle (flipping the winding order).
   *			 verts.push_back(v1);
   *			 verts.push_back(v4);
   *			 verts.push_back(v3);
   *
   *       //Add three more indices.
   *       indices.push_back(vertCounter); 
   *       ++vertCounter;
   *       indices.push_back(vertCounter); 
   *       ++vertCounter;
   *			 indices.push_back(vertCounter); 
   *       ++vertCounter;
   *     }
   *   }
   * }
   *
   * //Close the file and create the actual buffers.
   * obj.close(); */
  CreateBasicGeometry(vertices.data(), static_cast<UINT>(vertices.size()), indexVals.data(), static_cast<UINT>(indexVals.size()), device);
}

void Mesh::CreateBasicGeometry(Vertex* vertices, UINT vertexCount, UINT* indices, UINT IndexCount, ID3D11Device* device)
{
  CalculateTangents(vertices, vertexCount, indices, IndexCount);
  //Initializing is apparently important!
  vertexBuffer = nullptr;
  indexBuffer = nullptr;
  indexCount = IndexCount;

  //Creating vertex buffer description:
  D3D11_BUFFER_DESC vbd;
  vbd.Usage = D3D11_USAGE_IMMUTABLE;
  vbd.ByteWidth = sizeof(Vertex) * vertexCount; //Number of vertices: 3
  vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  vbd.CPUAccessFlags = 0;
  vbd.MiscFlags = 0;
  vbd.StructureByteStride = 0;

  /* Create the proper struct to hold the initial vertex data.
   * This is how we put the initial data into the buffer. */
  D3D11_SUBRESOURCE_DATA initialVertexData;
  initialVertexData.pSysMem = vertices;

  /* Actually create the buffer with the initial data.
   * We do this once: We'll NEVER CHANGE THE BUFFER AGAIN! */
  device->CreateBuffer(&vbd, &initialVertexData, &vertexBuffer);

  //Index buffer desc:
  D3D11_BUFFER_DESC ibd;

  /* "D3D11_USAGE_IMMUTABLE" signifies that we'll never change the data stored in the buffer, as well as that the buffer will be read only by the GPU. 
   * This is one of the fastest options: The driver stores the data directly in GPU memory. */
  ibd.Usage = D3D11_USAGE_IMMUTABLE;
  ibd.ByteWidth = sizeof(int) * IndexCount; //Number of indices
  ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
  ibd.CPUAccessFlags = 0;
  ibd.MiscFlags = 0;
  ibd.StructureByteStride = 0;

  D3D11_SUBRESOURCE_DATA initialIndexData;
  initialIndexData.pSysMem = indices;

  device->CreateBuffer(&ibd, &initialIndexData, &indexBuffer);
}

void Mesh::CalculateTangents(Vertex* vertices, UINT vertexCount, UINT* indices, UINT indexCount)
{
  XMFLOAT3* tan1 = new XMFLOAT3[vertexCount * 2];
  XMFLOAT3* tan2 = tan1 + vertexCount;
  ZeroMemory(tan1, vertexCount * sizeof(XMFLOAT3) * 2);
  int triangleCount = indexCount / 3;
  for(UINT i = 0; i < indexCount; i += 3)
  {
    int i1 = indices[i];
    int i2 = indices[i + 2];
    int i3 = indices[i + 1];
    auto v1 = vertices[i1].Position;
    auto v2 = vertices[i2].Position;
    auto v3 = vertices[i3].Position;

    auto w1 = vertices[i1].UV;
    auto w2 = vertices[i2].UV;
    auto w3 = vertices[i3].UV;

    float x1 = v2.x - v1.x;
    float x2 = v3.x - v1.x;
    float y1 = v2.y - v1.y;
    float y2 = v3.y - v1.y;
    float z1 = v2.z - v1.z;
    float z2 = v3.z - v1.z;

    float s1 = w2.x - w1.x;
    float s2 = w3.x - w1.x;
    float t1 = w2.y - w1.y;
    float t2 = w3.y - w1.y;
    float r = 1.0f / (s1 * t2 - s2 * t1);

    XMFLOAT3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);

    XMFLOAT3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

    XMStoreFloat3(&tan1[i1], XMLoadFloat3(&tan1[i1]) + XMLoadFloat3(&sdir));
    XMStoreFloat3(&tan1[i2], XMLoadFloat3(&tan1[i2]) + XMLoadFloat3(&sdir));
    XMStoreFloat3(&tan1[i3], XMLoadFloat3(&tan1[i3]) + XMLoadFloat3(&sdir));

    XMStoreFloat3(&tan2[i1], XMLoadFloat3(&tan2[i1]) + XMLoadFloat3(&tdir));
    XMStoreFloat3(&tan2[i2], XMLoadFloat3(&tan2[i2]) + XMLoadFloat3(&tdir));
    XMStoreFloat3(&tan2[i3], XMLoadFloat3(&tan2[i3]) + XMLoadFloat3(&tdir));
  }

  for(UINT a = 0; a < vertexCount; ++a)
  {
    auto n = vertices[a].Normal;
    auto t = tan1[a];

    //Gram-Schmidt orthogonalize:
    auto dot = XMVector3Dot(XMLoadFloat3(&n), XMLoadFloat3(&t));
    XMStoreFloat3(&vertices[a].Tangent, XMVector3Normalize(XMLoadFloat3(&t) - XMLoadFloat3(&n) * dot));

    //Calculate handedness.
    //tangent[a].w = (Dot(Cross(n, t), tan2[a]) < 0.0f) ? -1.0f : 1.0f;
  }

  delete[] tan1;
}

ID3D11Buffer* Mesh::GetVertexBuffer() {return vertexBuffer;}

ID3D11Buffer* Mesh::GetIndexBuffer() {return indexBuffer;}

int Mesh::GetIndexCount() {return indexCount;}

Mesh::~Mesh()
{
  if(vertexBuffer) 
    vertexBuffer->Release();

  if(indexBuffer) 
    indexBuffer->Release();
}