#ifndef NOISE_INCLUDED
#define NOISE_INCLUDED
// Description: Array and textureless GLSL 2D simplex noise function.
//      Author: Ian McEwan, Ashima Arts.
//  Maintainer: ijm
//     Lastmod: 20110822 (ijm)
//     License: Copyright (C) 2011 Ashima Arts. All rights reserved.
//              Distributed under the MIT License. See LICENSE file.
//              https://github.com/ashima/webgl-noise
// Modified by Daniel Dubs

float4 mod289(float4 x) {return x - floor(x * (1.0f / 289.0f)) * 289.0f;}

float3 mod289(float3 x) {return x - floor(x * (1.0f / 289.0f)) * 289.0f;}

float2 mod289(float2 x) {return x - floor(x * (1.0f / 289.0f)) * 289.0f;}

float mod289(float x) {return x - floor(x * (1.0f / 289.0f)) * 289.0f;}

float permute(float x) {return mod289(((x * 34.0f) + 1.0f) * x);}

float3 permute(float3 x) {return mod289((x * 34.0f + 1.0f) * x);}

float4 permute(float4 x) {return mod289((x * 34.0f + 1.0f) * x);}

float4 taylorInvSqrt(float4 r) {return 1.79284291400159f - 0.85373472095314f * r;}

float taylorInvSqrt(float r) {return 1.79284291400159f - 0.85373472095314f * r;}

float4 grad4(float j, float4 ip)
{
  const float4 ones = float4(1.0f, 1.0f, 1.0f, -1.0f);
  float4 p, s;

  p.xyz = floor(frac(float3(j, j, j) * ip.xyz) * 7.0f) * ip.z - 1.0f;
  p.w = 1.5f - dot(abs(p.xyz), ones.xyz);
  //s = p;//float4(lessThan(p, float4(0.0f)));
  if(p.x < 0.0f)
    s.x = 1.0f;
  else
    s.x = 0.0f;
  if(p.y < 0.0f)
    s.y = 1.0f;
  else
    s.y = 0.0f;
  if(p.z < 0.0f)
    s.z = 1.0f;
  else
    s.z = 0.0f;
  if(p.w < 0.0f)
    s.w = 1.0f;
  else
    s.w = 0.0f;
  p.xyz = p.xyz + (s.xyz * 2.0f - 1.0f) * s.www;

  return p;
}

float snoise(float2 v)
{
  const float4 C = float4(0.211324865405187f,  //(3.0f - sqrt(3.0f)) / 6.0f
                          0.366025403784439f,  //0.5f * (sqrt(3.0f) - 1.0f)
                          -0.577350269189626f, //-1.0f + 2.0f * C.x
                          0.024390243902439f); //1.0f / 41.0f

  //First corner:
  float2 i = floor(v + dot(v, C.yy));
  float2 x0 = v - i + dot(i, C.xx);

  //Other corners:
  float2 i1;
  //i1.x = step(x0.y, x0.x); //x0.x > x0.y ? 1.0f : 0.0f
  //i1.y = 1.0f - i1.x;
  i1 = (x0.x > x0.y) ? float2(1.0f, 0.0f) : float2(0.0f, 1.0f);
  //x0 = x0 - 0.0f + 0.0f * C.xx;
  //x1 = x0 - i1 + 1.0f * C.xx;
  //x2 = x0 - 1.0f + 2.0f * C.xx;
  float4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;

  //Permutations:
  i = mod289(i); //Avoid truncation effects in permutation.
  float3 p = permute(permute(i.y + float3(0.0f, i1.y, 1.0f)) + i.x + float3(0.0f, i1.x, 1.0f));

  float3 m = max(0.5f - float3(dot(x0, x0), dot(x12.xy, x12.xy), dot(x12.zw, x12.zw)), 0.0f);
  m = m * m;
  m = m * m;

  /* Gradients: 41 points uniformly over a line, mapped onto a diamond.
   * The ring size (17 * 17 = 289) is close to a multiple of 41 (41 * 7 = 287). */

  float3 x = 2.0f * frac(p * C.www) - 1.0f;
  float3 h = abs(x) - 0.5f;
  float3 ox = floor(x + 0.5f);
  float3 a0 = x - ox;

  /* Normalise gradients implicitly by scaling m.
   * Approximation of: m *= inversesqrt(a0 * a0 + h * h); */
  m *= 1.79284291400159f - 0.85373472095314f * (a0 * a0 + h * h);

  //Compute final noise value at P.
  float3 g;
  g.x = a0.x * x0.x + h.x * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0f * dot(m, g);
}

float snoise(float3 v)
{
  const float2 C = float2(1.0f / 6.0f, 1.0f / 3.0f);
  const float4 D = float4(0.0f, 0.5f, 1.0f, 2.0f);

  //First corner:
  float3 i = floor(v + dot(v, C.yyy));
  float3 x0 = v - i + dot(i, C.xxx);

  //Other corners:
  float3 g = step(x0.yzx, x0.xyz);
  float3 l = 1.0f - g;
  float3 i1 = min(g.xyz, l.zxy);
  float3 i2 = max(g.xyz, l.zxy);

  //x0 = x0 - 0.0f + 0.0f * C.xxx;
  //x1 = x0 - i1  + 1.0f * C.xxx;
  //x2 = x0 - i2  + 2.0f * C.xxx;
  //x3 = x0 - 1.0f + 3.0f * C.xxx;
  float3 x1 = x0 - i1 + C.xxx;
  float3 x2 = x0 - i2 + C.yyy; //2.0f * C.x = 1f / 3f = C.y
  float3 x3 = x0 - D.yyy;      //-1.0f + 3.0f * C.x = -0.5f = -D.y

  //Permutations:
  i = mod289(i);
  float4 p = permute(permute(permute(i.z + float4(0.0f, i1.z, i2.z, 1.0f)) + i.y + float4(0.0f, i1.y, i2.y, 1.0f)) + i.x + float4(0.0f, i1.x, i2.x, 1.0f));

  /* Gradients: 7 * 7 points over a square, mapped onto an octahedron.
   * The ring size (17 * 17 = 289) is close to a multiple of 49 (49 * 6 = 294). */
  float n_ = 0.142857142857f; //1.0f / 7.0f
  float3  ns = n_ * D.wyz - D.xzx;

  float4 j = p - 49.0f * floor(p * ns.z * ns.z); //mod(p, 49)

  float4 x_ = floor(j * ns.z);
  float4 y_ = floor(j - 7.0f * x_); //mod(j,N)

  float4 x = x_ * ns.x + ns.yyyy;
  float4 y = y_ * ns.x + ns.yyyy;
  float4 h = 1.0f - abs(x) - abs(y);

  float4 b0 = float4(x.xy, y.xy);
  float4 b1 = float4(x.zw, y.zw);

  //float4 s0 = float4(lessThan(b0, 0.0f)) * 2.0f - 1.0f;
  //float4 s1 = float4(lessThan(b1, 0.0f)) * 2.0f - 1.0f;
  float4 s0 = floor(b0) * 2.0f + 1.0f;
  float4 s1 = floor(b1) * 2.0f + 1.0f;
  float4 sh = -step(h, float4(0.0f, 0.0f, 0.0f, 0.0f));

  float4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
  float4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

  float3 p0 = float3(a0.xy, h.x);
  float3 p1 = float3(a0.zw, h.y);
  float3 p2 = float3(a1.xy, h.z);
  float3 p3 = float3(a1.zw, h.w);

  //Normalise gradients:
  float4 norm = taylorInvSqrt(float4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

  //Mix final noise value:
  float4 m = max(0.5f - float4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0f);
  m = m * m;

  return 42.0f * dot(m * m, float4(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3)));
}

//(sqrt(5.0f) - 1.0f) / 4.0f = F4, used once below
#define F4 0.309016994374947451f

float snoise(float4 v)
{
  const float4  C = float4(0.138196601125011f,  //(5.0f - sqrt(5.0f)) / 20.0f  G4
                           0.276393202250021,   //2.0f * G4
                           0.414589803375032,   //3.0f * G4
                           -0.447213595499958); //-1.0f + 4.0f * G4

  //First corner:
  float4 i = floor(v + dot(v, float4(F4, F4, F4, F4)));
  float4 x0 = v - i + dot(i, C.xxxx);

  //Other corners:

  //Rank sorting originally contributed by Bill Licea-Kane, AMD (formerly ATI)
  float4 i0;
  float3 isX = step(x0.yzw, x0.xxx);
  float3 isYZ = step(x0.zww, x0.yyz);
  //i0.x = dot(isX, float3(1.0f));
  i0.x = isX.x + isX.y + isX.z;
  i0.yzw = 1.0f - isX;
  //i0.y += dot(isYZ.xy, float2(1.0f));
  i0.y += isYZ.x + isYZ.y;
  i0.zw += 1.0f - isYZ.xy;
  i0.z += isYZ.z;
  i0.w += 1.0f - isYZ.z;

  //"i0" now contains the unique values 0, 1, 2, 3 in each channel.
  float4 i3 = clamp(i0, 0.0f, 1.0f);
  float4 i2 = clamp(i0 - 1.0f, 0.0f, 1.0f);
  float4 i1 = clamp(i0 - 2.0f, 0.0f, 1.0f);

  //x0 = x0 - 0.04 + 0.0f * C.xxxx
  //x1 = x0 - i1 + 1.0f * C.xxxx
  //x2 = x0 - i2 + 2.0f * C.xxxx
  //x3 = x0 - i3  + 3.0f * C.xxxx
  //x4 = x0 - 1.0f + 4.0f * C.xxxx
  float4 x1 = x0 - i1 + C.xxxx;
  float4 x2 = x0 - i2 + C.yyyy;
  float4 x3 = x0 - i3 + C.zzzz;
  float4 x4 = x0 + C.wwww;

  //Permutations:
  i = mod289(i);
  float j0 = permute(permute(permute(permute(i.w) + i.z) + i.y) + i.x);
  float4 j1 = permute(permute(permute(permute(i.w + float4(i1.w, i2.w, i3.w, 1.0f)) + i.z + float4(i1.z, i2.z, i3.z, 1.0))
                                           + i.y + float4(i1.y, i2.y, i3.y, 1.0f)) + i.x + float4(i1.x, i2.x, i3.x, 1.0f));

  /* Gradients: 7 * 7 * 6 points over a cube, mapped onto a 4-cross polytope.
   * 7 * 7 * 6 = 294, which is close to the ring size 17 * 17 = 289. */
  float4 ip = float4(1.0f / 294.0f, 1.0f / 49.0f, 1.0f / 7.0f, 0.0f);

  float4 p0 = grad4(j0, ip);
  float4 p1 = grad4(j1.x, ip);
  float4 p2 = grad4(j1.y, ip);
  float4 p3 = grad4(j1.z, ip);
  float4 p4 = grad4(j1.w, ip);

  //Normalise gradients:
  float4 norm = taylorInvSqrt(float4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;
  p4 *= taylorInvSqrt(dot(p4, p4));

  //Mix contributions from the five corners:
  float3 m0 = max(0.5f - float3(dot(x0, x0), dot(x1, x1), dot(x2, x2)), 0.0f);
  float2 m1 = max(0.5f - float2(dot(x3, x3), dot(x4, x4)), 0.0f);
  m0 = m0 * m0;
  m1 = m1 * m1;

  return 49.0f * (dot(m0 * m0, float3(dot(p0, x0), dot(p1, x1), dot(p2, x2))) + dot(m1 * m1, float2(dot(p3, x3), dot(p4, x4))));
}

float3 snoise3D(float3 v)
{
  float3 n = float3(snoise(float2(v.x, v.y)), snoise(float2(v.y, v.z)), snoise(float2(v.z, v.x)));
  return n;
}

float curlX(float3 v, float d)
{
  return ((snoise3D(float3(v.x, v.y + d, v.z)).z - snoise3D(float3(v.x, v.y - d, v.z)).z) - (snoise3D(float3(v.x, v.y, v.z + d)).y - snoise3D(float3(v.x, v.y, v.z - d)).y)) / 2.0f / d;
}

float curlY(float3 v, float d)
{
  return ((snoise3D(float3(v.x, v.y, v.z + d)).x - snoise3D(float3(v.x, v.y, v.z - d)).x) - (snoise3D(float3(v.x + d, v.y, v.z)).z - snoise3D(float3(v.x - d, v.y, v.z)).z)) / 2.0f / d;
}

float curlZ(float3 v, float d)
{
  return ((snoise3D(float3(v.x + d, v.y, v.z)).y - snoise3D(float3(v.x - d, v.y, v.z)).y) - (snoise3D(float3(v.x, v.y + d, v.z)).x - snoise3D(float3(v.x, v.y - d, v.z)).x)) / 2.0f / d;
}

/* Edited to match HLSL (prior: GLSL) and parameterize the "d" variable.
 * From: https://github.com/cabbibo/glsl-curl-noise/blob/master/curl.glsl */
float3 curlNoise3D(float3 p, float d)
{
  //const float d = 0.1f;
  float3 dx = float3(d, 0.0f, 0.0f);
  float3 dy = float3(0.0f, d, 0.0f);
  float3 dz = float3(0.0f, 0.0f, d);

  float3 p_x0 = snoise3D(p - dx);
  float3 p_x1 = snoise3D(p + dx);
  float3 p_y0 = snoise3D(p - dy);
  float3 p_y1 = snoise3D(p + dy);
  float3 p_z0 = snoise3D(p - dz);
  float3 p_z1 = snoise3D(p + dz);

  float x = p_y1.z - p_y0.z - p_z1.y + p_z0.y;
  float y = p_z1.x - p_z0.x - p_x1.z + p_x0.z;
  float z = p_x1.y - p_x0.y - p_y1.x + p_y0.x;

  //const float divisor = 1.0f / (2.0f * d);
  return (float3(x, y, z) * (2.0f * d));
}

/* Calculates multiple octaves of noise and combines them together - like "Generate Clouds" from Photoshop.
 * -> http://cmaher.github.io/posts/working-with-simplex-noise/ */
float CalcNoiseWithOctaves(float3 seed, float scale, float offset, float persistence, int iterations)
{
  float maxAmp = 0.0f;
  float amp = 1.0f;
  float noise = 0.0f;
  float freq = scale;

  for(int i = 0; i < iterations; ++i)
  {
    float3 itSeed = seed;
    itSeed.xy *= freq;
    itSeed.z = offset;

    float adjNoise = snoise(itSeed) * 0.5f + 0.5f;
    noise += adjNoise * amp;
    maxAmp += amp;
    amp *= persistence;
    freq *= 2.0f;
  }

  //Get the average.
  return noise / maxAmp;
}
#endif