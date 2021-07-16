//
// Created by LEI XU on 4/27/19.
//

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H
#include "global.hpp"
#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>
#include <math.h>

class Texture{
private:
    cv::Mat image_data;

public:
    Texture(const std::string& name)
    {
        image_data = cv::imread(name);
        cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
        width = image_data.cols;
        height = image_data.rows;
    }

    int width, height;

    Eigen::Vector3f getColor(float u, float v)
    {
        auto u_img = u * width;
        auto v_img = (1 - v) * height;
        auto color = image_data.at<cv::Vec3b>(v_img, u_img);
        return Eigen::Vector3f(color[0], color[1], color[2]);
    }
    
    cv::Vec3b lerp(float x, cv::Vec3b v0, cv::Vec3b v1)
    {
        return v0 + (v1-v0) * x;
    }

    Eigen::Vector3f getColorBiLinear(float u, float v)
    {
        auto u_img = u * width;
        auto v_img = (1 - v) * height;
        
        int v0 = floorf(v_img);
        int v1 = ceilf(v_img);

        int u0 = floorf(u_img);
        int u1 = ceilf(u_img);

        float pv = v_img - v0;
        float pu = u_img - u0;

        auto color00 = image_data.at<cv::Vec3b>(v0, u0);
        auto color10 = image_data.at<cv::Vec3b>(v0, u1);
        auto c0 = lerp(pu, color00, color10);

        auto color01 = image_data.at<cv::Vec3b>(v1, u0);
        auto color11 = image_data.at<cv::Vec3b>(v1, u1);
        auto c1 = lerp(pu, color01, color11);

        auto color = lerp(pv, c0, c1);

        // std::cout << v0 << " " << v1 << " " << pv << std::endl;
        // std::cout << u0 << " " << u1 << " " << pu << std::endl;
        // std::cout << "1" << color00 << " " << color10 << " " << c0 << std::endl;
        // std::cout << "2" << color01 << " " << color11 << " " << c1 << std::endl;
        // std::cout << "3" << c0 << " " << c1 << " " << color << std::endl;

        return Eigen::Vector3f(color[0], color[1], color[2]);
    }
};
#endif //RASTERIZER_TEXTURE_H
