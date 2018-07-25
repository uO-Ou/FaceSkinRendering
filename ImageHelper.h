#pragma once
#include <opencv2/opencv.hpp>
using namespace cv;
class ImageHelper{
public:
	explicit ImageHelper(){};
	static void swapchannel(std::string path){
		auto pic = cv::imread(path);
		for (int i = 0; i < pic.rows; ++i) for (int j = 0; j < pic.cols; ++j){
			cv::Vec3b col = pic.at<cv::Vec3b>(i, j);
			std::swap(col[0], col[2]);
			pic.at<cv::Vec3b>(i, j) = col;
		}
		cv::imwrite(path, pic);
	}
	static void swapchannel(Mat& pic){
		for (int i = 0; i < pic.rows; ++i) for (int j = 0; j < pic.cols; ++j){
			cv::Vec3b col = pic.at<cv::Vec3b>(i, j);
			std::swap(col[0], col[2]);
			pic.at<cv::Vec3b>(i, j) = col;
		}
	}
	
	static void swapbatch(const char* base,int cnt){
		for (int i = 0; i < cnt; ++i){
			swapchannel(std::string(base) + std::to_string(i) + ".bmp");
		}
	}

	static void makeAVideo(const char* input,const char* output,int frameCnt,int fps){
		cv::VideoWriter vedio;
		auto f = cv::imread(std::string(input) + "0.bmp"); 
		auto size = cv::Size(f.cols*0.5, f.rows*0.5);
		
		vedio.open(output, CV_FOURCC('D', 'I', 'V', 'X'), fps, size, true);
		for (int i = 0; i < frameCnt; ++i){
			auto img = cv::imread(std::string(input) + std::to_string(i) + ".bmp");
			//swapchannel(img);
			resize(img, img, size);
			vedio << img;
		}
	}
};