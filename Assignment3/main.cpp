#include <iostream>
#include <opencv2/opencv.hpp>

#include "global.hpp"
#include "rasterizer.hpp"
#include "Triangle.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "OBJ_Loader.h"

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1,0,0,-eye_pos[0],
                 0,1,0,-eye_pos[1],
                 0,0,1,-eye_pos[2],
                 0,0,0,1;

    view = view*translate;

    return view;
}

Eigen::Matrix4f get_model_matrix(float angle)
{
    Eigen::Matrix4f rotation;
    angle = angle * MY_PI / 180.f;
    rotation << cos(angle), 0, sin(angle), 0,
                0, 1, 0, 0,
                -sin(angle), 0, cos(angle), 0,
                0, 0, 0, 1;

    Eigen::Matrix4f scale;
    scale << 10, 0, 0, 0,
              0, 10, 0, 0,
              0, 0, 10, 0,
              0, 0, 0, 1;

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1;

    return translate * rotation * scale;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio, float zNear, float zFar)
{
    float n = -zNear;
    float f = -zFar;

   // Students will implement this function
    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the projection matrix for the given parameters.
    // Then return it.
    projection <<   n, 0, 0, 0,
                    0, n, 0, 0,
                    0, 0, n + f, -f*n,
                    0, 0, 1, 0;

    // orth matrix    
    Eigen::Matrix4f orth = Eigen::Matrix4f::Identity();
    eye_fov = eye_fov * MY_PI / 180;
    float b = tanf(eye_fov/2) * n;
    float t = -b;
    float l = b * aspect_ratio;
    float r = -l;
    
    orth << 2/(r-l), 0, 0, -(r+l)/(r-l),
            0, 2/(t-b), 0, -(t+b)/(t-b),
            0, 0, 2/(n-f), -(n+f)/(n-f),
            0, 0, 0, 1;
    return orth * projection;
}

Eigen::Vector3f vertex_shader(const vertex_shader_payload& payload)
{
    return payload.position;
}

Eigen::Vector3f normal_fragment_shader(const fragment_shader_payload& payload)
{
    // auto return_color = payload.normal.head<3>().normalized();

    Eigen::Vector3f return_color = (payload.normal.head<3>().normalized() + Eigen::Vector3f(1.0f, 1.0f, 1.0f)) / 2.f;
    Eigen::Vector3f result;
    result << return_color.x() * 255, return_color.y() * 255, return_color.z() * 255;
    return result;
}

static Eigen::Vector3f reflect(const Eigen::Vector3f& vec, const Eigen::Vector3f& axis)
{
    auto costheta = vec.dot(axis);
    return (2 * costheta * axis - vec).normalized();
}

struct light
{
    Eigen::Vector3f position;
    Eigen::Vector3f intensity;
};

Eigen::Vector3f texture_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f return_color = {0, 0, 0};

    Eigen::Vector3f texture_color;
    texture_color << return_color.x(), return_color.y(), return_color.z();

    if (payload.texture)
    {
        // TODO: Get the texture value at the texture coordinates of the current fragment
        // texture_color = payload.texture->getColor(payload.tex_coords.x(), payload.tex_coords.y());
        texture_color = payload.texture->getColorBiLinear(payload.tex_coords.x(), payload.tex_coords.y());
    }

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = texture_color / 255.f;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = texture_color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};

    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *result_color* object.
        float r2 = (point - light.position).norm() * (point - light.position).norm();
        auto l = (light.position - point).normalized();
        float ndotl = normal.dot(l) < 0 ? 0 : normal.dot(l);
        auto view = (eye_pos - point).normalized();
        auto half = (view + l).normalized();
        float ndoth = normal.dot(half) < 0 ? 0 : normal.dot(half);

        result_color += ka.cwiseProduct(amb_light_intensity) + kd.cwiseProduct(light.intensity) / r2 * ndotl + ks.cwiseProduct(light.intensity)/r2 * powf(ndoth, p);
    }

    return result_color * 255.f;
}

Eigen::Vector3f phong_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};
    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *result_color* object.
        
        float r2 = (point - light.position).norm() * (point - light.position).norm();
        auto l = (light.position - point).normalized();
        float ndotl = normal.dot(l) < 0 ? 0 : normal.dot(l);
        auto view = (eye_pos - point).normalized();
        auto half = (view + l).normalized();
        float ndoth = normal.dot(half) < 0 ? 0 : normal.dot(half);

        result_color += ka.cwiseProduct(amb_light_intensity) + kd.cwiseProduct(light.intensity) / r2 * ndotl + ks.cwiseProduct(light.intensity)/r2 * powf(ndoth, p);
    }

    return result_color * 255.f;
}



Eigen::Vector3f displacement_fragment_shader(const fragment_shader_payload& payload)
{
    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color; 
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    float kh = 0.2, kn = 0.1;
    
    // TODO: Implement displacement mapping here
    // Let n = normal = (x, y, z)
    // Vector t = (x*y/sqrt(x*x+z*z),sqrt(x*x+z*z),z*y/sqrt(x*x+z*z))
    // Vector b = n cross product t
    // Matrix TBN = [t b n]
    // dU = kh * kn * (h(u+1/w,v)-h(u,v))
    // dV = kh * kn * (h(u,v+1/h)-h(u,v))
    // Vector ln = (-dU, -dV, 1)
    // Position p = p + kn * n * h(u,v)
    // Normal n = normalize(TBN * ln)
    auto n = payload.normal;
    auto t = Eigen::Vector3f(n.x()*n.y()/sqrt(n.x()*n.x()+n.z()*n.z()),sqrt(n.x()*n.x()+n.z()*n.z()),n.z()*n.y()/sqrt(n.x()*n.x()+n.z()*n.z()));
    auto b = n.cross(t);
    Eigen::Matrix3f TBN;
    TBN <<  t.x(), b.x(), n.x(),
            t.y(), b.y(), n.y(),
            t.z(), b.z(), n.z();

    float u = payload.tex_coords.x();
    float v = payload.tex_coords.y();
    float w = payload.texture->width;
    float h = payload.texture->height;

    auto dU = kh * kn * (payload.texture->getColor(u+1/w, v).norm() - payload.texture->getColor(u, v).norm());
    auto dV = kh * kn * (payload.texture->getColor(u,v+1/h).norm() - payload.texture->getColor(u,v).norm());
    auto ln = Vector3f(-dU, -dV, 1.0f);
    point = point + kn * normal * payload.texture->getColor(u,v).norm();
    normal = (TBN * ln).normalized();

    Eigen::Vector3f result_color = {0, 0, 0};

    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *result_color* object.
        float r2 = (point - light.position).norm() * (point - light.position).norm();
        auto l = (light.position - point).normalized();
        float ndotl = normal.dot(l) < 0 ? 0 : normal.dot(l);
        auto view = (eye_pos - point).normalized();
        auto half = (view + l).normalized();
        float ndoth = normal.dot(half) < 0 ? 0 : normal.dot(half);

        result_color += ka.cwiseProduct(amb_light_intensity) + kd.cwiseProduct(light.intensity) / r2 * ndotl + ks.cwiseProduct(light.intensity)/r2 * powf(ndoth, p);

    }

    return result_color * 255.f;
}


Eigen::Vector3f bump_fragment_shader(const fragment_shader_payload& payload)
{
    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color; 
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;


    float kh = 0.2, kn = 0.1;

    // TODO: Implement bump mapping here
    // Let n = normal = (x, y, z)
    // Vector t = (x*y/sqrt(x*x+z*z),sqrt(x*x+z*z),z*y/sqrt(x*x+z*z))
    // Vector b = n cross product t
    // Matrix TBN = [t b n]
    // dU = kh * kn * (h(u+1/w,v)-h(u,v))
    // dV = kh * kn * (h(u,v+1/h)-h(u,v))
    // Vector ln = (-dU, -dV, 1)
    // Normal n = normalize(TBN * ln)

    auto n = payload.normal;
    auto t = Eigen::Vector3f(n.x()*n.y()/sqrt(n.x()*n.x()+n.z()*n.z()),sqrt(n.x()*n.x()+n.z()*n.z()),n.z()*n.y()/sqrt(n.x()*n.x()+n.z()*n.z()));
    auto b = n.cross(t);
    Eigen::Matrix3f TBN;
    TBN <<  t.x(), b.x(), n.x(),
            t.y(), b.y(), n.y(),
            t.z(), b.z(), n.z();

    float u = payload.tex_coords.x();
    float v = payload.tex_coords.y();
    float w = payload.texture->width;
    float h = payload.texture->height;

    auto dU = kh * kn * (payload.texture->getColor(u+1/w, v).norm() - payload.texture->getColor(u, v).norm());
    auto dV = kh * kn * (payload.texture->getColor(u,v+1/h).norm() - payload.texture->getColor(u,v).norm());
    auto ln = Vector3f(-dU, -dV, 1.0f);
    normal = (TBN * ln).normalized();

    // Eigen::Vector3f return_color = (normal + Eigen::Vector3f(1.0f, 1.0f, 1.0f)) / 2.f;
    Eigen::Vector3f return_color = normal;
    return return_color * 255.f;
}

int main(int argc, const char** argv)
{
    std::vector<Triangle*> TriangleList;

    float angle = 135.0;
    bool command_line = false;

    std::string filename = "output.png";
    objl::Loader Loader;
    std::string obj_path = "../models/spot/";

    // Load .obj File
    // bool loadout = Loader.LoadFile("../models/spot/spot_triangulated_good.obj");
    bool loadout = Loader.LoadFile("../models/bunny/bunny.obj");
    // bool loadout = Loader.LoadFile("../models/cube/cube.obj");
    // bool loadout = Loader.LoadFile("../models/suzanne/suzanne.obj");

    std::cout << loadout << std::endl;
    for(auto mesh:Loader.LoadedMeshes)
    {
        for(int i=0;i<mesh.Vertices.size();i+=3)
        {
            Triangle* t = new Triangle();
            for(int j=0;j<3;j++)
            {
                t->setVertex(j,Vector4f(mesh.Vertices[i+j].Position.X,mesh.Vertices[i+j].Position.Y,mesh.Vertices[i+j].Position.Z,1.0));
                t->setNormal(j,Vector3f(mesh.Vertices[i+j].Normal.X,mesh.Vertices[i+j].Normal.Y,mesh.Vertices[i+j].Normal.Z));
                t->setTexCoord(j,Vector2f(mesh.Vertices[i+j].TextureCoordinate.X, mesh.Vertices[i+j].TextureCoordinate.Y));
            }
            TriangleList.push_back(t);
        }
    }

    rst::rasterizer r(700, 700);
    auto texture_path = "hmap.jpg";
    r.set_texture(Texture(obj_path + texture_path));

    std::function<Eigen::Vector3f(fragment_shader_payload)> active_shader = phong_fragment_shader;

    if (argc >= 2)
    {
        command_line = true;
        filename = std::string(argv[1]);

        if (argc >= 3 && std::string(argv[2]) == "texture")
        {
            std::cout << "Rasterizing using the texture shader\n";
            active_shader = texture_fragment_shader;
            texture_path = "spot_texture.png";
            if (argc >= 4)
            {
                texture_path = argv[3];
            }
            
            std::cout << texture_path;
            r.set_texture(Texture(obj_path + texture_path));
        }
        else if (argc == 3 && std::string(argv[2]) == "normal")
        {
            std::cout << "Rasterizing using the normal shader\n";
            active_shader = normal_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "phong")
        {
            std::cout << "Rasterizing using the phong shader\n";
            active_shader = phong_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "bump")
        {
            std::cout << "Rasterizing using the bump shader\n";
            active_shader = bump_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "displacement")
        {
            std::cout << "Rasterizing using the bump shader\n";
            active_shader = displacement_fragment_shader;
        }
    }
    else
    {
        active_shader = normal_fragment_shader;
    }
    
    Eigen::Vector3f eye_pos = {0,0,10};

    r.set_vertex_shader(vertex_shader);
    r.set_fragment_shader(active_shader);

    int key = 0;
    int frame_count = 0;

    auto viewMatrix = get_view_matrix(eye_pos);
    auto projectionMatrix = get_projection_matrix(45.0, 1, 0.1, 50);

    auto vp = projectionMatrix * viewMatrix;

    std::cout << "inverse view:" << std::endl;
    std::cout << viewMatrix.inverse() << std::endl;

    std::cout << "v:" << std::endl;
    std::cout << viewMatrix << std::endl;

    std::cout << "p:" << std::endl;
    std::cout << projectionMatrix << std::endl;

    std::cout << "vp:" << std::endl;
    std::cout << vp << std::endl;

    if (command_line)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);
        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));

        r.draw(TriangleList);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imwrite(filename, image);

        return 0;
    }

    while(key != 27)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));

        r.draw(TriangleList);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imshow("image", image);
        cv::imwrite(filename, image);
        key = cv::waitKey(10);

        if (key == 'a' )
        {
            angle -= 0.1;
        }
        else if (key == 'd')
        {
            angle += 0.1;
        }

    }
    return 0;
}
