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

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // shade(p, wo)
    //      sampleLight(inter , pdf_light)
    //      Get x, ws, NN, emit from inter
    //      Shoot a ray from p to x
    //      If the ray is not blocked in the middle
    //          L_dir = emit * eval(wo, ws, N) * dot(ws, N) * dot(ws, NN) / |x-p|^2 / pdf_light
    //      L_indir = 0.0
    //      Test Russian Roulette with probability RussianRoulette
    //      wi = sample(wo, N)
    //      Trace a ray r(p, wi)
    //      If ray r hit a non -emitting object at q
    //          L_indir = shade(q, wi) * eval(wo, wi, N) * dot(wi, N) / pdf(wo, wi, N) / RussianRoulette
    // 
    //      Return L_dir + L_indir

    // TO DO Implement Path Tracing Algorithm here
    Intersection p = intersect(ray);
    // 射线与场景求交，获取着色点p
    if (!p.happened)
    {
        return Vector3f(0.0f);
    }
    
    // 如果直接打到光源上，返回光源的L
    if (p.m->hasEmission())
    {
        return p.emit;
    }
    
    auto format = [](Vector3f &a) {
        if(a.x < 0) a.x = 0;
        if(a.y < 0) a.y = 0;
        if(a.z < 0) a.z = 0;
    };

    // 直接光计算
    Intersection inter;
    float pdf_light;
    // 采样光源上一点，并获取采样的pdf
    sampleLight(inter, pdf_light);

    Vector3f x = inter.coords;//光源上的点x
    Vector3f ws = normalize(x-p.coords);//p到光源x的方向
    Vector3f NN = normalize(inter.normal);//光源上的normal
    Vector3f emit = inter.emit;//光源上的Li
    Vector3f wo = normalize(-ray.direction);

    // p到光源的ray
    Ray r = Ray(p.coords, ws);
    Vector3f L_dir = Vector3f(0.0f);
    
    auto d = (x-p.coords).norm();
    inter = intersect(r);
    if (inter.happened && inter.obj->hasEmit())
    {
        // bxdf，材质表面的反射函数
        auto fr = p.m->eval(ws, wo, p.normal); 
        // 入射光与法线夹角
        auto costheta = dotProduct(ws, p.normal);
        // 与面光源法线的夹角
        auto costhetap = dotProduct(NN, -ws);
        L_dir = emit * fr * costheta * costhetap/d/d/pdf_light;
        format(L_dir);
    }
    
    // 间接光计算
    Vector3f L_indir = Vector3f(0.0f);
    if (get_random_float() < RussianRoulette)
    {
        // 着色点随机打出一根光线
        auto wi = p.m->sample(wo, p.normal);
        r = Ray(p.coords, wi);
        auto q = intersect(r);
        // 如果打到了不是光源的物体，就计算间接光
        if (q.happened && !q.obj->hasEmit())
        {
            L_indir = castRay(r, depth+1) * p.m->eval(wi, wo, p.normal) * dotProduct(wi, p.normal) / p.m->pdf(wi, wo, p.normal)/RussianRoulette;
        }
        format(L_indir);
    }
    
    return L_dir + L_indir;
    
}