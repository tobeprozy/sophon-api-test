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

  string left_img = "../Left.png";
  string right_img = "../Right.png";

  int dev_id = 0;
  BMNNHandlePtr handle = make_shared<BMNNHandle>(dev_id);
  bm_handle_t h = handle->handle();
  bm_image src_img[2];
  // picDec(h, left_img.c_str(), src_img[0]);
  // picDec(h, right_img.c_str(), src_img[1]);

  cv::Mat left_img_mat = cv::imread(left_img);
  cv::Mat right_img_mat = cv::imread(right_img);

  cv::bmcv::toBMI(left_img_mat, &src_img[0],true);
  cv::bmcv::toBMI(right_img_mat, &src_img[1],true);


  int input_num = 2;
  bmcv_rect_t dst_crop[input_num];
  bmcv_rect_t src_crop[input_num];

  int src_crop_stx = 0;
  int src_crop_sty = 0;
  int src_crop_w = src_img[0].width;
  int src_crop_h = src_img[0].height;

  int dst_w = src_img[0].width+src_img[1].width;
  int dst_h = max(src_img[0].height,src_img[1].height);
  int dst_crop_w = dst_w;
  int dst_crop_h = dst_h;

  src_crop[0].start_x = 0 ;
  src_crop[0].start_y = 0;
  src_crop[0].crop_w = src_img[0].width;
  src_crop[0].crop_h = src_img[0].height;

  src_crop[1].start_x = 0;
  src_crop[1].start_y = 0;
  src_crop[1].crop_w = src_img[1].width;
  src_crop[1].crop_h = src_img[1].height;

  dst_crop[0].start_x = 0 ;
  dst_crop[0].start_y = 0;
  dst_crop[0].crop_w = src_img[0].width;
  dst_crop[0].crop_h = src_img[0].height;

  dst_crop[1].start_x = src_img[0].width;
  dst_crop[1].start_y = 0;
  dst_crop[1].crop_w = src_img[1].width;
  dst_crop[1].crop_h = src_img[1].height;
  



  bm_image dst_img;
  bm_image_format_ext src_fmt=src_img[0].image_format;
  bm_image_create(h,dst_h,dst_w,src_fmt,DATA_TYPE_EXT_1N_BYTE,&dst_img);

  auto start =std::chrono::system_clock::now();
  bmcv_image_vpp_stitch(h, input_num, src_img, dst_img, dst_crop, src_crop);
  auto end =std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  std::cout<<"elapsed time: "<<elapsed_seconds.count()<<""<<std::endl;

  bm_image_write_to_bmp(dst_img,"dst.png");

}
