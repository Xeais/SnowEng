#include "Game.h"
#include "DirectXMath.h"
#include "WICTextureLoader.h"
#include "DDSTextureLoader.h"
#include "ObjLoader.h"

using namespace DirectX;

Vertex MapObjlToVertex(objl::Vertex vertex)
{
  auto pos = XMFLOAT3(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
  auto normal = XMFLOAT3(vertex.Normal.X, vertex.Normal.Y, vertex.Normal.Z);
  auto uv = XMFLOAT2(vertex.TextureCoordinate.X, vertex.TextureCoordinate.Y);
  return {pos, normal, uv};
}

std::vector<Vertex> MapObjlToVertex(std::vector<objl::Vertex> vertices)
{
  std::vector<Vertex> verts;
  for(auto v : vertices)
    verts.push_back(MapObjlToVertex(v));
 
  return verts;
}

Game::Game(HINSTANCE hInstance) : DXCore(hInstance, "SnowEng", 1280, 720, true) //Window width and height must be literal values! As variables, they could not be retrieved fast enough, resulting in a tiny window at startup.
{
  vertexBuffer = nullptr;
  indexBuffer = nullptr;
  vertexShader = nullptr;
  pixelShader = nullptr;
  camera = nullptr;
  noiseTextureSize = 0;
  prevP = false;

#ifndef NDEBUG
  CreateConsoleWindow(500, 120, 32, 120);
  printf("Console window was created successfully. Feel free to \"printf()\" here.");
#endif
}

//Cleaning everything, even possible memory leaks ...
Game::~Game()
{
  if(vertexShader) 
    delete vertexShader;
  if(pixelShader) 
    delete pixelShader;

  if(quadPS) 
    delete quadPS;
  if(quadVS) 
    delete quadVS;
  if(blurPS) 
    delete blurPS;
  if(cocPS) 
    delete cocPS;
  if(dofPS) 
    delete dofPS;
  if(particlePS) 
    delete particlePS;
  if(particleEmitCS) 
    delete particleEmitCS;
  if(particleUpdateCS) 
    delete particleUpdateCS;
  if(particleDeadListInitCS) 
    delete particleDeadListInitCS;
  if(particleCopyDrawCountCS) 
    delete particleCopyDrawCountCS;
  if(particleDefaultVS) 
    delete particleDefaultVS;
  if(particleVS) 
    delete particleVS;
  if(skyPS) 
    delete skyPS;
  if(skyVS) 
    delete skyVS;

  if(noiseCS) 
    delete noiseCS;

  if(ppRTV) 
    ppRTV->Release();
  if(ppSRV) 
    ppSRV->Release();
  if(blurRTV) 
    blurRTV->Release();
  if(blurSRV) 
    blurSRV->Release();
  if(cocRTV) 
    cocRTV->Release();
  if(cocSRV) 
    cocSRV->Release();
  if(dofRTV) 
    dofRTV->Release();
  if(dofSRV) 
    dofSRV->Release();
  if(depthBufferSRV) 
    depthBufferSRV->Release();

  if(earthSRV) 
    earthSRV->Release();
  if(earthNormalSRV) 
    earthNormalSRV->Release();
  if(scratchedA) 
    scratchedA->Release();
  if(scratchedM) 
    scratchedM->Release();
  if(scratchedN) 
    scratchedN->Release();
  if(scratchedR) 
    scratchedR->Release();

  cobbleA->Release();
  cobbleM->Release();
  cobbleN->Release();
  cobbleR->Release();

  groundA->Release();
  groundN->Release();
  groundR->Release();
  groundM->Release();

  waterA->Release();
  waterN->Release();
  waterM->Release();
  waterR->Release();

  woodA->Release();
  woodN->Release();
  woodM->Release();
  woodR->Release();

  houseA->Release();
  houseN->Release();
  houseM->Release();
  houseR->Release();

  statueMeshA->Release();
  statueMeshN->Release();
  statueMeshM->Release();
  statueMeshR->Release();

  snowmanA->Release();
  snowmanN->Release();
  snowmanM->Release();
  snowmanR->Release();

  skySRV->Release();
  skyIrradiance->Release();
  skyPrefilter->Release();
  brdfLookUpTexture->Release();
  sampler->Release();

  skyDepthState->Release();
  skyRasterState->Release();

  if(computeTextureSRV) 
    computeTextureSRV->Release();
  if(computeTextureUAV) 
    computeTextureUAV->Release();

  delete earthMaterial;
  delete marsMaterial;
  delete saturnMaterial;
  delete sphereMaterial;
  delete cubeMaterial;
  delete houseMaterial;
  delete statueMaterial;
  delete snowmanMaterial;

  delete earthMesh;
  delete quadMesh;
  delete sphereMesh;
  delete cubeMesh;
  delete skyMesh;
  delete houseMesh;
  delete statueMesh;
  delete snowmanMesh;

  delete camera;

  delete particleEmitter;

  for(auto e : entities)
    delete e;
}

void Game::Init()
{
  //Directional light: Ambient, diffuse and direction
  light1 = {XMFLOAT4(+0.1f, +0.1f, +0.1f, 1.0f), XMFLOAT4(+0.2f, +0.2f, +0.2f, +1.0f), XMFLOAT3(+1.0f, +0.0f, 0.8f)};
  light2 = {XMFLOAT4(+0.1f, +0.1f, +0.1f, 1.0f), XMFLOAT4(+1.0f, +0.0f, +0.0f, +1.0f), XMFLOAT3(+1.0f, +0.0f, 0.0f)};

  //Point light: color, position, range and intensity
  light3 = {XMFLOAT4(+0.1f, +0.8f, +0.0f, 1.0f), XMFLOAT3(2.0f, -2.0f, 3.0f), 5.0f, 1.0f};

  //Spot light: color, position, intensity, direction, range and spot falloff
  light4 = {XMFLOAT4(+0.7f, +0.4f, +0.1f, 1.0f), XMFLOAT3(-3.0f, 1.0f, 3.0f), 105.0f, XMFLOAT3(0.0f, -1.0f, 0.0f), 50.0f, 10.0f};

  LoadShaders();
  CreateMatrices();

  InitTextures();
  InitializeComputeShader();

  //Load skybox-texture from DDS file:
  //CreateDDSTextureFromFile(device, L"../../Assets/Textures/orbitalSkybox.dds", 0, &skySRV);
  CreateDDSTextureFromFile(device, L"../../Assets/Textures/Skybox/envEnvHDR.dds", 0, &skySRV);

  //IBL baker textures:
  CreateDDSTextureFromFile(device, L"../../Assets/Textures/Skybox/envDiffuseHDR.dds", 0, &skyIrradiance);
  CreateDDSTextureFromFile(device, L"../../Assets/Textures/Skybox/envSpecularHDR.dds", 0, &skyPrefilter);
  CreateDDSTextureFromFile(device, L"../../Assets/Textures/Skybox/envBrdf.dds", 0, &brdfLookUpTexture);

  CreateMesh();
  //Create a sampler state for the textures.
  D3D11_SAMPLER_DESC sd = {};
  sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

  //sd.Filter = D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR; //tri-linear

  sd.Filter = D3D11_FILTER_ANISOTROPIC;
  sd.MaxAnisotropy = 16;
  sd.MaxLOD = D3D11_FLOAT32_MAX;

  device->CreateSamplerState(&sd, &sampler);

  //Create states for sky rendering.
  D3D11_RASTERIZER_DESC rs = {};
  rs.CullMode = D3D11_CULL_FRONT;
  rs.FillMode = D3D11_FILL_SOLID;
  device->CreateRasterizerState(&rs, &skyRasterState);

  D3D11_DEPTH_STENCIL_DESC ds = {};
  ds.DepthEnable = true;
  ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  ds.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
  device->CreateDepthStencilState(&ds, &skyDepthState);

  //Texture2D
  //RTV
  //SRV

  //Post processing: 
  ID3D11Texture2D* postProcTexture;
  ID3D11Texture2D* blurTexture;
  ID3D11Texture2D* cocTexture;
  ID3D11Texture2D* dofTexture;

  //Set up a render texture.
  D3D11_TEXTURE2D_DESC textureDesc = {};
  textureDesc.Width = width;
  textureDesc.Height = height;
  textureDesc.ArraySize = 1;
  textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  textureDesc.CPUAccessFlags = 0;
  textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDesc.MipLevels = 1;
  textureDesc.MiscFlags = 0;
  textureDesc.SampleDesc.Count = 1;
  textureDesc.SampleDesc.Quality = 0;
  textureDesc.Usage = D3D11_USAGE_DEFAULT;
  device->CreateTexture2D(&textureDesc, nullptr, &postProcTexture);
  device->CreateTexture2D(&textureDesc, nullptr, &blurTexture);
  device->CreateTexture2D(&textureDesc, nullptr, &cocTexture);
  device->CreateTexture2D(&textureDesc, nullptr, &dofTexture);

  //Set up a render target view.
  D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
  rtvDesc.Format = textureDesc.Format;
  rtvDesc.Texture2D.MipSlice = 0;
  rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  if(postProcTexture)
    device->CreateRenderTargetView(postProcTexture, &rtvDesc, &ppRTV);
  if(blurTexture)
    device->CreateRenderTargetView(blurTexture, &rtvDesc, &blurRTV);
  if(cocTexture)
    device->CreateRenderTargetView(cocTexture, &rtvDesc, &cocRTV);
  if(dofTexture)
    device->CreateRenderTargetView(dofTexture, &rtvDesc, &dofRTV);

  //Set up shader resource view.
  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = textureDesc.Format;
  srvDesc.Texture2D.MipLevels = 1;
  srvDesc.Texture2D.MostDetailedMip = 0;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  if(postProcTexture)
    device->CreateShaderResourceView(postProcTexture, &srvDesc, &ppSRV);
  if(blurTexture)
    device->CreateShaderResourceView(blurTexture, &srvDesc, &blurSRV);
  if(cocTexture)
    device->CreateShaderResourceView(cocTexture, &srvDesc, &cocSRV);
  if(dofTexture)
    device->CreateShaderResourceView(dofTexture, &srvDesc, &dofSRV);

  postProcTexture->Release();
  blurTexture->Release();
  cocTexture->Release();
  dofTexture->Release();

  //Particle setup:
  particleEmitter = new Emitter(100000,
                                100000.0f, //Particles per second
                                1000.0f,   //Particle lifetime
                                device,
                                context,
                                particleDeadListInitCS,
                                particleEmitCS,
                                particleUpdateCS,
                                particleCopyDrawCountCS,
                                particleDefaultVS,
                                particleVS,
                                particlePS);

  //What kind of shape?
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//Load shaders from compile shader object (.cso) and send data to individual GPU-variables.
void Game::LoadShaders()
{
  vertexShader = new SimpleVertexShader(device, context);
  vertexShader->LoadShaderFile(L"VertexShader.cso");

  pixelShader = new SimplePixelShader(device, context);
  pixelShader->LoadShaderFile(L"PixelShader.cso");

  skyPS = new SimplePixelShader(device, context);
  skyPS->LoadShaderFile(L"SkyPS.cso");

  skyVS = new SimpleVertexShader(device, context);
  skyVS->LoadShaderFile(L"SkyVS.cso");

  quadVS = new SimpleVertexShader(device, context);
  quadVS->LoadShaderFile(L"QuadVS.cso");

  quadPS = new SimplePixelShader(device, context);
  quadPS->LoadShaderFile(L"QuadPS.cso");

  blurPS = new SimplePixelShader(device, context);
  blurPS->LoadShaderFile(L"BlurPS.cso");

  cocPS = new SimplePixelShader(device, context);
  cocPS->LoadShaderFile(L"CircleOfConfusionPS.cso");

  dofPS = new SimplePixelShader(device, context);
  dofPS->LoadShaderFile(L"DepthOfFieldCompPS.cso");

  noiseCS = new SimpleComputeShader(device, context);
  if(!noiseCS->LoadShaderFile(L"Debug/ComputeShader.cso"))
    noiseCS->LoadShaderFile(L"ComputeShader.cso");

  particleEmitCS = new SimpleComputeShader(device, context);
  if(!particleEmitCS->LoadShaderFile(L"Debug/ParticleEmitCS.cso"))
    particleEmitCS->LoadShaderFile(L"ParticleEmitCS.cso");

  particleUpdateCS = new SimpleComputeShader(device, context);
  if(!particleUpdateCS->LoadShaderFile(L"Debug/ParticleUpdateCS.cso"))
    particleUpdateCS->LoadShaderFile(L"ParticleUpdateCS.cso");

  particleDeadListInitCS = new SimpleComputeShader(device, context);
  if(!particleDeadListInitCS->LoadShaderFile(L"Debug/ParticleDeadListInitCS.cso"))
    particleDeadListInitCS->LoadShaderFile(L"ParticleDeadListInitCS.cso");

  particleCopyDrawCountCS = new SimpleComputeShader(device, context);
  if(!particleCopyDrawCountCS->LoadShaderFile(L"Debug/ParticleCopyDrawCountCS.cso"))
    particleCopyDrawCountCS->LoadShaderFile(L"ParticleCopyDrawCountCS.cso");

  particleDefaultVS = new SimpleVertexShader(device, context);
  if(!particleDefaultVS->LoadShaderFile(L"Debug/ParticleDefaultVS.cso"))
    particleDefaultVS->LoadShaderFile(L"ParticleDefaultVS.cso");

  particleVS = new SimpleVertexShader(device, context);
  if(!particleVS->LoadShaderFile(L"Debug/ParticleVS.cso"))
    particleVS->LoadShaderFile(L"ParticleVS.cso");

  particlePS = new SimplePixelShader(device, context);
  if(!particlePS->LoadShaderFile(L"Debug/ParticlePS.cso"))
    particlePS->LoadShaderFile(L"ParticlePS.cso");
}

//Matrices for geometry:
void Game::CreateMatrices()
{
  /* Set up the world matrix.
   * Transpose happens because HLSL expects a different matrix than the one from the DirectX Math Library. */
  XMMATRIX W = XMMatrixIdentity();
  XMStoreFloat4x4(&worldMatrix, XMMatrixTranspose(W));

  //View matrix:
  XMVECTOR pos = XMVectorSet(0.0f, 0.0f, -5.0, 0.0f);
  XMVECTOR dir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  XMMATRIX V = XMMatrixLookToLH(pos, dir, up);

  XMStoreFloat4x4(&viewMatrix, XMMatrixTranspose(V));

  //Projection matrix:
  XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, //FOV (field of view angle)
                                        static_cast<float>(width) / height,
                                        0.1f,
                                        100.0f);

  XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P));

  camera = new Camera(0.0f, 0.0f, -5.0f);
  camera->UpdateProjectionMatrix(static_cast<float>(width) / height);
}

void Game::CreateMesh()
{
  earthMesh = new Mesh("../../Assets/Models/sphere.obj", device);
  quadMesh = new Mesh("../../Assets/Models/quad.obj", device);
  sphereMesh = new Mesh("../../Assets/Models/sphere.obj", device);
  cubeMesh = new Mesh("../../Assets/Models/cube.obj", device);
  skyMesh = new Mesh("../../Assets/Models/cube.obj", device);
  houseMesh = new Mesh("../../Assets/Models/hatka_local.obj", device);
  statueMesh = new Mesh("../../Assets/Models/lion-snake.obj", device);
  snowmanMesh = new Mesh("../../Assets/Models/snowman.obj", device);

  //Materials:
  earthMaterial = new Material(vertexShader, pixelShader, computeTextureSRV, earthNormalSRV, earthSRV, 0, sampler);
  marsMaterial = new Material(vertexShader, pixelShader, groundA, groundN, groundR, groundM, sampler);
  saturnMaterial = new Material(vertexShader, pixelShader, waterA, waterN, waterR, waterM, sampler);
  sphereMaterial = new Material(vertexShader, pixelShader, scratchedA, scratchedN, scratchedR, scratchedM, sampler);
  cubeMaterial = new Material(vertexShader, pixelShader, woodA, woodN, woodR, woodM, sampler);
  houseMaterial = new Material(vertexShader, pixelShader, houseA, houseN, houseR, houseM, sampler);
  statueMaterial = new Material(vertexShader, pixelShader, statueMeshA, statueMeshN, statueMeshR, statueMeshM, sampler);
  snowmanMaterial = new Material(vertexShader, pixelShader, snowmanA, snowmanN, snowmanR, snowmanM, sampler);

  //Entities:
  entities.push_back(new GameEntity(sphereMesh, earthMaterial));
  entities.push_back(new GameEntity(quadMesh, marsMaterial));
  entities.push_back(new GameEntity(sphereMesh, saturnMaterial));
  entities.push_back(new GameEntity(sphereMesh, sphereMaterial));
  entities.push_back(new GameEntity(sphereMesh, cubeMaterial));
  entities.push_back(new GameEntity(houseMesh, houseMaterial));
  entities.push_back(new GameEntity(statueMesh, statueMaterial));
  entities.push_back(new GameEntity(snowmanMesh, snowmanMaterial));
}

void Game::DrawSky()
{
  ID3D11Buffer* skyVB = skyMesh->GetVertexBuffer();
  ID3D11Buffer* skyIB = skyMesh->GetIndexBuffer();

  //Set buffers:
  UINT stride = sizeof(Vertex);
  UINT offset = 0;

  context->IASetVertexBuffers(0, 1, &skyVB, &stride, &offset);
  context->IASetIndexBuffer(skyIB, DXGI_FORMAT_R32_UINT, 0);

  //Set up sky shaders. (No need for world matrix!)
  skyVS->SetMatrix4x4("view", camera->GetViewMatrix());
  skyVS->SetMatrix4x4("projection", camera->GetProjectionMatrix());
  skyVS->CopyAllBufferData();
  skyVS->SetShader();

  skyPS->SetShaderResourceView("SkyTexture", skySRV);
  skyPS->SetSamplerState("BasicSampler", sampler);
  skyPS->SetShader();

  //Set up render states necessary for the sky.
  context->RSSetState(skyRasterState);
  context->OMSetDepthStencilState(skyDepthState, 0);
  context->DrawIndexed(skyMesh->GetIndexCount(), 0, 0);

  //When done rendering, reset all states for the next frame.
  context->RSSetState(nullptr);
  context->OMSetDepthStencilState(nullptr, 0);
}

void Game::DrawBlur()
{
  context->RSSetState(nullptr);
  context->OMSetDepthStencilState(nullptr, 0);
  context->OMSetRenderTargets(1, &blurRTV, nullptr);

  //Post process shaders:
  quadVS->SetShader();
  blurPS->SetShader();

  blurPS->SetShaderResourceView("Pixels", ppSRV);
  blurPS->SetShaderResourceView("DepthBuffer", depthBufferSRV);
  blurPS->SetSamplerState("Sampler", sampler);
  blurPS->SetFloat("blurAmount", 5);
  blurPS->SetFloat("pixelWidth", 1.0f / width);
  blurPS->SetFloat("pixelHeight", 1.0f / height);
  blurPS->SetFloat("focusPlaneZ", focusZ);
  blurPS->SetFloat("zFar", 100.0f);
  blurPS->SetFloat("zNear", 0.1f);
  blurPS->CopyAllBufferData();

  UINT stride = sizeof(Vertex);
  UINT offset = 0;

  ID3D11Buffer* nothing = nullptr;
  context->IASetVertexBuffers(0, 1, &nothing, &stride, &offset);
  context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

  context->Draw(3, 0);
  blurPS->SetShaderResourceView("Pixels", nullptr);
  blurPS->SetShaderResourceView("DepthBuffer", nullptr);
}

void Game::DrawCircleofConfusion()
{
  //Set up "cocRTV":
  context->RSSetState(nullptr);
  context->OMSetDepthStencilState(nullptr, 0);
  context->OMSetRenderTargets(1, &cocRTV, nullptr);

  quadVS->SetShader();
  cocPS->SetShader();

  cocPS->SetShaderResourceView("Pixels", ppSRV);
  cocPS->SetSamplerState("Sampler", sampler);
  cocPS->SetShaderResourceView("DepthBuffer", depthBufferSRV);
  cocPS->SetFloat("focusPlaneZ", focusZ);
  cocPS->SetFloat("scale", 0.15f);
  cocPS->SetFloat("zFar", 100.0f);
  cocPS->SetFloat("zNear", 0.1f);
  cocPS->CopyAllBufferData();

  UINT stride = sizeof(Vertex);
  UINT offset = 0;

  ID3D11Buffer* nothing = nullptr;
  context->IASetVertexBuffers(0, 1, &nothing, &stride, &offset);
  context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

  context->Draw(3, 0);
  cocPS->SetShaderResourceView("Pixels", nullptr);
  cocPS->SetShaderResourceView("DepthBuffer", nullptr);
}

void Game::DrawDepthofField()
{
  //Set up "dofRTV":
  context->RSSetState(nullptr);
  context->OMSetDepthStencilState(nullptr, 0);
  context->OMSetRenderTargets(1, &dofRTV, nullptr);

  quadVS->SetShader();
  dofPS->SetShader();

  dofPS->SetShaderResourceView("Pixels", ppSRV);
  dofPS->SetSamplerState("Sampler", sampler);
  dofPS->SetShaderResourceView("BlurTexture", blurSRV);
  dofPS->SetShaderResourceView("Radius", cocSRV);

  dofPS->CopyAllBufferData();
  UINT stride = sizeof(Vertex);
  UINT offset = 0;

  ID3D11Buffer* nothing = nullptr;
  context->IASetVertexBuffers(0, 1, &nothing, &stride, &offset);
  context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

  context->Draw(3, 0);
  dofPS->SetShaderResourceView("Pixels", nullptr);
  dofPS->SetShaderResourceView("BlurTexture", nullptr);
  dofPS->SetShaderResourceView("Radius", nullptr);
}

void Game::DrawSimplex(float deltaTime, float totalTime)
{
  //Launch ("dispatch") compute shader.
  noiseCS->SetInt("iterations", 8);
  noiseCS->SetFloat("persistence", 0.5f);
  noiseCS->SetFloat("scale", 0.01f);
  noiseCS->SetFloat("offset", totalTime);
  noiseCS->SetUnorderedAccessView("outputTexture", computeTextureUAV);
  noiseCS->SetShader();
  noiseCS->CopyAllBufferData();
  noiseCS->DispatchByThreads(noiseTextureSize, noiseTextureSize, 1);

  //Unbind the texture, so it can later be used in "Draw()".
  noiseCS->SetUnorderedAccessView("outputTexture", 0);
}

void Game::InitializeComputeShader()
{
  noiseTextureSize = 256;
  ID3D11Texture2D* noiseTexture;

  D3D11_TEXTURE2D_DESC texDesc = {};
  texDesc.Width = noiseTextureSize;
  texDesc.Height = noiseTextureSize;
  texDesc.ArraySize = 1;
  texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
  texDesc.CPUAccessFlags = 0;
  texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  texDesc.MipLevels = 1;
  texDesc.MiscFlags = 0;
  texDesc.SampleDesc.Count = 1;
  texDesc.SampleDesc.Quality = 0;
  texDesc.Usage = D3D11_USAGE_DEFAULT;
  device->CreateTexture2D(&texDesc, nullptr, &noiseTexture);

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = texDesc.Format;
  srvDesc.Texture2D.MipLevels = 1;
  srvDesc.Texture2D.MostDetailedMip = 0;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  if(noiseTexture)
    device->CreateShaderResourceView(noiseTexture, &srvDesc, &computeTextureSRV);

  D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
  uavDesc.Format = texDesc.Format;
  uavDesc.Texture2D.MipSlice = 0;
  uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
  if(noiseTexture)
    device->CreateUnorderedAccessView(noiseTexture, &uavDesc, &computeTextureUAV);

  noiseTexture->Release();
}

void Game::InitTextures()
{
  //Load textures and normal maps.
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/earth_Diffuse.jpg", nullptr, &earthSRV);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/earth_Normal.jpg", nullptr, &earthNormalSRV);

  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Scratched/scratched_albedo.png", nullptr, &scratchedA);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Scratched/scratched_normals.png", nullptr, &scratchedN);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Scratched/scratched_roughness.png", nullptr, &scratchedR);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Scratched/scratched_metal.png", nullptr, &scratchedM);

  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Cobble/cobblestone_albedo.png", nullptr, &cobbleA);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Cobble/cobblestone_normals.png", nullptr, &cobbleN);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Cobble/cobblestone_roughness.png", nullptr, &cobbleR);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Cobble/cobblestone_metal.png", nullptr, &cobbleM);

  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Ground/ground_albedo.png", nullptr, &groundA);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Ground/ground_normals.png", nullptr, &groundN);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Ground/ground_roughness.png", nullptr, &groundR);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Ground/ground_metal.png", nullptr, &groundM);

  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Water/water_albedo.jpg", nullptr, &waterA);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Water/water_normal.jpg", nullptr, &waterN);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Water/water_roughness.jpg", nullptr, &waterR);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Water/water_metal.jpg", nullptr, &waterM);

  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Floor/floor_diffuse.png", nullptr, &woodA);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Floor/floor_normal.png", nullptr, &woodN);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Floor/floor_roughness.png", nullptr, &woodR);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Floor/floor_metal.png", nullptr, &woodM);

  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/House/house_diffuse.png", nullptr, &houseA);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/House/house_normal.png", nullptr, &houseN);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/House/house_roughness.png", nullptr, &houseR);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/House/house_metal.png", nullptr, &houseM);

  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Hydrant/hydrant_diffuse.png", nullptr, &statueMeshA);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Hydrant/hydrant_normal.png", nullptr, &statueMeshN);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Hydrant/hydrant_roughness.png", nullptr, &statueMeshR);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Hydrant/hydrant_metal.png", nullptr, &statueMeshM);

  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Snowman/snowman_diffuse.png", nullptr, &snowmanA);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Snowman/snowman_normal.png", nullptr, &snowmanN);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Snowman/snowman_roughness.png", nullptr, &snowmanR);
  CreateWICTextureFromFile(device, context, L"../../Assets/Textures/Snowman/snowman_metal.png", nullptr, &snowmanM);
}

void Game::DrawEntity(GameEntity* gameEntityObject)
{
  vertexShader->SetMatrix4x4("world", gameEntityObject->GetMatrix());
  vertexShader->SetMatrix4x4("view", camera->GetViewMatrix());
  vertexShader->SetMatrix4x4("projection", camera->GetProjectionMatrix());

  pixelShader->SetData("light1", &light1, sizeof(DirectionalLight));
  pixelShader->SetData("light2", &light2, sizeof(DirectionalLight));
  pixelShader->SetData("light3", &light3, sizeof(PointLight));
  pixelShader->SetData("light4", &light4, sizeof(SpotLight));
  pixelShader->SetFloat3("CameraPosition", camera->GetPosition());

  gameEntityObject->PrepareMaterial(viewMatrix, projectionMatrix, sampler, skyIrradiance, skyPrefilter, brdfLookUpTexture);
  vertexShader->CopyAllBufferData();
  vertexShader->SetShader();
  pixelShader->CopyAllBufferData();
  pixelShader->SetShader();

  DrawMesh(gameEntityObject->GetMesh());
}

void Game::OnResize()
{
  DXCore::OnResize();

  //Update projection matrix, since window size has changed.
  XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(width) / height, 0.1f, 100.0f);

  XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P));

  if(camera)
    camera->UpdateProjectionMatrix(static_cast<float>(width) / height);
}

void Game::Update(float deltaTime, float totalTime)
{
  /* Buttons
   * -------
   *
   * Quit if escape is hit. */
  if(GetAsyncKeyState(VK_ESCAPE))
    Quit();

  if(GetAsyncKeyState(VK_TAB))
    isdofEnabled = true;
  else
    isdofEnabled = false;

  static bool updateParticles = false;
  bool currentP = (GetAsyncKeyState('P') & 0x8000) != 0;
  if(currentP && !prevP)
    updateParticles = !updateParticles;
  prevP = currentP;

  if(GetAsyncKeyState('I'))
  {
    light3 = {XMFLOAT4(+0.1f, +0.8f, +0.0f, 1.0f), XMFLOAT3(2.0f, -2.0f, 3.0f), 8.0f, 2.0f};
    pixelShader->SetData("light3", &light3, sizeof(PointLight));
  }
  else
  {
    light3 = {XMFLOAT4(+0.1f, +0.8f, +0.0f, 1.0f), XMFLOAT3(2.0f, -2.0f, 3.0f), 4.0f, 1.0f};
    pixelShader->SetData("light3", &light3, sizeof(PointLight));
  }

  if(GetAsyncKeyState('F'))
  {
    light4 = {XMFLOAT4(+0.7f, +0.4f, +0.1f, 1.0f), XMFLOAT3(-3.0f, 1.0f, 3.0f), 150.0f, XMFLOAT3(0.0f, -1.0f, 0.0f), 100.0f, 5.0f};
    pixelShader->SetData("light4", &light4, sizeof(PointLight));
  }
  else
  {
    light4 = {XMFLOAT4(+0.7f, +0.4f, +0.1f, 1.0f), XMFLOAT3(-3.0f, 1.0f, 3.0f), 100.0f, XMFLOAT3(0.0f, -1.0f, 0.0f), 50.0f, 9.0f};
    pixelShader->SetData("light4", &light4, sizeof(PointLight));
  }

  if(updateParticles)
    particleEmitter->Update(deltaTime, totalTime);
  //=======================

  camera->Update(deltaTime);

  DrawSimplex(deltaTime, totalTime);

  //Earth:
  entities[0]->SetScale(XMFLOAT3(1.0f, 1.0f, 1.0f));
  entities[0]->SetRotation(XM_PI, totalTime * 0.25f, 0.0f);
  entities[0]->SetTranslation(XMFLOAT3(5.0f, -1.0f, 0.0f));

  //Mars:
  entities[1]->SetTranslation(XMFLOAT3(0.0f, -2.0f, 7.0f));
  entities[1]->SetScale(XMFLOAT3(20.0f, 10.0f, 20.0f));

  //Saturn:
  //entities[2]->SetTranslation(XMFLOAT3(3.0f, 0.0f, 7.0f));
  entities[2]->SetTranslation(XMFLOAT3(5.0f, -1.0f, 3.0f));

  //Sphere:
  entities[3]->SetRotation(XM_PI, totalTime * 0.25f, 0.0f);
  entities[3]->SetTranslation(XMFLOAT3(5.0f, -1.0f, 2.0f));

  //Cube:
  entities[4]->SetTranslation(XMFLOAT3(5.0f, -1.0f, 1.0f));

  //House:
  entities[5]->SetScale(XMFLOAT3(0.007f, .007f, .007f));
  entities[5]->SetRotation(0.0f, XM_PIDIV2, 0.0f);
  entities[5]->SetTranslation(XMFLOAT3(0.0f, -2.0f, 7.0f));
  //entities[5]->SetTranslation(XMFLOAT3(0.0f, 0.0f, 2.0f));

  //Statue:
  entities[6]->SetScale(XMFLOAT3(0.05f, 0.05f, 0.05f));
  entities[6]->SetRotation(0.0f, XM_PI, 0.0f);
  entities[6]->SetTranslation(XMFLOAT3(4.0f, -2.0f, 7.0f));

  //Snowman:
  entities[7]->SetRotation(0.0f, XM_PI, 0.0f);
  entities[7]->SetTranslation(XMFLOAT3(1.0f, -2.0f, 1.0f));
}

void Game::Draw(float deltaTime, float TotalTime)
{
  //Background color:
  const float color[4] = {0.1f, 0.1f, 0.1f, 0.0f};

  //Do this before drawing ANYTHING!
  context->ClearRenderTargetView(backBufferRTV, color);
  context->ClearRenderTargetView(ppRTV, color);
  context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

  context->OMSetRenderTargets(1, &ppRTV, depthStencilView);

  UINT stride = sizeof(Vertex);
  UINT offset = 0;
  context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
  context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

  for(auto e : entities)
    DrawEntity(e);

  DrawSky();

  DrawBlur();

  DrawCircleofConfusion();

  DrawDepthofField();

  //Set up "backbufferRTV":
  context->RSSetState(nullptr);
  context->OMSetDepthStencilState(nullptr, 0);
  context->OMSetRenderTargets(1, &backBufferRTV, nullptr);

  quadVS->SetShader();
  quadPS->SetShader();
  ID3D11ShaderResourceView* result = nullptr;
  if(isdofEnabled)
    result = dofSRV;
  else
    result = ppSRV;
  quadPS->SetShaderResourceView("Pixels", result);
  quadPS->SetSamplerState("Sampler", sampler);
  quadPS->CopyAllBufferData();

  context->Draw(3, 0);

  //Draw particles.
  D3D11_DEPTH_STENCIL_DESC ds = {};
  ds.DepthEnable = true;
  ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
  ds.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
  device->CreateDepthStencilState(&ds, &depthWriteDisabled);
  context->OMSetDepthStencilState(depthWriteDisabled, 0);
  context->OMSetRenderTargets(1, &backBufferRTV, depthStencilView);

  particleEmitter->Draw(camera, static_cast<float>(width) / height, static_cast<float>(width), static_cast<float>(height), true);
  depthWriteDisabled->Release();

  //Done!
  swapChain->Present(0, 0);
  quadPS->SetShaderResourceView("Pixels", nullptr);
}

#pragma region Mouse Input
void Game::OnMouseDown(WPARAM buttonState, int x, int y)
{
  prevMousePos.x = x;
  prevMousePos.y = y;

  SetCapture(hWnd);
}

void Game::OnMouseUp(WPARAM buttonState, int x, int y) {ReleaseCapture();}

void Game::OnMouseMove(WPARAM buttonState, int x, int y)
{
  if(buttonState & 0x0001)
  {
    float xDiff = (x - prevMousePos.x) * 0.005f;
    float yDiff = (y - prevMousePos.y) * 0.005f;
    camera->Rotate(yDiff, xDiff);
  }

  prevMousePos.x = x;
  prevMousePos.y = y;
}

//void Game::OnMouseWheel(float wheelDelta, int x, int y) {}

void Game::DrawMesh(Mesh* mesh)
{
  UINT stride = sizeof(Vertex);
  UINT offset = 0;
  auto vertexBuffer = mesh->GetVertexBuffer();
  context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
  context->IASetIndexBuffer(mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

  context->DrawIndexed(mesh->GetIndexCount(), 0, 0);
}
#pragma endregion