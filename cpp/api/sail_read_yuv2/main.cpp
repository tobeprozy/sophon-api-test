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

    char * img_path="../../../../datasets/images/1_nv12.yuv";
    uint32_t img_len=0;
    char *buf =read_file_content(img_path,&img_len);

    void* buff[2];
    buff[0]=reinterpret_cast<void*>(buf);
    buff[1]=reinterpret_cast<void*>(buf+3686400);

    sail::BMImage img_input1(handle,1440,2560,FORMAT_NV12,DATA_TYPE_EXT_1N_BYTE);
    auto ret = bm_image_copy_host_to_device(img_input1.data(),buff);


    cv::Mat cvimg;
    cv::bmcv::toMAT(&img_input1.data(),cvimg);
    cv::imwrite("img_input1.jpg",cvimg);

    sail::BMImage img_input2(handle,1440,2560,FORMAT_NV12,DATA_TYPE_EXT_1N_BYTE);
    ret = bm_image_copy_host_to_device(img_input2.data(),reinterpret_cast<void**>(&buf));
    cv::bmcv::toMAT(&img_input2.data(),cvimg);
    cv::imwrite("img_input2.jpg",cvimg);



}