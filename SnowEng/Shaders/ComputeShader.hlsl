/* Ideal to dispatch multiple groups - rather than one huge group.
 * Many GPUs can handle group sizes of 64.
 * Very small group sizes are not great.
 * One thread group runs per hardware shader unit.
 * Small groups result in poor hardware saturation.
 *
 * Structured buffers on the other hand utilize the unified cache architecture, which means the first read is slow, 
 * but all subsequent reads are very fast (if the requested data is already in the cache). */

#include "SimplexNoise.hlsli"

RWTexture2D<float4> outputTexture : register(u0);

cbuffer data : register(b0)
{
  float offset;
  float scale;
  float persistence;
  int iterations;
}

/* Calculates multiple octaves of noise and combines them together - like "Generate Clouds" from Photoshop.
 * -> http://cmaher.github.io/posts/working-with-simplex-noise/ */
float CalcNoiseWithOctaves(float3 seed)
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

[numthreads(8, 8, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
  //Start the seed as threadId(x, y, z)
  float3 seed = (float3)threadId;
  seed.z = offset;

  //Calculate the final noise for this thread.
  float noise = CalcNoiseWithOctaves(seed);

  //Store in texture at [x, y]
  outputTexture[threadId.xy] = float4(noise, noise, noise, 1.0f);
}