//===----------------------------------------------------------------------===//
//
// Copyright (C) 2023 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-DEMO is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//
#include <iostream>
#include <fstream>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "json.hpp"
#include "opencv2/opencv.hpp"
#include "ff_decode.hpp"
#include <unordered_map>
#include <iostream>

#include "bmnn_utils.h"
#include "utils.hpp"
#include "bm_wrapper.hpp"
#include <cvwrapper.h>

using json = nlohmann::json;
using namespace std;


static char* read_file_content(const char* filepath, uint32_t* size) {
  char* content = NULL;
  unsigned int file_content_size = 0;
  FILE* fp = NULL;
  if (filepath == NULL) {
    return NULL;
  }

  fp = fopen(filepath, "rb");
  if (fp == NULL) {
    return NULL;
  }

  fseek(fp, 0L, SEEK_END);
  file_content_size = ftell(fp);
  if (file_content_size <= 0) {
    fclose(fp);
    return NULL;
  }

  content = (char*)malloc(file_content_size + 1);
  if (content == NULL) {
    fclose(fp);
    return NULL;
  }
  memset(content, 0, file_content_size + 1);
  fseek(fp, 0L, SEEK_SET);
  fread(content, file_content_size, 1, fp);

  fclose(fp);
  if (size != NULL) {
    *size = file_content_size;
  }
  return content;
}


int main(int argc, char *argv[]){

    int dev_id=0;
    auto handle =sail::Handle(dev_id);
    sail::Bmcv bmcv(handle);

    char * img_path="../../../../datasets/images/src_mat.bgr";
    uint32_t img_len=0;
    char *buf =read_file_content(img_path,&img_len);

    sail::BMImage img_input(handle,1440,2560,FORMAT_BGR_PACKED,DATA_TYPE_EXT_1N_BYTE);
    auto ret = bm_image_copy_host_to_device(img_input.data(),reinterpret_cast<void**>(&buf));

    sail::BMImage img_crop=bmcv.crop_and_resize(img_input,1997,30,200,200,300,300,BMCV_INTER_NEAREST);

    cv::Mat cvimg;
    cv::bmcv::toMAT(&img_input.data(),cvimg);
    cv::imwrite("cvimg.jpg",cvimg);

    cv::Mat bgr_pic =cv::Mat(cvimg.rows,cvimg.cols,CV_8UC3,cvimg.data);
    printf("bgr_pic %d %zu %zu\n",bgr_pic.type(),bgr_pic.elemSize(),bgr_pic.total());

    auto bgr_pic2=cvimg;
    printf("bgr_pic2 %d %zu %zu\n",bgr_pic2.type(),bgr_pic2.elemSize(),bgr_pic2.total());

    cv::imwrite("my_test_src_dst.jpg",bgr_pic);
    cv::imwrite("my_test_src_dst2.jpg",bgr_pic2);



    cv::Rect rect =cv::Rect(1997,111,48,19);
    auto src_rect =cvimg(rect);
    cv::imwrite("src_rect.jpg",src_rect);

    bm_image_write_to_bmp(img_crop.data(),"img_crop.bmp");

}