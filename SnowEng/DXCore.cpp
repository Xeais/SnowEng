#include <WindowsX.h>
#include "DXCore.h"
#include <sstream>

DXCore* DXCore::DXCoreInstance = nullptr;

LRESULT DXCore::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {return DXCoreInstance->ProcessMessage(hWnd, uMsg, wParam, lParam);}

DXCore::DXCore(HINSTANCE HInstance, const char* TitleBarText, unsigned int WindowWidth, unsigned int WindowHeight, bool DebugTitleBarStats)
{
  DXCoreInstance = this;

  hInstance = HInstance;
  titleBartext = TitleBarText;
  width = WindowWidth;
  height = WindowHeight;
  titleBarStats = DebugTitleBarStats;

  fpsFrameCount = 0;
  fpsTimeElapsed = 0.0f;

  device = nullptr;
  context = nullptr;
  swapChain = nullptr;
  backBufferRTV = nullptr;
  depthStencilView = nullptr;

  __int64 perfFreq;
  QueryPerformanceFrequency((LARGE_INTEGER*)& perfFreq);
  perfCounterSeconds = 1.0 / static_cast<double>(perfFreq);
}

DXCore::~DXCore()
{
  if(depthStencilView) 
    depthStencilView->Release();

  if(backBufferRTV) 
    backBufferRTV->Release(); 

  if(swapChain) 
    swapChain->Release();

  if(context) 
    context->Release(); 

  if(device)
    device->Release();
}

HRESULT DXCore::InitWndow()
{
  WNDCLASS wndClass = {};
  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = DXCore::WindowProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hInstance = hInstance;
  wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wndClass.lpszMenuName = NULL;
  wndClass.lpszClassName = "Direct3DWindowClass";

  if(!RegisterClass(&wndClass))
  {
    DWORD error = GetLastError();
    if(error != ERROR_CLASS_ALREADY_EXISTS)
      return HRESULT_FROM_WIN32(error);
  }
  RECT clientRect;
  SetRect(&clientRect, 0, 0, width, height);
  AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, false);

  RECT desktopRect;
  GetClientRect(GetDesktopWindow(), &desktopRect);
  int centeredX = (desktopRect.right / 2) - (clientRect.right / 2);
  int ceneteredY = (desktopRect.bottom / 2) - (clientRect.bottom / 2);

  hWnd = CreateWindow(wndClass.lpszClassName, titleBartext.c_str(), WS_OVERLAPPEDWINDOW, centeredX, ceneteredY, 
                      clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, 0, 0, hInstance, 0);

  if(hWnd == NULL)
  {
    DWORD error = GetLastError();
    return HRESULT_FROM_WIN32(error);
  }

  ShowWindow(hWnd, SW_SHOW);

  return S_OK;
}

HRESULT DXCore::InitDirectX() //Requires a window!
{
  UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifndef NDEBUG //If the project is in a debug build, enable the debug layer.
  deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  //Define the ordering of feature levels that Direct3D attempts to create.
  D3D_FEATURE_LEVEL featureLevels[] =
  {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3,
    D3D_FEATURE_LEVEL_9_1
  };

  //Handles two buffers, aka page flipping
  DXGI_SWAP_CHAIN_DESC swapDesc = {};
  swapDesc.BufferCount = 1;
  swapDesc.BufferDesc.Width = width;
  swapDesc.BufferDesc.Height = height;
  swapDesc.BufferDesc.RefreshRate.Numerator = 60;
  swapDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapDesc.Flags = 0;
  swapDesc.OutputWindow = hWnd;
  swapDesc.SampleDesc.Count = 1;
  swapDesc.SampleDesc.Quality = 0;
  swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  swapDesc.Windowed = true;

  HRESULT hr = S_OK;

  hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
                                     &swapDesc, &swapChain, &device, &dxFeatureLevel, &context);

  if(FAILED(hr)) 
    return hr;

  ID3D11Texture2D* backBufferTexture;
  swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)& backBufferTexture);

  if(backBufferTexture)
  {
    device->CreateRenderTargetView(backBufferTexture, nullptr, &backBufferRTV);
    backBufferTexture->Release();
  }

  D3D11_TEXTURE2D_DESC depthStencilDesc = {};
  depthStencilDesc.Width = width;
  depthStencilDesc.Height = height;
  depthStencilDesc.MipLevels = 1;
  depthStencilDesc.ArraySize = 1;
  depthStencilDesc.Format = DXGI_FORMAT_R32_TYPELESS;
  depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
  depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
  depthStencilDesc.CPUAccessFlags = 0;
  depthStencilDesc.MiscFlags = 0;
  depthStencilDesc.SampleDesc.Count = 1;
  depthStencilDesc.SampleDesc.Quality = 0;

  D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
  dsv_desc.Flags = 0;
  dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
  dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
  dsv_desc.Texture2D.MipSlice = 0;

  ID3D11Texture2D* depthBufferTexture;
  device->CreateTexture2D(&depthStencilDesc, nullptr, &depthBufferTexture);
  if(depthBufferTexture)
    device->CreateDepthStencilView(depthBufferTexture, &dsv_desc, &depthStencilView);

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = 1;
  srvDesc.Texture2D.MostDetailedMip = 0;
  if(depthBufferTexture)
  {
    device->CreateShaderResourceView(depthBufferTexture, &srvDesc, &depthBufferSRV);
    depthBufferTexture->Release();
  }

  context->OMSetRenderTargets(1, &backBufferRTV, depthStencilView);

  D3D11_VIEWPORT viewport = {};
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = static_cast<float>(width);
  viewport.Height = static_cast<float>(height);
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  context->RSSetViewports(1, &viewport);

  return S_OK;
}

//Thereby, window resizing does not change anything!
void DXCore::OnResize()
{
  if(depthStencilView) 
    depthStencilView->Release();

  if(backBufferRTV)
    backBufferRTV->Release();

  swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

  ID3D11Texture2D* backBufferTexture;
  swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBufferTexture));
  if(backBufferTexture)
  {
    device->CreateRenderTargetView(backBufferTexture, nullptr, &backBufferRTV);
    backBufferTexture->Release();
  }

  D3D11_TEXTURE2D_DESC depthStencilDesc;
  depthStencilDesc.Width = width;
  depthStencilDesc.Height = height;
  depthStencilDesc.MipLevels = 1;
  depthStencilDesc.ArraySize = 1;
  depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
  depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  depthStencilDesc.CPUAccessFlags = 0;
  depthStencilDesc.MiscFlags = 0;
  depthStencilDesc.SampleDesc.Count = 1;
  depthStencilDesc.SampleDesc.Quality = 0;

  ID3D11Texture2D* depthBufferTexture;
  device->CreateTexture2D(&depthStencilDesc, nullptr, &depthBufferTexture);
  if(depthBufferTexture)
  {
    device->CreateDepthStencilView(depthBufferTexture, nullptr, &depthStencilView);
    depthBufferTexture->Release();
  }

  context->OMSetRenderTargets(1, &backBufferRTV, depthStencilView);

  //Render to the correct window portion.
  D3D11_VIEWPORT viewport = {};
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = static_cast<float>(width);
  viewport.Height = static_cast<float>(height);
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  context->RSSetViewports(1, &viewport);
}

//MAIN GAME LOOP
HRESULT DXCore::Run()
{
  __int64 now;
  QueryPerformanceCounter((LARGE_INTEGER*)& now);
  startTime = now;
  currentTime = now;
  previousTime = now;

  Init();

  MSG msg = {};

  while(msg.message != WM_QUIT)
  {
    //Look if there is a message waiting.
    if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else
    {
      UpdateTimer();
      if(titleBarStats)
        UpdateTitleBarStats();

      Update(deltaTime, totalTime);
      Draw(deltaTime, totalTime);
    }
  }

  return static_cast<HRESULT>(msg.wParam);
}

void DXCore::Quit() {PostMessage(hWnd, WM_CLOSE, NULL, NULL);}

void DXCore::UpdateTimer()
{
  __int64 now;
  QueryPerformanceCounter((LARGE_INTEGER*)& now);
  currentTime = now;

  deltaTime = max(static_cast<float>((currentTime - previousTime) * perfCounterSeconds), 0.0f);

  totalTime = static_cast<float>((currentTime - startTime) * perfCounterSeconds);

  previousTime = currentTime;
}

void DXCore::UpdateTitleBarStats()
{
  ++fpsFrameCount;

  float timeDiff = totalTime - fpsTimeElapsed;
  if(timeDiff < 1.0f)
    return;

  //How long did each frame take?
  float mspf = 1000.0f / fpsFrameCount;

  std::ostringstream output;
  output.precision(6);
  output << titleBartext << " (Width: " << width << ", Height: " << height << " | FPS: " << fpsFrameCount << ", Frame Time: " << mspf << " ms)";

  switch(dxFeatureLevel)
  {
    case D3D_FEATURE_LEVEL_11_1: 
      output << " || DX 11.1"; 
      break;
    case D3D_FEATURE_LEVEL_11_0: 
      output << " || DX 11.0"; 
      break;
    case D3D_FEATURE_LEVEL_10_1: 
      output << " || DX 10.1"; 
      break;
    case D3D_FEATURE_LEVEL_10_0: 
      output << " || DX 10.0"; 
      break;
    case D3D_FEATURE_LEVEL_9_3:  
      output << " || DX 9.3"; 
      break;
    case D3D_FEATURE_LEVEL_9_2:  
      output << " || DX 9.2"; 
      break;
    case D3D_FEATURE_LEVEL_9_1:  
      output << " || DX 9.1"; 
      break;
    default:                     
      output << " || DX ?.?"; 
      break;
  }

  SetWindowText(hWnd, output.str().c_str());
  fpsFrameCount = 0;
  fpsTimeElapsed += 1.0f;
}

//Console window for debugging:
void DXCore::CreateConsoleWindow(int bufferLines, int bufferColumns, int windowLines, int windowColumns)
{
  CONSOLE_SCREEN_BUFFER_INFO conInfo;

  AllocConsole();
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &conInfo);
  conInfo.dwSize.Y = bufferLines;
  conInfo.dwSize.X = bufferColumns;
  SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), conInfo.dwSize);

  SMALL_RECT rect;
  rect.Left = 0;
  rect.Right = windowColumns;
  rect.Top = 0;
  rect.Bottom = windowLines;
  SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);

  FILE* stream;
  freopen_s(&stream, "CONIN$", "r", stdin);
  freopen_s(&stream, "CONOUT$", "w", stdout);
  freopen_s(&stream, "CONOUT$", "w", stderr);

  //To prevent accidental closing of window:
  HWND consoleHandle = GetConsoleWindow();
  HMENU hmenu = GetSystemMenu(consoleHandle, FALSE);
  EnableMenuItem(hmenu, SC_CLOSE, MF_GRAYED);
}

LRESULT DXCore::ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  //Check incoming message handle.
  switch(uMsg)
  {
    case WM_DESTROY:
      PostQuitMessage(0); //Send quit message to our own program.

      return 0;
    case WM_MENUCHAR:
      return MAKELRESULT(0, MNC_CLOSE);
    case WM_GETMINMAXINFO:
      //Prevent window from becoming too small.
      ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 1296;
      ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 759;

      return 0;
    case WM_SIZE:
      if(wParam == SIZE_MINIMIZED)
        return 0;

      width = LOWORD(lParam);
      height = HIWORD(lParam);

      if(device)
        OnResize();

      return 0;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
      OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

      return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
      OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

      return 0;
    case WM_MOUSEMOVE:
      OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

      return 0;
  }

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}