#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace cv;
using namespace std;

// 装甲板参数结构体
struct ArmorParam {
    // 预处理参数
    int brightness_threshold;
    int color_threshold;
    
    // 灯条筛选参数
    float light_min_area;
    float light_max_angle;
    float light_max_ratio;
    float light_contour_min_solidity;
    
    // 灯条匹配参数
    float light_max_angle_diff;
    float light_max_height_diff_ratio;
    float light_max_y_diff_ratio;
    float light_min_x_diff_ratio;
    
    // 装甲板参数
    float armor_big_armor_ratio;
    float armor_small_armor_ratio;
    float armor_min_aspect_ratio;
    float armor_max_aspect_ratio;
    
    // 默认构造函数
    ArmorParam() {
        brightness_threshold = 210;
        color_threshold = 40;
        light_min_area = 10;
        light_max_angle = 45.0;
        light_max_ratio = 1.0;
        light_contour_min_solidity = 0.5;
        light_max_angle_diff = 7.0;
        light_max_height_diff_ratio = 0.2;
        light_max_y_diff_ratio = 2.0;
        light_min_x_diff_ratio = 0.5;
        armor_big_armor_ratio = 3.2;
        armor_small_armor_ratio = 2.0;
        armor_min_aspect_ratio = 1.0;
        armor_max_aspect_ratio = 5.0;
    }
};

// 灯条描述符
class LightDescriptor {
public:
    LightDescriptor() {}
    LightDescriptor(const RotatedRect& light) {
        width = light.size.width;
        height = light.size.height;
        center = light.center;
        angle = light.angle;
        area = light.size.area();
    }
    
    RotatedRect rec() const {
        return RotatedRect(center, Size2f(width, height), angle);
    }
    
public:
    float width;
    float height;
    Point2f center;
    float angle;
    float area;
};

// 装甲板描述符
class ArmorDescriptor {
public:
    ArmorDescriptor() {}
    ArmorDescriptor(const LightDescriptor& leftLight, const LightDescriptor& rightLight, int armorType) {
        // 处理两个灯条
        lightPairs[0] = leftLight.rec();
        lightPairs[1] = rightLight.rec();
        
        // 计算装甲板的四个顶点
        Point2f leftTop, leftBottom, rightTop, rightBottom;
        
        // 扩展灯条高度
        Size exLSize(int(lightPairs[0].size.width), int(lightPairs[0].size.height * 2));
        Size exRSize(int(lightPairs[1].size.width), int(lightPairs[1].size.height * 2));
        RotatedRect exLLight(lightPairs[0].center, exLSize, lightPairs[0].angle);
        RotatedRect exRLight(lightPairs[1].center, exRSize, lightPairs[1].angle);
        
        // 获取扩展灯条的顶点
        Point2f pts_l[4];
        exLLight.points(pts_l);
        leftTop = pts_l[2];
        leftBottom = pts_l[3];
        
        Point2f pts_r[4];
        exRLight.points(pts_r);
        rightTop = pts_r[1];
        rightBottom = pts_r[0];
        
        // 设置装甲板顶点
        vertex.resize(4);
        vertex[0] = leftTop;      // 左上
        vertex[1] = rightTop;     // 右上
        vertex[2] = rightBottom;  // 右下
        vertex[3] = leftBottom;   // 左下
        
        // 设置装甲板类型
        type = armorType;
        
        // 计算装甲板中心
        center = (leftLight.center + rightLight.center) / 2;
        
        // 计算边界框
        vector<Point> contour;
        for (const auto& v : vertex) {
            contour.emplace_back(Point(v.x, v.y));
        }
        boundingRect = cv::boundingRect(contour);
    }
    
public:
    array<RotatedRect, 2> lightPairs;  // 两个灯条
    vector<Point2f> vertex;            // 装甲板的四个顶点
    Point2f center;                    // 装甲板中心
    Rect boundingRect;                 // 装甲板边界框
    int type;                          // 装甲板类型：0-小装甲板，1-大装甲板
};

// 调整旋转矩形
RotatedRect& adjustRec(RotatedRect& rec, const int mode) {
    float& width = rec.size.width;
    float& height = rec.size.height;
    float& angle = rec.angle;
    
    if (mode == 0) {  // WIDTH_GREATER_THAN_HEIGHT
        if (width < height) {
            swap(width, height);
            angle += 90.0;
        }
    }
    
    while (angle >= 90.0) angle -= 180.0;
    while (angle < -90.0) angle += 180.0;
    
    if (mode == 1) {  // ANGLE_TO_UP
        if (angle >= 45.0) {
            swap(width, height);
            angle -= 90.0;
        }
        else if (angle < -45.0) {
            swap(width, height);
            angle += 90.0;
        }
    }
    
    return rec;
}

// 计算两点间距离
float distance(const Point2f& p1, const Point2f& p2) {
    return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

// 提取灯条
vector<LightDescriptor> extractLights(const Mat& srcImg, int enemy_color, const ArmorParam& param) {
    vector<LightDescriptor> lights;
    
    // 转换为灰度图
    Mat grayImg;
    cvtColor(srcImg, grayImg, COLOR_BGR2GRAY);
    
    // 亮度阈值化
    Mat binBrightImg;
    threshold(grayImg, binBrightImg, param.brightness_threshold, 255, THRESH_BINARY);
    
    // 形态学操作
    Mat element = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
    dilate(binBrightImg, binBrightImg, element);
    
    // 查找轮廓
    vector<vector<Point>> contours;
    findContours(binBrightImg.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    
    for (const auto& contour : contours) {
        // 过滤小轮廓
        if (contour.size() <= 5 || contourArea(contour) < param.light_min_area) {
            continue;
        }
        
        // 椭圆拟合
        RotatedRect lightRec = fitEllipse(contour);
        adjustRec(lightRec, 1);  // ANGLE_TO_UP
        
        // 过滤长宽比
        if (lightRec.size.width / lightRec.size.height > param.light_max_ratio) {
            continue;
        }
        
        // 过滤凸度
        float solidity = contourArea(contour) / lightRec.size.area();
        if (solidity < param.light_contour_min_solidity) {
            continue;
        }
        
        // 扩展灯条区域用于颜色检测
        lightRec.size.width *= 1.1;
        lightRec.size.height *= 1.1;
        Rect lightRect = lightRec.boundingRect();
        Rect srcBound(Point(0, 0), srcImg.size());
        lightRect &= srcBound;
        
        // 提取灯条区域图像
        Mat lightImg = srcImg(lightRect);
        Mat lightMask = Mat::zeros(lightRect.size(), CV_8UC1);
        Point2f lightVertexArray[4];
        lightRec.points(lightVertexArray);
        vector<Point> lightVertex;
        for (int i = 0; i < 4; i++) {
            lightVertex.emplace_back(Point(lightVertexArray[i].x - lightRect.tl().x,
                                         lightVertexArray[i].y - lightRect.tl().y));
        }
        fillConvexPoly(lightMask, lightVertex, 255);
        dilate(lightMask, lightMask, element);
        
        // 计算灯条区域的平均颜色
        Scalar meanVal = mean(lightImg, lightMask);
        
        // 颜色判断
        bool isEnemyColor = false;
        if (enemy_color == 0) {  // 蓝色
            isEnemyColor = (meanVal[0] - meanVal[2] > param.color_threshold);
        } else {  // 红色
            isEnemyColor = (meanVal[2] - meanVal[0] > param.color_threshold);
        }
        
        if (isEnemyColor) {
            // 恢复原始灯条大小
            lightRec.size.width /= 1.1;
            lightRec.size.height /= 1.1;
            lights.push_back(LightDescriptor(lightRec));
        }
    }
    
    return lights;
}

// 匹配灯条对
vector<ArmorDescriptor> matchLights(const vector<LightDescriptor>& lights, const ArmorParam& param) {
    vector<ArmorDescriptor> armors;
    
    // 按x坐标排序
    vector<LightDescriptor> sortedLights = lights;
    sort(sortedLights.begin(), sortedLights.end(), 
         [](const LightDescriptor& ld1, const LightDescriptor& ld2) {
             return ld1.center.x < ld2.center.x;
         });
    
    // 两两匹配
    for (size_t i = 0; i < sortedLights.size(); i++) {
        for (size_t j = i + 1; j < sortedLights.size(); j++) {
            const LightDescriptor& leftLight = sortedLights[i];
            const LightDescriptor& rightLight = sortedLights[j];
            
            // 计算角度差
            float angleDiff = abs(leftLight.angle - rightLight.angle);
            if (angleDiff > param.light_max_angle_diff) {
                continue;
            }
            
            // 计算长度差比例
            float lenDiffRatio = abs(leftLight.height - rightLight.height) / 
                                 max(leftLight.height, rightLight.height);
            if (lenDiffRatio > param.light_max_height_diff_ratio) {
                continue;
            }
            
            // 计算位置关系
            float dis = distance(leftLight.center, rightLight.center);
            float meanLen = (leftLight.height + rightLight.height) / 2;
            float yDiff = abs(leftLight.center.y - rightLight.center.y);
            float yDiffRatio = yDiff / meanLen;
            float xDiff = abs(leftLight.center.x - rightLight.center.x);
            float xDiffRatio = xDiff / meanLen;
            float ratio = dis / meanLen;
            
            // 过滤位置关系
            if (yDiffRatio > param.light_max_y_diff_ratio ||
                xDiffRatio < param.light_min_x_diff_ratio ||
                ratio > param.armor_max_aspect_ratio ||
                ratio < param.armor_min_aspect_ratio) {
                continue;
            }
            
            // 判断装甲板类型
            int armorType = (ratio > param.armor_big_armor_ratio) ? 1 : 0;
            
            // 创建装甲板描述符
            ArmorDescriptor armor(leftLight, rightLight, armorType);
            armors.push_back(armor);
            
            // 每个灯条只匹配一次
            break;
        }
    }
    
    return armors;
}

// 绘制检测结果
Mat drawDetectionResult(const Mat& img, const vector<ArmorDescriptor>& armors) {
    Mat result = img.clone();
    
    for (const auto& armor : armors) {
        // 绘制装甲板顶点
        for (size_t i = 0; i < armor.vertex.size(); i++) {
            circle(result, armor.vertex[i], 3, Scalar(0, 255, 0), -1);
            if (i < armor.vertex.size() - 1) {
                line(result, armor.vertex[i], armor.vertex[i + 1], Scalar(0, 255, 0), 2);
            } else {
                line(result, armor.vertex[i], armor.vertex[0], Scalar(0, 255, 0), 2);
            }
        }
        
        // 绘制装甲板边界框
        rectangle(result, armor.boundingRect, Scalar(0, 255, 0), 2);
        
        // 绘制装甲板中心
        circle(result, armor.center, 5, Scalar(0, 0, 255), -1);
        
        // 添加装甲板类型标签
        string typeText = (armor.type == 1) ? "Big Armor" : "Small Armor";
        putText(result, typeText, Point(armor.boundingRect.x, armor.boundingRect.y - 10),
                FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 1);
    }
    
    return result;
}

int main() {
    // 读取图像
    Mat img = imread("../resources/test_image_2.png");
    if (img.empty()) {
        cout << "无法加载图像！" << endl;
        return -1;
    }
    
    // 设置参数
    ArmorParam param;
    int enemy_color = 0;  // 0: 蓝色, 1: 红色
    
    // 提取灯条
    vector<LightDescriptor> lights = extractLights(img, enemy_color, param);
    cout << "提取到 " << lights.size() << " 个灯条" << endl;
    
    // 匹配装甲板
    vector<ArmorDescriptor> armors = matchLights(lights, param);
    cout << "匹配到 " << armors.size() << " 个装甲板" << endl;
    
    // 绘制检测结果
    Mat result = drawDetectionResult(img, armors);
    
    // 显示结果
    namedWindow("装甲板检测结果", WINDOW_AUTOSIZE);
    imshow("装甲板检测结果", result);
    
    // 保存结果
    imwrite("../output/armor_detection_v2_result.jpg", result);
    
    // 等待按键
    waitKey(0);
    
    return 0;
}