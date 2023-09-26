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

int bm_image_dump(bm_image& image, const char* filepath) {
  if (filepath == nullptr) {
    std::cout << "bm_image_dumpdata: OUT file name err!!" << std::endl;
    return BM_ERR_PARAM;
  }
  if (image.width == 0 || image.height == 0) {
    std::cout << "input image err!!" << std::endl;
  }

  int size = 0;
  bm_status_t ret = bm_image_get_byte_size(image, &size);
  if (ret != BM_SUCCESS) {
    std::cout << "get image size err!!" << std::endl;
    return ret;
  }

  unsigned char* data;
  data = new unsigned char[size];
  if (data == nullptr) {
    std::cout << "malloc memory failed!!" << std::endl;
    return BM_ERR_PARAM;
  }
  memset(data, 0, size);

  FILE* fp;
  fp = fopen(filepath, "w+");
  if (fp == nullptr) {
    std::cout << "open file failed!!" << std::endl;
    return BM_ERR_PARAM;
  }
  ret = bm_image_copy_device_to_host(image, (void**)&data);
  if (ret != BM_SUCCESS) {
    std::cout << "copy device data err!!" << std::endl;
    return ret;
  }

   if (image.data_type == DATA_TYPE_EXT_FLOAT32) {
    float* image_data = reinterpret_cast<float*>(data);
    for (int row = 0; row < image.height; row++) {
      for (int col = 0; col < image.width; col++) {
        int index = (row * image.width + col) * 3; // Each pixel has 3 bytes (RGB)
        float r = image_data[index];
        float g = image_data[index + 1];
        float b = image_data[index + 2];
        fprintf(fp, "[r:%f g:%f b:%f]", r, g, b); // Write RGB values in decimal format
      }
      fprintf(fp, "\n"); // New line after each row
    }
  } else if (image.data_type == DATA_TYPE_EXT_1N_BYTE || image.data_type == DATA_TYPE_EXT_4N_BYTE) {
    for (int row = 0; row < image.height; row++) {
      for (int col = 0; col < image.width; col++) {
        int index = (row * image.width + col) * 3; // Each pixel has 3 bytes (RGB)
        unsigned char r = data[index];
        unsigned char g = data[index + 1];
        unsigned char b = data[index + 2];
        fprintf(fp, "[r:%u g:%u b:%u]", r, g, b); // Write RGB values in decimal format
        // printf("[r:%f g:%f b:%f]", r, g, b);
      }
      fprintf(fp, "\n"); // New line after each row
    }
  } else {
    std::cout << "Unsupported image data type!" << std::endl;
    return BM_ERR_PARAM;
  }

  delete[] data;
  fclose(fp);

  return BM_SUCCESS;
}

int main(int argc, char* argv[]) {
  cout << "nihao!!" << endl;

  string img_file = "../../../../datasets/images/1920x1080_yuvj420.jpg";
  int dev_id = 0;
  if (argc > 1) img_file = argv[1];

  BMNNHandlePtr handle = make_shared<BMNNHandle>(dev_id);
  bm_handle_t h = handle->handle();
  bm_image bmimg;
  picDec(h, img_file.c_str(), bmimg);

  bmcv_rect_t crop_rect{1600,800, 50, 50};
  bmcv_padding_atrr_t padding_attr;
  
  padding_attr.dst_crop_sty = 0;
  padding_attr.dst_crop_stx = 0;
  
  padding_attr.dst_crop_h = 100;
  padding_attr.dst_crop_w = 100;

  padding_attr.padding_b = 114;
  padding_attr.padding_g = 114;
  padding_attr.padding_r = 114;
  padding_attr.if_memset = 1; //这行必须写，不然会报错
  
  bm_image img_crop;
  bm_image_create(h, padding_attr.dst_crop_h, padding_attr.dst_crop_w,FORMAT_BGR_PLANAR, bmimg.data_type, &img_crop);
  auto ret = bmcv_image_vpp_convert_padding(h, 1, bmimg, &img_crop,
                                              &padding_attr, &crop_rect);

  bm_image_write_to_bmp(img_crop,"./padandcrop.jpg");

  int size = 0;
  ret = bm_image_get_byte_size(img_crop, &size);
  unsigned char* data;
  data = new unsigned char[size];
  memset(data, 0, size);
  ret = bm_image_copy_device_to_host(img_crop, (void**)&data);

  // 输出图像数据
  for (int i = 0; i < 1; ++i) {
    for (int j = 0; j < 128; ++j) {
      std::cout << static_cast<uint8_t>(data[i * 128 + j]) - 3 << " ";
    }
    std::cout << std::endl;  // 在每行结束时换行
  }

  delete[] data;

   printf("[r:%f g:%f b:%f]", 10, 10, 10);

  auto ret2 = bm_image_dump(img_crop, "image.txt");
}
