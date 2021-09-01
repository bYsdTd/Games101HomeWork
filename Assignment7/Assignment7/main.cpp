#include "Renderer.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "Sphere.hpp"
#include "Vector.hpp"
#include "global.hpp"
#include <chrono>

// In the main function of the program, we create the scene (create objects and
// lights) as well as set the options for the render (image width and height,
// maximum recursion depth, field-of-view, etc.). We then call the render
// function().
int main(int argc, char** argv)
{

    // Change the definition here to change resolution
    Scene scene(784, 784);

    Material* red = new Material(DIFFUSE, Vector3f(0.0f));
    red->Kd = Vector3f(0.63f, 0.065f, 0.05f);
    Material* green = new Material(DIFFUSE, Vector3f(0.0f));
    green->Kd = Vector3f(0.14f, 0.45f, 0.091f);
    Material* white = new Material(DIFFUSE, Vector3f(0.0f));
    white->Kd = Vector3f(0.725f, 0.71f, 0.68f);
    Material* light = new Material(DIFFUSE, (8.0f * Vector3f(0.747f+0.058f, 0.747f+0.258f, 0.747f) + 15.6f * Vector3f(0.740f+0.287f,0.740f+0.160f,0.740f) + 18.4f *Vector3f(0.737f+0.642f,0.737f+0.159f,0.737f)));
    light->Kd = Vector3f(0.65f);

    // Material* microfacet_diffuse = new Material(DIFFUSE, Vector3f(0));
  
    Material* microfacet_diffuse1 = new Material(MICROFACET_DIFFUSE, Vector3f(0));
    microfacet_diffuse1->Ks = Vector3f(0.4, 0.4, 0.4);
    microfacet_diffuse1->Kd = Vector3f(0.2, 0.2, 0.05);
    microfacet_diffuse1->F0 = Vector3f(1.00, 0.71, 0.29);
    microfacet_diffuse1->smoothness = 0.3f;

    Material* microfacet_diffuse2 = new Material(MICROFACET_DIFFUSE, Vector3f(0));
    microfacet_diffuse2->Ks = Vector3f(0.4, 0.4, 0.4);
    microfacet_diffuse2->Kd = Vector3f(0.3, 0.3, 0.05);
    microfacet_diffuse2->F0 = Vector3f(0.95, 0.93, 0.88);
    microfacet_diffuse2->smoothness = 0.3f;

    MeshTriangle floor("../models/cornellbox/floor.obj", white);
    MeshTriangle shortbox("../models/cornellbox/shortbox.obj", white);
    MeshTriangle tallbox("../models/cornellbox/tallbox.obj", white);
    MeshTriangle left("../models/cornellbox/left.obj", red);
    MeshTriangle right("../models/cornellbox/right.obj", green);
    MeshTriangle light_("../models/cornellbox/light.obj", light);
    // x: right to left (0, 552), y: bottom to top(0, 548), z: back to front(0, 559) 
    Sphere sphere1(Vector3f(150, 100, 400), 100, microfacet_diffuse1);
    Sphere sphere2(Vector3f(400, 100, 300), 100, microfacet_diffuse2);

    scene.Add(&floor);

    // scene.Add(&shortbox);
    // scene.Add(&tallbox);
    
    scene.Add(&left);
    scene.Add(&right);
    scene.Add(&light_);
    
    scene.Add(&sphere1);
    scene.Add(&sphere2);

    scene.buildBVH();

    Renderer r;

    auto start = std::chrono::system_clock::now();
    r.Render(scene);
    auto stop = std::chrono::system_clock::now();

    std::cout << "Render complete: \n";
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::hours>(stop - start).count() << " hours\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::minutes>(stop - start).count() << " minutes\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() << " seconds\n";

    return 0;
}