#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <math.h>

std::vector<cv::Point2f> control_points;

void mouse_handler(int event, int x, int y, int flags, void *userdata) 
{
    if (event == cv::EVENT_LBUTTONDOWN && control_points.size() < 4) 
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", "
        << y << ")" << '\n';
        control_points.emplace_back(x, y);
    }     
}

void naive_bezier(const std::vector<cv::Point2f> &points, cv::Mat &window) 
{
    auto &p_0 = points[0];
    auto &p_1 = points[1];
    auto &p_2 = points[2];
    auto &p_3 = points[3];

    for (double t = 0.0; t <= 1.0; t += 0.001) 
    {
        auto point = std::pow(1 - t, 3) * p_0 + 3 * t * std::pow(1 - t, 2) * p_1 +
                 3 * std::pow(t, 2) * (1 - t) * p_2 + std::pow(t, 3) * p_3;

        window.at<cv::Vec3b>(point.y, point.x)[2] = 255;
    }
}

cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t) 
{
    // TODO: Implement de Casteljau's algorithm
    if (control_points.size() == 0)
    {
        return cv::Point2f();
    }
    
    if (control_points.size() == 1)
    {
        return control_points[0];
    }
    
    std::vector<cv::Point2f> next;

    for (size_t i = 0; i < control_points.size()-1; i++)
    {
        auto new_point = control_points[i] * (1-t) + control_points[i+1] * t;
        next.emplace_back(new_point);
    }

    return recursive_bezier(next, t);
}

void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window) 
{
    // TODO: Iterate through all t = 0 to t = 1 with small steps, and call de Casteljau's 
    // recursive Bezier algorithm.
    for (double t = 0.0; t <= 1.0; t += 0.001) 
    {
        auto point = recursive_bezier(control_points, t);
        int centerX = floor(point.x);
        float temp = point.x - centerX;
        if (temp > 0.5f)
        {
            centerX += 1;
        }
        int centerY = floor(point.y);
        temp = point.y - centerY;
        if (temp > 0.5f)
        {
            centerY += 1;
        }
        
        for (int xoffset = -1; xoffset <= 1; xoffset++)
        {
            for (int yoffset = -1; yoffset <=1; yoffset++)
            {
                int xPos = centerX + xoffset;
                int yPos = centerY + yoffset;
                if (xPos >= 0 && xPos < 700 && yPos >= 0 && yPos < 700)
                {
                    auto t1 = (point.x-xPos);
                    auto t2 = point.y-yPos;
                    float d = sqrt(t1*t1 + t2*t2);
                    int c = 255 * (1-d);
                    if (c < 0)
                    {
                        c = 0;
                    }
                    
                    int old = window.at<cv::Vec3b>(yPos, xPos)[1];
                    if (c > old)
                    {
                        window.at<cv::Vec3b>(yPos, xPos)[1] = c;    
                    }
                }
            }
        }
    }
}

int main() 
{
    cv::Mat window = cv::Mat(700, 700, CV_8UC3, cv::Scalar(0));
    cv::cvtColor(window, window, cv::COLOR_BGR2RGB);
    cv::namedWindow("Bezier Curve", cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);

    int key = -1;
    while (key != 27) 
    {
        for (auto &point : control_points) 
        {
            cv::circle(window, point, 3, {255, 255, 255}, 3);
        }

        if (control_points.size() == 4) 
        {
            // naive_bezier(control_points, window);
            bezier(control_points, window);

            cv::imshow("Bezier Curve", window);
            cv::imwrite("my_bezier_curve.png", window);
            key = cv::waitKey(0);

            return 0;
        }

        cv::imshow("Bezier Curve", window);
        key = cv::waitKey(20);
    }

return 0;
}
