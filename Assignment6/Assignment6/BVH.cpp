#include <algorithm>
#include <cassert>
#include <math.h>
#include "BVH.hpp"

BVHAccel::BVHAccel(std::vector<Object*> p, int maxPrimsInNode,
                   SplitMethod splitMethod)
    : maxPrimsInNode(std::min(255, maxPrimsInNode)), splitMethod(splitMethod),
      primitives(std::move(p))
{
    time_t start, stop;
    time(&start);
    if (primitives.empty())
        return;

    root = recursiveBuild(primitives);

    time(&stop);
    double diff = difftime(stop, start);
    int hrs = (int)diff / 3600;
    int mins = ((int)diff / 60) - (hrs * 60);
    int secs = (int)diff - (hrs * 3600) - (mins * 60);

    printf(
        "\rBVH Generation complete: \nTime Taken: %i hrs, %i mins, %i secs\n\n",
        hrs, mins, secs);
}

BVHBuildNode* BVHAccel::recursiveBuild(std::vector<Object*> objects)
{
    BVHBuildNode* node = new BVHBuildNode();

    // Compute bounds of all primitives in BVH node
    Bounds3 bounds;
    for (int i = 0; i < objects.size(); ++i)
        bounds = Union(bounds, objects[i]->getBounds());
    if (objects.size() == 1) {
        // Create leaf _BVHBuildNode_
        node->bounds = objects[0]->getBounds();
        node->object = objects[0];
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }
    else if (objects.size() == 2) {
        node->left = recursiveBuild(std::vector{objects[0]});
        node->right = recursiveBuild(std::vector{objects[1]});

        node->bounds = Union(node->left->bounds, node->right->bounds);
        return node;
    }
    else {

        if (splitMethod == SplitMethod::NAIVE)
        {
            Bounds3 centroidBounds;
            for (int i = 0; i < objects.size(); ++i)
                centroidBounds =
                    Union(centroidBounds, objects[i]->getBounds().Centroid());
            int dim = centroidBounds.maxExtent();
            switch (dim) {
            case 0:
                std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                    return f1->getBounds().Centroid().x <
                        f2->getBounds().Centroid().x;
                });
                break;
            case 1:
                std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                    return f1->getBounds().Centroid().y <
                        f2->getBounds().Centroid().y;
                });
                break;
            case 2:
                std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                    return f1->getBounds().Centroid().z <
                        f2->getBounds().Centroid().z;
                });
                break;
            }

            auto beginning = objects.begin();
            auto middling = objects.begin() + (objects.size() / 2);
            auto ending = objects.end();

            auto leftshapes = std::vector<Object*>(beginning, middling);
            auto rightshapes = std::vector<Object*>(middling, ending);

            assert(objects.size() == (leftshapes.size() + rightshapes.size()));
            node->left = recursiveBuild(leftshapes);
            node->right = recursiveBuild(rightshapes);

            node->bounds = Union(node->left->bounds, node->right->bounds);
        }
        else
        {
            // 根据轴向获取obj中心的value
            auto getCenter = [](size_t axis, Object* obj) -> float {
                if (axis == 0) return obj->getBounds().Centroid().x;
                if (axis == 1) return obj->getBounds().Centroid().y;
                if (axis == 2) return obj->getBounds().Centroid().z;    
                return 0;            
            };
            
            // 根据轴向获取obj的最小值
            auto getMin = [](size_t axis, Object* obj) -> float{
                if (axis == 0) return obj->getBounds().pMin.x;
                if (axis == 1) return obj->getBounds().pMin.y;
                if (axis == 2) return obj->getBounds().pMin.z;
                return 0;
            };

            // 根据轴向获取obj的最大值
            auto getMax = [](size_t axis, Object* obj) -> float{
                if (axis == 0) return obj->getBounds().pMax.x;
                if (axis == 1) return obj->getBounds().pMax.y;
                if (axis == 2) return obj->getBounds().pMax.z;
                return 0;
            };

            struct Bucket
            {
                std::vector<Object*> objsInBucket;
                Bounds3 bounds;
            };
            
            float minC = std::numeric_limits<float>::infinity();
            std::vector<Object*> leftShapes;
            std::vector<Object*> rightShapes;

            for (size_t axis = 0; axis < 2; axis++)
            {
                float minValue = std::numeric_limits<float>::infinity();
                float maxValue = -std::numeric_limits<float>::infinity();
                for (size_t i = 0; i < objects.size(); i++)
                {
                    minValue = std::min(getMin(axis, objects[i]), minValue);
                    maxValue = std::max(getMax(axis, objects[i]), maxValue);  
                }
                
                // below 32, 初始化每个桶的object和bound
                int B = 30;
                float step = (maxValue - minValue)/B;
                auto buckets = std::vector<Bucket>(B);
                for (size_t i = 0; i < objects.size(); i++)
                {
                    float center = getCenter(axis, objects[i]);
                    int bucketIndex = std::min(int((center - minValue) / step), B-1);
                    
                    buckets[bucketIndex].objsInBucket.push_back(objects[i]);
                    buckets[bucketIndex].bounds = Union(buckets[bucketIndex].bounds, objects[i]->getBounds());
                }
                
                // 遍历B-1中分割方法, 找到C值最小的
                // C = boundsA.surfaceArea/totalBounds.surfaceArea * objCountA + boundsB.surfaceArea/totalBounds.surfaceArea * objCountB
                
                auto leftBounds = std::vector<Bounds3>(B-1);
                auto rightBounds = std::vector<Bounds3>(B-1);
                auto leftCounts = std::vector<int>(B-1);
                auto rightCounts = std::vector<int>(B-1);

                for (size_t i = 0; i < B-1; i++)
                {
                    if (i==0)
                    {
                        leftBounds[i] = buckets[i].bounds;
                        rightBounds[B-2-i] = buckets[B-1-i].bounds;

                        leftCounts[i] = buckets[i].objsInBucket.size();
                        rightCounts[B-2-i] = buckets[B-1-i].objsInBucket.size();
                    }
                    else
                    {
                        leftBounds[i] = Union(buckets[i].bounds, leftBounds[i-1]);
                        rightBounds[B-2-i] = Union(buckets[B-1-i].bounds, rightBounds[B-1-i]);

                        leftCounts[i] = leftCounts[i-1] + buckets[i].objsInBucket.size();
                        rightCounts[B-2-i] = buckets[B-1-i].objsInBucket.size() + rightCounts[B-1-i];
                    }
                }
                
                auto totalBounds = Union(leftBounds[0], rightBounds[0]);

                int breakIndex = 0;
                float c = std::numeric_limits<float>::infinity();
                for (size_t i = 0; i < B-1; i++)
                {
                    auto tempC = leftBounds[i].SurfaceArea() * leftCounts[i] / totalBounds.SurfaceArea()
                            + rightBounds[i].SurfaceArea() * rightCounts[i] / totalBounds.SurfaceArea();

                    if (tempC < c)
                    {
                        c = tempC;
                        breakIndex = i;
                    }
                }
                
                // 每个轴计算一次划分后的child objects
                if (c < minC)
                {
                    leftShapes.clear();
                    rightShapes.clear();
                    
                    for (size_t i = 0; i < B; i++)
                    {
                        if (i <= breakIndex)
                        {
                            leftShapes.insert(leftShapes.end(), buckets[i].objsInBucket.begin(), buckets[i].objsInBucket.end());
                        }
                        else
                        {
                            rightShapes.insert(rightShapes.end(), buckets[i].objsInBucket.begin(), buckets[i].objsInBucket.end());
                        }
                    }   
                }
            }

            assert(objects.size() == (leftShapes.size() + rightShapes.size()));
            node->left = recursiveBuild(leftShapes);
            node->right = recursiveBuild(rightShapes);

            node->bounds = Union(node->left->bounds, node->right->bounds);
        }

    }

    return node;
}

Intersection BVHAccel::Intersect(const Ray& ray) const
{
    Intersection isect;
    if (!root)
        return isect;
    isect = BVHAccel::getIntersection(root, ray);
    return isect;
}

Intersection BVHAccel::getIntersection(BVHBuildNode* node, const Ray& ray) const
{    
    // TODO Traverse the BVH to find intersection
    if (!node || !node->bounds.IntersectP(ray, ray.direction_inv, {int(ray.direction.x > 0), int(ray.direction.y > 0), int(ray.direction.z > 0)}))
    {
        return Intersection();
    }
    
    if (node->object)
    {
        // leaf node
        return node->object->getIntersection(ray);
    }

    auto hit1 = getIntersection(node->left, ray);
    auto hit2 = getIntersection(node->right, ray);
    
    if (hit1.distance < hit2.distance)
    {
        return hit1;
    }
    else
    {
        return hit2;
    }
}