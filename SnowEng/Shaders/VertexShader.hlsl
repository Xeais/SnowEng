cbuffer externalData : register(b0)
{
  matrix world;
  matrix view;
  matrix projection;
};

struct VertexShaderInput
{
  float3 position: POSITION;
  float3 normal: NORMAL;
  float2 uv: TEXCOORD;
  float3 tangent: TANGENT;
};

//This must match pixel shader's input!
struct VertexToPixel
{
  float4 position: SV_POSITION;
  float3 normal: NORMAL;
  float2 uv: TEXCOORD;
  float3 tangent: TANGENT;
  float3 worldPos: POSITION; //In world space
  float linearZ: LINEARZ;
};

float2 GetProjectionConstants(float nearZ, float farZ)
{
  float2 projectionConstants;
  projectionConstants.x = farZ / (farZ - nearZ);
  projectionConstants.y = (-farZ * nearZ) / (farZ - nearZ);
  return projectionConstants;
}

VertexToPixel main(VertexShaderInput input)
{
  VertexToPixel output;

  float2 ProjectionConstants = GetProjectionConstants(0.1f, 100.f);

  matrix worldViewProj = mul(mul(world, view), projection);

  output.position = mul(float4(input.position, 1.0f), worldViewProj);

  //Normal to world space:
  output.normal = mul(input.normal, (float3x3)world);

  //Calculate the world position of this vertex (to be used in the pixel shader for point/spot lights).
  output.worldPos = mul(float4(input.position, 1.0f), world).xyz;

  //Also convert the tangent from local to world space.
  output.tangent = normalize(mul(input.tangent, (float3x3)world));

  output.uv = input.uv;

  float depth = output.position.z / output.position.w;

  output.linearZ = ProjectionConstants.y / (depth - ProjectionConstants.x);

  return output;
}