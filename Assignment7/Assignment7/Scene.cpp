//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject) const
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// // p 着色点，wo是入射光方向
// Vector3f Scene::shade(const Intersection &p, const Vector3f &wo) const
// {
//     // shade(p, wo)
//     //  sampleLight(inter, pdf_light)
//     //  Get x, ws, NN, emit from inter
//     //  Shoot a ray from p to x
//     //  If the ray is not blocked in the middle
//     //      L_dir = emit * eval(wo, ws, N) * dot(ws, N) * dot(ws, NN) / |x-p|^2 / pdf_light

//     // L_indir = 0.0
//     // Test Russian Roulette with probability RussianRoulette wi = sample(wo, N)
//     // Trace a ray r(p, wi)
//     // If ray r hit a non-emitting object at q
//     //      L_indir = shade(q, wi) * eval(wo, wi, N) * dot(wi, N) / pdf(wo, wi, N) / RussianRoulette
//     // Return L_dir + L_indir

//     Vector3f p0 = p.coords;
//     // 直接光计算
//     Intersection inter;
//     float pdf_light;
//     sampleLight(inter, pdf_light);
//     Vector3f x = inter.coords;
//     Vector3f ws = normalize(x-p0);
//     Vector3f NN = inter.normal;
//     Vector3f emit = inter.emit;
//     Ray r = Ray(p0, ws);

//     float tNear;
//     uint32_t hitIndex;
//     Object* hitObject;

//     Vector3f L_dir = Vector3f(0.0f);
    
//     auto d = (x-p0).norm();
//     if (result && fabs(tNear - d) < 0.001f)
//     {
//         L_dir = emit * p.m->eval(ws, wo, p.normal) * dotProduct(ws, p.normal) * dotProduct(ws, NN)/d/d/pdf_light;
//     }
    
//     Vector3f L_indir = Vector3f(0.0f);
//     auto wi = p.m->sample(wo, p.normal);
//     r = Ray(p0, wi);
//     auto q = intersect(r);
//     if (q.happened && !q.obj->hasEmit())
//     {
//         auto q = p0 + wi*tNear;
//         L_indir = shade(q, wi) * p.m->eval(wi, wo, p.normal) * dotProduct(wi, p.normal) / p.m->pdf(wi, wo, p.normal)/RussianRoulette;
//     }

//     return L_dir + L_indir;
// }

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TO DO Implement Path Tracing Algorithm here
    Intersection p = intersect(ray);
    if (p.happened)
    {
        // 直接光计算
        Intersection inter;
        float pdf_light;
        sampleLight(inter, pdf_light);
        Vector3f x = inter.coords;
        Vector3f ws = normalize(x-p.coords);
        Vector3f NN = inter.normal;
        Vector3f emit = inter.emit;
        Ray r = Ray(p.coords, ws);
        Vector3f L_dir = Vector3f(0.0f);
        
        auto d = (x-p.coords).norm();
        inter = intersect(r);
        if (inter.happened && inter.obj->hasEmit())
        {
            L_dir = emit * p.m->eval(ws, ray.direction, p.normal) * dotProduct(ws, p.normal) * dotProduct(ws, NN)/d/d/pdf_light;
        }
        
        Vector3f L_indir = Vector3f(0.0f);
        if (get_random_float() < RussianRoulette)
        {
            auto wi = p.m->sample(ray.direction, p.normal);
            r = Ray(p.coords, wi);
            auto q = intersect(r);
            if (q.happened && !q.obj->hasEmit())
            {
                L_indir = castRay(r, depth) * p.m->eval(wi, r.direction, p.normal) * dotProduct(wi, p.normal) / p.m->pdf(wi, r.direction, p.normal)/RussianRoulette;
            }
        }
        

        return L_dir + L_indir;
    }

    return Vector3f(0.0f);
}