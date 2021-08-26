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

// p 着色点，wo是入射光方向
Vector3f Scene::shade(Vector3f p, Vector3f wo) const
{
    // shade(p, wo)
    //  sampleLight(inter, pdf_light)
    //  Get x, ws, NN, emit from inter
    //  Shoot a ray from p to x
    //  If the ray is not blocked in the middle
    //      L_dir = emit * eval(wo, ws, N) * dot(ws, N) * dot(ws, NN) / |x-p|^2 / pdf_light

    // L_indir = 0.0
    // Test Russian Roulette with probability RussianRoulette wi = sample(wo, N)
    // Trace a ray r(p, wi)
    // If ray r hit a non-emitting object at q
    //      L_indir = shade(q, wi) * eval(wo, wi, N) * dot(wi, N) / pdf(wo, wi, N) / RussianRoulette
    // Return L_dir + L_indir

    // 直接光计算
    Intersection pos;
    float pdf_light;
    sampleLight(pos, pdf_light);
    Vector3f x = pos.coords;
    Vector3f ws = normalize(x-p);
    Vector3f NN = pos.normal;
    Vector3f emit = pos.emit;
    Ray r = Ray(p, ws);

    float tNear;
    uint32_t hitIndex;
    Object* hitObject;
    bool result = trace(r, objects, tNear, hitIndex, &hitObject);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TO DO Implement Path Tracing Algorithm here
    Intersection p = intersect(ray);
    if (p.happened)
    {
        return shade(p.coords, ray.direction);
    }

    return Vector3f(0.0f);
}