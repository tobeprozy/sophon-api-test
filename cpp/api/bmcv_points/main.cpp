//===----------------------------------------------------------------------===//
//
// Copyright (C) 2023 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-DEMO is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <iostream>

#include "bm_wrapper.hpp"
#include "bmnn_utils.h"
#include "ff_decode.hpp"
#include "json.hpp"
#include "opencv2/opencv.hpp"
#include "utils.hpp"

using json = nlohmann::json;
using namespace std;
#define USE_OPENCV_DECODE 0

using namespace std;

int main(int argc, char* argv[]) {
  cout << "nihao!!" << endl;

  string img_file = "../../../../datasets/images/zidane.jpg";
  int dev_id = 0;

  BMNNHandlePtr handle = make_shared<BMNNHandle>(dev_id);
  bm_handle_t h = handle->handle();
  bm_image bmimg;
  picDec(h, img_file.c_str(), bmimg);
  bm_image_write_to_bmp(bmimg, "bmimg.bmp");

  for (size_t i = 0; i < 100; i++) {
    int point_num = 1;
    bmcv_point_t coord = {300, 300};
    int length = 100;

    auto start = std::chrono::high_resolution_clock::now();
    bmcv_image_draw_point(h, bmimg, point_num, &coord, length, 255, 0, 0);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    double elapsed_ms = duration.count();
    std::cout << "bmcv_image_draw_point time: " << elapsed_ms << " milliseconds"
              << std::endl;
    bm_image_write_to_bmp(bmimg, "bmimg_point.bmp");

    bmcv_rect_t rects = {0, 0, 30, 30};
    start = std::chrono::high_resolution_clock::now();
    bmcv_image_fill_rectangle(h, bmimg, point_num, &rects, 255, 0, 0);
    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    elapsed_ms = duration.count();
    std::cout << "bmcv_image_fill_rectangle time: " << elapsed_ms << " milliseconds"
              << std::endl;
    bm_image_write_to_bmp(bmimg, "bmimg_point.bmp");
  }

  cv::Mat cvmat2;
  cvmat2 = cv::imread(img_file);

  bm_image bmimg2;
  cv::bmcv::toBMI(cvmat2, &bmimg2, true);
  bm_image_write_to_bmp(bmimg2, "bmimg2.bmp");
}
