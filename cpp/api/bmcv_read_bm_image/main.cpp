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

  string img_file = "../../../../datasets/images/1920x1080_yuvj420.jpg";
  int dev_id = 0;

  BMNNHandlePtr handle = make_shared<BMNNHandle>(dev_id);
  bm_handle_t h = handle->handle();
  bm_image bmimg;
  picDec(h, img_file.c_str(), bmimg);

  int size = 0;
  auto ret = bm_image_get_byte_size(bmimg, &size);
  unsigned char* data;
  data = new unsigned char[size];
  memset(data, 0, size);
  ret = bm_image_copy_device_to_host(bmimg, (void**)&data);

  // 输出图像数据
  for (int i = 0; i < 1; ++i) {
    for (int j = 0; j < 128; ++j) {
      std::cout << static_cast<float>(data[i * 128 + j]) - 3 << " ";
    }
    std::cout << std::endl;  // 在每行结束时换行
  }
}
