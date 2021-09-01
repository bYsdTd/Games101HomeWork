//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_MATERIAL_H
#define RAYTRACING_MATERIAL_H

#include "Vector.hpp"

enum MaterialType { DIFFUSE, MICROFACET_DIFFUSE, MICROFACET_GLOSSY};

class Material{
private:

    // Compute reflection direction
    Vector3f reflect(const Vector3f &I, const Vector3f &N) const
    {
        return I - 2 * dotProduct(I, N) * N;
    }

    // Compute refraction direction using Snell's law
    //
    // We need to handle with care the two possible situations:
    //
    //    - When the ray is inside the object
    //
    //    - When the ray is outside.
    //
    // If the ray is outside, you need to make cosi positive cosi = -N.I
    //
    // If the ray is inside, you need to invert the refractive indices and negate the normal N
    Vector3f refract(const Vector3f &I, const Vector3f &N, const float &ior) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        Vector3f n = N;
        if (cosi < 0) { cosi = -cosi; } else { std::swap(etai, etat); n= -N; }
        float eta = etai / etat;
        float k = 1 - eta * eta * (1 - cosi * cosi);
        return k < 0 ? 0 : eta * I + (eta * cosi - sqrtf(k)) * n;
    }

    // Compute Fresnel equation
    //
    // \param I is the incident view direction
    //
    // \param N is the normal at the intersection point
    //
    // \param ior is the material refractive index
    //
    // \param[out] kr is the amount of light reflected
    void fresnel(const Vector3f &I, const Vector3f &N, const float &ior, float &kr) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        if (cosi > 0) {  std::swap(etai, etat); }
        // Compute sini using Snell's law
        float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
        // Total internal reflection
        if (sint >= 1) {
            kr = 1;
        }
        else {
            float cost = sqrtf(std::max(0.f, 1 - sint * sint));
            cosi = fabsf(cosi);
            float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
            float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
            kr = (Rs * Rs + Rp * Rp) / 2;
        }
        // As a consequence of the conservation of energy, transmittance is given by:
        // kt = 1 - kr;
    }

    Vector3f toWorld(const Vector3f &a, const Vector3f &N){
        Vector3f B, C;
        if (std::fabs(N.x) > std::fabs(N.y)){
            float invLen = 1.0f / std::sqrt(N.x * N.x + N.z * N.z);
            C = Vector3f(N.z * invLen, 0.0f, -N.x *invLen);
        }
        else {
            float invLen = 1.0f / std::sqrt(N.y * N.y + N.z * N.z);
            C = Vector3f(0.0f, N.z * invLen, -N.y *invLen);
        }
        B = crossProduct(C, N);
        return a.x * B + a.y * C + a.z * N;
    }

public:
    MaterialType m_type;
    //Vector3f m_color;
    Vector3f m_emission;
    float ior;
    Vector3f Kd, Ks, F0;
    float smoothness;
    float specularExponent;
    //Texture tex;

    inline Material(MaterialType t=DIFFUSE, Vector3f e=Vector3f(0,0,0));
    inline MaterialType getType();
    //inline Vector3f getColor();
    inline Vector3f getColorAt(double u, double v);
    inline Vector3f getEmission();
    inline bool hasEmission();

    // sample a ray by Material properties
    inline Vector3f sample(const Vector3f &wi, const Vector3f &N);
    // given a ray, calculate the PdF of this ray
    inline float pdf(const Vector3f &wi, const Vector3f &wo, const Vector3f &N);
    // given a ray, calculate the contribution of this ray
    inline Vector3f eval(const Vector3f &wi, const Vector3f &wo, const Vector3f &N);

    inline Vector3f cookTorrance(const Vector3f &wi, const Vector3f &wo, const Vector3f &N);

    inline float DistributionGGX(const Vector3f &N, const Vector3f &H, float a);
    inline float GeometrySchlickGGX(float NdotV, float k);
    inline float GeometrySmith(const Vector3f &N, const Vector3f &V, const Vector3f &L, float k);
    inline Vector3f fresnelSchlick(float cosTheta, const Vector3f &F0);
};

Material::Material(MaterialType t, Vector3f e){
    m_type = t;
    //m_color = c;
    m_emission = e;
}

MaterialType Material::getType(){return m_type;}
///Vector3f Material::getColor(){return m_color;}
Vector3f Material::getEmission() {return m_emission;}
bool Material::hasEmission() {
    if (m_emission.norm() > EPSILON) return true;
    else return false;
}

Vector3f Material::getColorAt(double u, double v) {
    return Vector3f();
}


Vector3f Material::sample(const Vector3f &wi, const Vector3f &N){
    switch(m_type){
        case DIFFUSE: case MICROFACET_DIFFUSE: case MICROFACET_GLOSSY:
        {
            // uniform sample on the hemisphere
            float x_1 = get_random_float(), x_2 = get_random_float();
            float z = std::fabs(1.0f - 2.0f * x_1);
            float r = std::sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
            Vector3f localRay(r*std::cos(phi), r*std::sin(phi), z);
            return toWorld(localRay, N);
            
            break;
        }
    }
}

float Material::pdf(const Vector3f &wi, const Vector3f &wo, const Vector3f &N){
    switch(m_type){
        case DIFFUSE: case MICROFACET_DIFFUSE: case MICROFACET_GLOSSY:
        {
            // uniform sample probability 1 / (2 * PI)
            if (dotProduct(wo, N) > 0.0f)
                return 0.5f / M_PI;
            else
                return 0.0f;
            break;
        }
    }
}

#include <assert.h>

float Material::DistributionGGX(const Vector3f &N, const Vector3f &H, float a)
{
    float a2 = a * a;
    float NdotH  = std::max(dotProduct(N, H), 0.0f);
    float NdotH2 = NdotH*NdotH;
    
    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = M_PI * denom * denom;
    
    return nom / denom;
}

float Material::GeometrySchlickGGX(float NdotV, float k)
{
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float Material::GeometrySmith(const Vector3f &N, const Vector3f &V, const Vector3f &L, float k)
{
    float NdotV = std::max(dotProduct(N, V), 0.0f);
    float NdotL = std::max(dotProduct(N, L), 0.0f);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
	
    return ggx1 * ggx2;
}

Vector3f Material::fresnelSchlick(float cosTheta, const Vector3f &F0)
{
    return F0 + (Vector3f(1.0f) - F0) * pow(1.0f - cosTheta, 5.0f);
}

Vector3f Material::cookTorrance(const Vector3f &wi, const Vector3f &wo, const Vector3f &N) {
    // return 0;
    
    auto V = wo;
    auto L = wi;
    auto H = normalize(V + L);
    auto type = m_type;

    if(!(dotProduct(N, V) > 0 && dotProduct(N, L) > 0)) return 0;

    //Distribution
    float D = DistributionGGX(N, H, smoothness);

    // Geometry
    float k = smoothness * smoothness * 0.5f;
    float G = GeometrySmith(N, V, L, k);
    
    // Fresnel
    auto F = fresnelSchlick(std::max(dotProduct(H, V), 0.0f), F0);

    auto ans = F * G * D / (dotProduct(N, L) * dotProduct(N, V) * 4);
    return ans;
}

static Vector3f eval_diffuse(const Vector3f &wi, const Vector3f &wo, const Vector3f &N) {
    return 1.0 / M_PI;
}

Vector3f Material::eval(const Vector3f &wi, const Vector3f &wo, const Vector3f &N){
    switch(m_type){
        case DIFFUSE: //case MICROFACET_DIFFUSE: case MICROFACET_GLOSSY:
        {
            float cosalpha = dotProduct(N, wo);
            if (cosalpha > 0.0f) {
                return Kd * eval_diffuse(wi, wo, N);
            }
            else
                return Vector3f(0.0f);
            break;
        }
        case MICROFACET_DIFFUSE: 
        {
            float cosalpha = dotProduct(N, wo);
            if (cosalpha > 0.0f) {
                auto ans = Ks * cookTorrance(wi, wo, N) + Kd * eval_diffuse(wi, wo, N);
              //  clamp(0, 1, ans.x); clamp(0, 1, ans.y); clamp(0, 1, ans.z);
            //   auto ans = Kd * eval_diffuse(wi, wo, N);
            // auto ans = Ks * cookTorrance(wi, wo, N);
                return ans;
            }
            else
                return Vector3f(0.0f);
            break;
        }
        case MICROFACET_GLOSSY:
        {
            float cosalpha = dotProduct(N, wo);
            if (cosalpha > 0.0f) {
                double p = 25;
                auto h = normalize(wi + wo);
                double spec = pow(std::max(0.0f, dotProduct(N, h)), p);
                auto ans = Ks * spec + Kd * eval_diffuse(wi, wo, N);
              //  clamp(0, 1, ans.x); clamp(0, 1, ans.y); clamp(0, 1, ans.z);
                return ans;
            }
            else
                return Vector3f(0.0f);
            break;
        }

    }
}

#endif //RAYTRACING_MATERIAL_H