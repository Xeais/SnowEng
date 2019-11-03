#include "Main.h"
#include <Windows.h>
#include <crtdbg.h>
#include "Game.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
  //MessageBox(nullptr, "Example", "Example", MB_ICONEXCLAMATION | MB_OK);

#ifndef NDEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
  {
    char currentDir[1024] = {};
    GetModuleFileName(0, currentDir, 1024);
    char* lastSlash = strrchr(currentDir, '\\');
    if(lastSlash)
    {
      *lastSlash = 0;
      SetCurrentDirectory(currentDir);
    }
  }

  Game dxGame(hInstance);

  HRESULT hr = S_OK;

  hr = dxGame.InitWndow();
  if(FAILED(hr)) 
    return hr;

  hr = dxGame.InitDirectX();
  if(FAILED(hr)) 
    return hr;

  return dxGame.Run();
}