#include <opencv2/opencv.hpp>
#include <iostream>
#include <sys/stat.h>

using namespace cv;
using namespace std;

// 创建输出目录的函数
void createOutputDirectory() {
    mkdir("../output", 0777);
}

int main() {
    // 创建输出目录
    createOutputDirectory();
    
    // 读取图像
    Mat img = imread("../resources/cosmos-7585071_1280.jpg");
    if (img.empty()) {
        cout << "无法加载图像！" << endl;
        return -1;
    }

    // 创建窗口
    namedWindow("原始图像", WINDOW_AUTOSIZE);
    imshow("原始图像", img);
    
    // 保存原始图像
    imwrite("../output/01_original_image.jpg", img);

    // 1. 图像颜色空间转换
    // 转换为灰度图
    Mat grayImg;
    cvtColor(img, grayImg, COLOR_BGR2GRAY);
    namedWindow("灰度图", WINDOW_AUTOSIZE);
    imshow("灰度图", grayImg);
    imwrite("../output/02_grayscale_image.jpg", grayImg);

    // 转换为HSV图像
    Mat hsvImg;
    cvtColor(img, hsvImg, COLOR_BGR2HSV);
    namedWindow("HSV图像", WINDOW_AUTOSIZE);
    imshow("HSV图像", hsvImg);
    imwrite("../output/03_hsv_image.jpg", hsvImg);

    // 2. 应用各种滤波操作
    // 应用均值滤波
    Mat blurImg;
    blur(img, blurImg, Size(5, 5));
    namedWindow("均值滤波", WINDOW_AUTOSIZE);
    imshow("均值滤波", blurImg);
    imwrite("../output/04_blur_image.jpg", blurImg);

    // 应用高斯滤波
    Mat gaussianImg;
    GaussianBlur(img, gaussianImg, Size(5, 5), 1.5);
    namedWindow("高斯滤波", WINDOW_AUTOSIZE);
    imshow("高斯滤波", gaussianImg);
    imwrite("../output/05_gaussian_blur_image.jpg", gaussianImg);

    // 3. 特征提取
    // 提取红色颜色区域 - HSV方法
    Mat redMask;
    inRange(hsvImg, Scalar(0, 100, 100), Scalar(10, 255, 255), redMask); // 红色低范围
    Mat redMask2;
    inRange(hsvImg, Scalar(160, 100, 100), Scalar(180, 255, 255), redMask2); // 红色高范围
    Mat finalRedMask = redMask | redMask2; // 合并两个红色范围
    
    namedWindow("红色区域掩码", WINDOW_AUTOSIZE);
    imshow("红色区域掩码", finalRedMask);
    imwrite("../output/06_red_mask.jpg", finalRedMask);

    // 寻找图像中红色的外轮廓
    vector<vector<Point>> redContours;
    vector<Vec4i> redHierarchy;
    findContours(finalRedMask, redContours, redHierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    
    Mat redContourImg = img.clone();
    for (size_t i = 0; i < redContours.size(); i++) {
        drawContours(redContourImg, redContours, (int)i, Scalar(0, 255, 0), 2); // 用绿色绘制轮廓
    }
    namedWindow("红色外轮廓", WINDOW_AUTOSIZE);
    imshow("红色外轮廓", redContourImg);
    imwrite("../output/07_red_contours.jpg", redContourImg);

    // 寻找图像中红色的bounding box
    Mat redBboxImg = img.clone();
    for (size_t i = 0; i < redContours.size(); i++) {
        Rect bbox = boundingRect(redContours[i]);
        rectangle(redBboxImg, bbox, Scalar(255, 0, 0), 2); // 用蓝色绘制bounding box
    }
    namedWindow("红色Bounding Box", WINDOW_AUTOSIZE);
    imshow("红色Bounding Box", redBboxImg);
    imwrite("../output/08_red_bounding_boxes.jpg", redBboxImg);

    // 计算轮廓的面积
    for (size_t i = 0; i < redContours.size(); i++) {
        double area = contourArea(redContours[i]);
        cout << "红色轮廓 " << i << " 的面积: " << area << endl;
    }

    // 提取高亮颜色区域并进行图形学处理
    // 灰度化
    Mat highlightGray;
    cvtColor(img, highlightGray, COLOR_BGR2GRAY);

    // 二值化（使用自适应阈值处理高亮区域）
    Mat binaryImg;
    adaptiveThreshold(highlightGray, binaryImg, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, 2);
    namedWindow("二值化", WINDOW_AUTOSIZE);
    imshow("二值化", binaryImg);
    imwrite("../output/09_binary_image.jpg", binaryImg);

    // 膨胀
    Mat dilateImg;
    Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));
    dilate(binaryImg, dilateImg, kernel);
    namedWindow("膨胀", WINDOW_AUTOSIZE);
    imshow("膨胀", dilateImg);
    imwrite("../output/10_dilated_image.jpg", dilateImg);

    // 腐蚀
    Mat erodeImg;
    erode(binaryImg, erodeImg, kernel);
    namedWindow("腐蚀", WINDOW_AUTOSIZE);
    imshow("腐蚀", erodeImg);
    imwrite("../output/11_eroded_image.jpg", erodeImg);

    // 对处理后的图像进行漫水处理
    Mat floodFillImg = img.clone();
    Point seedPoint(floodFillImg.cols / 2, floodFillImg.rows / 2);
    Scalar newVal(0, 255, 0); // 绿色
    floodFill(floodFillImg, seedPoint, newVal);
    namedWindow("漫水处理", WINDOW_AUTOSIZE);
    imshow("漫水处理", floodFillImg);
    imwrite("../output/12_flood_fill.jpg", floodFillImg);

    // 4. 图像绘制
    Mat drawingImg = img.clone();
    
    // 绘制任意圆形、方形和文字
    circle(drawingImg, Point(100, 100), 50, Scalar(255, 0, 0), 3); // 蓝色圆形
    rectangle(drawingImg, Point(200, 50), Point(300, 150), Scalar(0, 255, 0), 3); // 绿色方形
    putText(drawingImg, "OpenCV 图像处理", Point(50, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2); // 红色文字
    namedWindow("绘制图形和文字", WINDOW_AUTOSIZE);
    imshow("绘制图形和文字", drawingImg);
    imwrite("../output/13_drawing_shapes_text.jpg", drawingImg);

    // 绘制红色的外轮廓
    Mat redContourDrawImg = img.clone();
    for (size_t i = 0; i < redContours.size(); i++) {
        drawContours(redContourDrawImg, redContours, (int)i, Scalar(0, 0, 255), 2); // 用红色绘制轮廓
    }
    namedWindow("绘制红色外轮廓", WINDOW_AUTOSIZE);
    imshow("绘制红色外轮廓", redContourDrawImg);
    imwrite("../output/14_drawn_red_contours.jpg", redContourDrawImg);

    // 绘制红色的bounding box
    Mat redBboxDrawImg = img.clone();
    for (size_t i = 0; i < redContours.size(); i++) {
        Rect bbox = boundingRect(redContours[i]);
        rectangle(redBboxDrawImg, bbox, Scalar(0, 0, 255), 2); // 用红色绘制bounding box
    }
    namedWindow("绘制红色Bounding Box", WINDOW_AUTOSIZE);
    imshow("绘制红色Bounding Box", redBboxDrawImg);
    imwrite("../output/15_drawn_red_bounding_boxes.jpg", redBboxDrawImg);

    // 5. 对图像进行处理
    // 图像旋转35度
    Mat rotatedImg;
    Point2f center(img.cols / 2.0, img.rows / 2.0);
    Mat rotationMatrix = getRotationMatrix2D(center, 35, 1.0);
    warpAffine(img, rotatedImg, rotationMatrix, img.size());
    namedWindow("旋转35度", WINDOW_AUTOSIZE);
    imshow("旋转35度", rotatedImg);
    imwrite("../output/16_rotated_35_degrees.jpg", rotatedImg);

    // 图像裁剪为原图的左上角1/4
    Mat croppedImg = img(Rect(0, 0, img.cols / 2, img.rows / 2));
    namedWindow("裁剪左上角1/4", WINDOW_AUTOSIZE);
    imshow("裁剪左上角1/4", croppedImg);
    imwrite("../output/17_cropped_quarter.jpg", croppedImg);

    cout << "所有处理后的图像已保存到 ../output 目录" << endl;

    // 等待按键
    waitKey(0);

    return 0;
}
