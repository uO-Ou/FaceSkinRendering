#pragma once
#include <opencv2/opencv.hpp>

class ImgHelper {
public:
	static void swap_channel(const std::string& pname) {
		auto img = cv::imread(pname);
		if (img.empty()) return;
		for (int i = 0; i < img.rows; ++i) for (int j = 0; j < img.cols; ++j) {
			auto color = img.at<cv::Vec3b>(i, j);
			img.at<cv::Vec3b>(i, j) = cv::Vec3b(color[2], color[1], color[0]);
		}
		cv::imwrite(pname, img);
	}
};