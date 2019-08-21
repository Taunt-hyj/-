#include "ArmorDetector.h"
#include <cmath>
#include <iostream>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/core/core.hpp> 
#include <opencv2/opencv.hpp>
#include <vector>
using namespace cv;
using namespace std;

int main()
{
    Mat src; // ԭͼ
    Mat dst_BR; // �������Ⱥ�image
    Mat dst; // ��ֵ�����image
    vector<RotatedRect> vContour; // ���ֵ���ת����
    vector<RotatedRect> vRec; // ���ֵ�װ�װ�

	char Image_RED[] = "video\\1.MOV";
    char Image_BLUE[] = "video\\2.MOV";

    VideoCapture cap(Image_BLUE);
	//VideoWriter writer("..\\BLUE.avi", CAP_OPENCV_MJPEG, 25.0, Size(1920,1080));
    //src = imread("C:\\Users\\69022\\Desktop\\Rm\\text.png");
    namedWindow("original cap", CV_WINDOW_NORMAL);
    namedWindow("end cap1", CV_WINDOW_NORMAL);
    namedWindow("end cap2", CV_WINDOW_NORMAL);
    while (1) {
        cap >> src;
        if (src.empty()) {
            break;
        }
        imshow("original cap", src);
        //resize(src, src, Size(1470, 810));
        dst_BR = Adjust_Bright(src);
        dst = Threshold_Demo(dst_BR);
        bool vContour_BP = Found_Contour(dst, vContour);
        bool vRec_BP = 0;
        if (vContour_BP) {
            vRec_BP = Identify_board(dst, vContour, vRec);
        }
        if (vRec_BP)
            drawBox(vRec, src, dst);
        drawBox1(vContour, src, dst); // ������
        imshow("end cap1", dst);
		//writer << src;
        imshow("end cap2", src);
        waitKey(10);
    }
    waitKey(0);
    return 0;
}

Mat Adjust_Bright(Mat src)
{
    Mat dst_BR = Mat::zeros(src.size(), src.type());
    Mat BrightnessLut(1, 256, CV_8UC1);
    for (int i = 0; i < 256; i++) {
        BrightnessLut.at<uchar>(i) = saturate_cast<uchar>(i + _Armor.Image_bright);
    }
    LUT(src, BrightnessLut, dst_BR);
    return dst_BR;
}

Mat Threshold_Demo(Mat dst_BR)
{
    Mat dst, channels[3];
    // ����ͨ��
    split(dst_BR, channels);
	if (_Armor.Armor_Color)
		dst = (channels[2] - channels[1]) * 2;
    else
        dst = channels[0] - channels[2];
    blur(dst, dst, Size(1, 3));
    threshold(dst, dst, _Armor.threshold_value, 255, THRESH_BINARY); // ��ֵ��

    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3), Point(-1, -1));
    dilate(dst, dst, kernel); // ����
    return dst;
}

RotatedRect& adjustRec(RotatedRect& rec)
{
    while (rec.angle >= 90.0)
        rec.angle -= 180.0;
    while (rec.angle < -90.0)
        rec.angle += 180.0;
    if (rec.angle >= 45.0) {
        swap(rec.size.width, rec.size.height);
        rec.angle -= 90.0;
    } else if (rec.angle < -45.0) {
        swap(rec.size.width, rec.size.height);
        rec.angle += 90.0;
    }
    return rec;
}

bool Found_Contour(Mat dst, vector<RotatedRect>& vContour)
{
    vContour.clear();
    vector<vector<Point>> Light_Contour; // ���ֵ�����
    findContours(dst, Light_Contour, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE); // Ѱ������

    for (int i = 0; i < Light_Contour.size(); i++) {
        // ���������
        float Light_Contour_Area = contourArea(Light_Contour[i]);
        // ȥ����С����&fitEllipse����������
        if (Light_Contour_Area < _Armor.Light_Area_min || Light_Contour[i].size() <= 5)
            continue;
        // ����Բ�������õ���Ӿ���
        RotatedRect Light_Rec = fitEllipse(Light_Contour[i]);
        Light_Rec = adjustRec(Light_Rec);
        if (Light_Rec.angle > _Armor.Light_angle)
            continue;
        // ����Ⱥ�͹������
        if (Light_Rec.size.width / Light_Rec.size.height > _Armor.Light_Aspect_ratio
            || Light_Contour_Area / Light_Rec.size.area() < _Armor.Light_crown)
            continue;
        // ������������
        Light_Rec.size.height *= 1.1;
        Light_Rec.size.width *= 1.1;
        vContour.push_back(Light_Rec);
    }
    if (vContour.empty()) {
        cout << "not found Contour!!" << endl;
        return false;
    }
    if (vContour.size() < 2) {
        cout << "Contour is less!!" << endl;
        return false;
    }
    return true;
}

bool Identify_board(Mat dst, vector<RotatedRect>& vContour, vector<RotatedRect>& vRec)
{
    vRec.clear();
    for (int i = 0; i < vContour.size(); i++) {
        for (int j = i + 1; j < vContour.size(); j++) {

            //�ж��Ƿ�Ϊ��ͬ����
            float Contour_angle = abs(vContour[i].angle - vContour[j].angle); //�Ƕ�
            if (Contour_angle >= _Armor.Light_Contour_angle)
                continue;
            //���Ȳ����
            float Contour_Len1 = abs(vContour[i].size.height - vContour[j].size.height) / max(vContour[i].size.height, vContour[j].size.height);
            //��Ȳ����
            float Contour_Len2 = abs(vContour[i].size.width - vContour[j].size.width) / max(vContour[i].size.width, vContour[j].size.width);
            if (Contour_Len1 > _Armor.Light_Contour_Len || Contour_Len2 > _Armor.Light_Contour_Len)
                continue;
            //װ�װ�ƥ��
            RotatedRect Rect;
            Rect.center.x = (vContour[i].center.x + vContour[j].center.x) / 2.; //x����
            Rect.center.y = (vContour[i].center.y + vContour[j].center.y) / 2.; //y����
            Rect.angle = (vContour[i].angle + vContour[j].angle) / 2.; //�Ƕ�
            float nh, nw, yDiff, xDiff;
            nh = (vContour[i].size.height + vContour[j].size.height) / 2; //�߶�
            // ���
            nw = sqrt((vContour[i].center.x - vContour[j].center.x) * (vContour[i].center.x - vContour[j].center.x) + (vContour[i].center.y - vContour[j].center.y) * (vContour[i].center.y - vContour[j].center.y));
            float ratio = nw / nh; // ƥ�䵽��װ�װ�ĳ����
            xDiff = abs(vContour[i].center.x - vContour[j].center.x) / nh; //x�����
            yDiff = abs(vContour[i].center.y - vContour[j].center.y) / nh; //y�����
            Rect = adjustRec(Rect);
            if (Rect.angle > _Armor.Armor_angle_min || ratio < _Armor.Armor_ratio_min || ratio > _Armor.Armor_ratio_max || xDiff < _Armor.Armor_xDiff || yDiff > _Armor.Armor_yDiff)
                continue;
            Rect.size.height = nh;
            Rect.size.width = nw;
            vRec.push_back(Rect);
        }
    }
    if (vRec.empty()) {
        cout << "not found Rec!!" << endl;
        return false;
    }
    return true;
}

void drawBox1(vector<RotatedRect>& vRec, Mat& src, Mat& drc)
{
    Point2f pt[4];
    for (int i = 0; i < vRec.size(); i++) {
        RotatedRect Rect = vRec[i];
        Rect.points(pt);
        line(src, pt[0], pt[1], CV_RGB(255, 0, 0), 1, 8, 0);
        line(src, pt[1], pt[2], CV_RGB(255, 0, 0), 1, 8, 0);
        line(src, pt[2], pt[3], CV_RGB(255, 0, 0), 1, 8, 0);
        line(src, pt[3], pt[0], CV_RGB(255, 0, 0), 1, 8, 0);
        line(drc, pt[0], pt[1], CV_RGB(255, 0, 0), 1, 8, 0);
        line(drc, pt[1], pt[2], CV_RGB(255, 0, 0), 1, 8, 0);
        line(drc, pt[2], pt[3], CV_RGB(255, 0, 0), 1, 8, 0);
        line(drc, pt[3], pt[0], CV_RGB(255, 0, 0), 1, 8, 0);
    }
}

void drawBox(vector<RotatedRect>& vRec, Mat& src, Mat& drc)
{
    Point2f pt[4];
    for (int i = 0; i < vRec.size(); i++) {
        RotatedRect Rect = vRec[i];
        Rect.points(pt);
        line(src, pt[0], pt[1], CV_RGB(255, 0, 255), 2, 8, 0);
        line(src, pt[1], pt[2], CV_RGB(255, 0, 255), 2, 8, 0);
        line(src, pt[2], pt[3], CV_RGB(255, 0, 255), 2, 8, 0);
        line(src, pt[3], pt[0], CV_RGB(255, 0, 255), 2, 8, 0);
        line(drc, pt[0], pt[1], CV_RGB(255, 0, 255), 2, 8, 0);
        line(drc, pt[1], pt[2], CV_RGB(255, 0, 255), 2, 8, 0);
        line(drc, pt[2], pt[3], CV_RGB(255, 0, 255), 2, 8, 0);
        line(drc, pt[3], pt[0], CV_RGB(255, 0, 255), 2, 8, 0);
    }
}