static const float F0_NON_METAL = 0.04f;

//A minimum roughness is needed when spec distribution function denominator goes to zero!
static const float MIN_ROUGHNESS = 0.0000001f;

static const float PI = 3.14159265359f;

struct DirectionalLight
{
  float4 AmbientColor;
  float4 DiffuseColor;
  float3 Direction;
  float Intensity;
};

struct PointLight
{
  float4 Color;
  float3 Position;
  float Range;
  float Intensity;
  float3 padding;
};

struct SpotLight
{
  float4 Color;
  float3 Position;
  float Intensity;
  float3 Direction;
  float Range;
  float SpotFalloff;
  float3 padding;
};

float DiffusePBR(float3 normal, float3 dirToLight) {return saturate(dot(normal, dirToLight));}

float Attenuate(float3 lightPos, float3 worldPos, float lightRange)
{
  float dist = distance(lightPos, worldPos);

  //Ranged-based attenuation:
  float att = saturate(1.0f - (dist * dist / (lightRange * lightRange)));

  //Soft falloff
  return att * att;
}

/* GGX (Trowbridge-Reitz)
 *
 * a = Roughness
 * h = Half vector
 * n = Normal
 * 
 * D(h, n) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2 */
float SpecDistribution(float3 n, float3 h, float roughness)
{
  //Pre-calculations:
  float NdotH = saturate(dot(n, h));
  float NdotH2 = NdotH * NdotH;
  float a = roughness * roughness;
  float a2 = max(a * a, MIN_ROUGHNESS); //Applied after remap!

  //((n dot h)^2 * (a^2 - 1) + 1)
  float denomToSquare = NdotH2 * (a2 - 1.0f) + 1.0f;
  //Can reach zero if roughness is zero and "NdotH" is one!

  return a2 / (PI * denomToSquare * denomToSquare);
}

/* Fresnel term - Schlick approx.
 * 
 * v = View vector
 * h = Half vector
 * f0 = Value when l = n (full specular color)
 *
 * F(v, h, f0) = f0 + (1 - f0)(1 - (v dot h))^5 */
float3 Fresnel(float3 v, float3 h, float3 f0)
{
  //Pre-calculation:
  float VdotH = saturate(dot(v, h));

  return f0 + (1.0f - f0) * pow(1.0f - VdotH, 5.0f);
}

/* Geometric Shadowing - Schlick-GGX (based on Schlick-Beckmann)
 * k is remapped to a / 2, roughness remapped to (r + 1) / 2
 *
 * n = Normal
 * v = View vector
 *
 * G(l, v, h) */
float GeometricShadowing(float3 n, float3 v, float3 h, float roughness)
{
  //End result of remapping:
  float k = pow(roughness + 1.0f, 2.0f) / 8.0f;
  float NdotV = saturate(dot(n, v));

  return NdotV / (NdotV * (1.0f - k) + k);
}

/* Microfacet BRDF (Specular)
 *
 * f(l, v) = D(h) * F(v, h) * G(l, v, h) / 4(n dot l) * (n dot v)
 * Part of the denominator are canceled out by numerator (see below).
 *
 * D() = Spec Dist - Trowbridge-Reitz (GGX)
 * F() = Fresnel - Schlick approx.
 * G() = Geometric Shadowing - Schlick-GGX */
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float metalness, float3 specColor, out float3 kS)
{
  float3 h = normalize(v + l);

  float D = SpecDistribution(n, h, roughness);
  float3 F = Fresnel(v, h, specColor); //This ranges from "F0_NON_METAL" to the actual "specColor" based on metalness.
  float G = GeometricShadowing(n, v, h, roughness) * GeometricShadowing(n, l, h, roughness);
  kS = F;
  /* Final formula
   * Denominator dot products are partially canceled by "G()"!
   * See page 16: http://blog.selfshadow.com/publications/s2012-shading-course/hoffman/s2012_pbs_physics_math_notes.pdf */
  return (D * F * G) / (4.0f * max(dot(n, v), dot(n, l)));
}

/* Calculates diffuse color based on energy conservation.
 *
 * diffuse = Diffuse color
 * specular = Specular color (including light color)
 * metalness = Surface metalness amount
 *
 * Metals should have an albedo of (0, 0, 0), mostly.
 * See page 65: http://blog.selfshadow.com/publications/s2014-shading-course/hoffman/s2014_pbs_physics_math_slides.pdf */
float3 DiffuseEnergyConserve(float diffuse, float3 specular, float metalness) {return diffuse * ((1.0f - saturate(specular)) * (1.0f - metalness));}

float3 AmbientPBR(float3 kD, float metalness, float3 diffuse, float ao, float3 specular)
{
  kD *= (1.0f - metalness);
  return (kD * diffuse + specular) * ao;
}

float3 DirLightPBR(DirectionalLight light, float3 normal, float3 worldPos, float3 camPos,
                   float roughness, float metalness, float3 surfaceColor, float3 specularColor,
                   float3 irradiance, float3 prefilteredColor, float2 brdf)
{
  //Get normalized direction to the light.
  float3 toLight = normalize(-light.Direction);
  float3 toCam = normalize(camPos - worldPos);

  //Calculate the light amounts:
  float diff = DiffusePBR(normal, toLight);
  float3 kS = float3(0.0f, 0.0f, 0.0f);
  float ao = 1.0f;

  float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, metalness, specularColor, kS);
  float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);

  //Calculate diffuse with energy conservation (reflected light doesn't get diffused).
  float3 specular = prefilteredColor * (kS * brdf.x + brdf.y);
  float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
  float3 diffuse = irradiance * surfaceColor;
  float3 ambient = AmbientPBR(kD, metalness, diffuse, ao, specular);

  return (balancedDiff * surfaceColor + spec) * 1.0f /* light.Intensity */ * light.DiffuseColor.rgb + ambient;

  //float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);
  //return (balancedDiff * surfaceColor + spec) /* light.Intensity */ * light.DiffuseColor;
}

float3 PointLightPBR(PointLight light, float3 normal, float3 worldPos, float3 camPos,
                     float roughness, float metalness, float3 surfaceColor, float3 specularColor)
{
  float3 kS = float3(0.0f, 0.0f, 0.0f);
  float ao = 1.0f;
  //Calculate light direction.
  float3 toLight = normalize(light.Position - worldPos);
  float3 toCam = normalize(camPos - worldPos);

  //Calculate the light amounts:
  float atten = Attenuate(light.Position, worldPos, light.Range);
  float diff = DiffusePBR(normal, toLight);
  float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, metalness, specularColor, kS);

  //Calculate diffuse with energy conservation (reflected light doesn't get diffused).
  float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);

  //Combine:
  return (balancedDiff * surfaceColor + spec) * atten * light.Intensity * light.Color.xyz;
}

float3 SpotLightPBR(SpotLight light, float3 normal, float3 worldPos, float3 camPos,
                    float roughness, float metalness, float3 surfaceColor, float3 specularColor)
{
  float3 kS = float3(0.0f, 0.0f, 0.0f);
  float ao = 1.0f;
  //Calculate light direction.
  float3 toLight = normalize(light.Position - worldPos);
  float3 toCam = normalize(camPos - worldPos);

  //Calculate the spot falloff.
  float a = dot(-toLight, light.Direction);
  float b = saturate(a);

  float penumbra = pow(b, light.SpotFalloff);

  //Calculate the light amounts.
  float atten = Attenuate(light.Position, worldPos, light.Range);
  float diff = DiffusePBR(normal, toLight);
  float3 spec = MicrofacetBRDF(normal, toLight, toCam, roughness, metalness, specularColor, kS);

  //Calculate diffuse with energy conservation (reflected light doesn't get diffused).
  float3 balancedDiff = DiffuseEnergyConserve(diff, spec, metalness);

  //Combine:
  float3 final = (balancedDiff * surfaceColor + spec) * atten * light.Intensity * light.Color.xyz;
  final = final * penumbra; //Combine with the point light calculation (this could be optimized a bit).
  return final;
}