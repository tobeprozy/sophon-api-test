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


int drawsquare_(
            bm_handle_t handle,
            const bm_image &input,
            int x0,
            int y0,
            int w,
            const std::tuple<int, int, int> &color
    ) {
        int left = x0 > 0 ? x0 : 0;
        int top = y0 > 0 ? y0 : 0;
       
        left = left < input.width-1 ? left : input.width-1;
        top = top < input.height-1 ? top : input.height-1;
       
        bmcv_point_t coord = {x0, y0};
        int ret = bmcv_image_draw_point(
                handle,
                input,
                1,
                &coord,
                w,
                std::get<2>(color),  // R
                std::get<1>(color),  // G
                std::get<0>(color)); // B
        if (BM_SUCCESS != ret) {
            return ret;
        }
        return BM_SUCCESS;
};


int main(int argc, char* argv[]) {
  cout << "nihao!!" << endl;

  string img_file = "../../../../datasets/images/1920x1080_yuvj420.jpg";
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

    int ret=bm_image_is_attached(bmimg);

    std::tuple<int, int, int> color = std::make_tuple(255, 0, 0); // 红色
    start = std::chrono::high_resolution_clock::now();
    drawsquare_(h, bmimg, 300,300,20,color);
    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    elapsed_ms = duration.count();
    std::cout << "bmcv_image_draw_point api time: " << elapsed_ms << " milliseconds"
              << std::endl;
    bm_image_write_to_bmp(bmimg, "bmimg_point_api.bmp");


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
