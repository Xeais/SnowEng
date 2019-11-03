//This is the quad.
struct VertexToPixel
{
  float4 position:	SV_POSITION;
  float2 uv:	TEXCOORD0;
};

VertexToPixel main(uint id : SV_VERTEXID)
{
  VertexToPixel output;

  output.uv = float2((id << 1) & 2, id & 2);

  output.position = float4(output.uv, 0.f, 1.0f);
  output.position.x = output.position.x * 2.0f - 1.0f;
  output.position.y = output.position.y * - 2.0f + 1.0f;
  return output;
}