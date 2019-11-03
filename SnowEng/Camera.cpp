#include "Windows.h"
#include "Camera.h"

Camera::Camera(float x, float y, float z)
{
  //Creating a camera at a certain position.
  position = XMFLOAT3(x, y, z);
  startPosition = XMFLOAT3(x, y, z);
  XMStoreFloat4(&direction, XMQuaternionIdentity());
  roll = 0.0f;
  pitch = 0.0f;

  XMStoreFloat4x4(&viewMatrix, XMMatrixIdentity());
  XMStoreFloat4x4(&projMatrix, XMMatrixIdentity());
}

Camera::~Camera() {}

void Camera::MoveRelative(float x, float y, float z)
{
  XMVECTOR dir = XMVector3Rotate(XMVectorSet(x, y, z, 0.0f), XMLoadFloat4(&direction));

  XMStoreFloat3(&position, XMLoadFloat3(&position) + dir);
}

void Camera::MoveAbsolute(float x, float y, float z)
{
  //Simple addition:
  position.x += x;
  position.y += y;
  position.z += z;
}

void Camera::Rotate(float x, float y)
{
  roll += x;
  pitch += y;

  //Clamp x between PI/2 and -PI/2:
  roll = max(min(roll, XM_PIDIV2), -XM_PIDIV2);

  XMStoreFloat4(&direction, XMQuaternionRotationRollPitchYaw(roll, pitch, 0.0f));
}

XMFLOAT3 Camera::GetPosition() {return position;}

XMFLOAT4X4 Camera::GetViewMatrix() {return viewMatrix;}

XMFLOAT4X4 Camera::GetProjectionMatrix() {return projMatrix;}

void Camera::Update(float dt)
{
  float speed = dt * 3.0f;

  if(GetAsyncKeyState(VK_SHIFT))
    speed *= 5.0f;
 
  if(GetAsyncKeyState(VK_CONTROL))
    speed *= 0.1f;
  
  if(GetAsyncKeyState('W') & 0x8000)
    MoveRelative(0.0f, 0.0f, speed);

  if(GetAsyncKeyState('S') & 0x8000)
    MoveRelative(0.0f, 0.0f, -speed);

  if(GetAsyncKeyState('A') & 0x8000)
    MoveRelative(-speed, 0.0f, 0.0f);
 
  if(GetAsyncKeyState('D') & 0x8000)
    MoveRelative(speed, 0.0f, 0.0f);

  if(GetAsyncKeyState('X') & 0x8000)
    MoveAbsolute(0.0f, -speed, 0.0f);

  if(GetAsyncKeyState(' ') & 0x8000)
    MoveAbsolute(0.0f, speed, 0.0f);

  if(GetAsyncKeyState('R') & 0x8000)
  {
    position = startPosition;
    roll = 0.0f;
    pitch = 0.0f;
    XMStoreFloat4(&direction, XMQuaternionIdentity());
  }

  UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
  XMVECTOR dir = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), XMLoadFloat4(&direction));

  XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&position), dir, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

  XMStoreFloat4x4(&viewMatrix, XMMatrixTranspose(view));
}

void Camera::UpdateProjectionMatrix(float aspectRatio)
{
  XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, aspectRatio, 0.1f, 100);

  XMStoreFloat4x4(&projMatrix, XMMatrixTranspose(P));
}