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
#include <iostream>
#include "opencv2/opencv.hpp"
#include "bmnn_utils.h"
#include "utils.hpp"
#include "bm_wrapper.hpp"


using json = nlohmann::json;
using namespace std;
#define USE_OPENCV_DECODE 0

using namespace std;

int main(int argc, char *argv[])
{

  cout<<"nihao!!"<<endl;

  string img_file="../../../../datasets/images/demo.png";
  int dev_id=0;

  BMNNHandlePtr handle = make_shared<BMNNHandle>(dev_id);
  bm_handle_t h = handle->handle();
  bm_image bmimg;
  picDec(h, img_file.c_str(), bmimg);

  int i=0;
  cv::Mat cvmat;
  cv::bmcv::toMAT(&bmimg, cvmat);
  std::string fname = cv::format("cbmat_%d.jpg", i);
  cv::imwrite(fname, cvmat);

  cv::Mat cvmat2;
  cvmat2=cv::imread(img_file);

  bm_image bmimg2;
  cv::bmcv::toBMI(cvmat2, &bmimg2,true);
  bm_image_write_to_bmp(bmimg2,"bmimg2.bmp");

  bm_image frame;
  bm_image_create(h, bmimg2.height, bmimg2.width, FORMAT_YUV420P, bmimg2.data_type, &frame);
  bmcv_image_storage_convert(h, 1, &bmimg2, &frame);


  string img_path
  void *jpeg_data = NULL;
  size_t out_size = 0;
  int ret = bmcv_image_jpeg_enc(h, 1, &frame, &jpeg_data, &out_size);
  if (ret == BM_SUCCESS)
  {
    FILE *fp = fopen(img_path.c_str(), "wb");
    fwrite(jpeg_data, out_size, 1, fp);
    fclose(fp);
  }
  free(jpeg_data);
  bm_image_destroy(frame);





  // VideoDecFFM decoder;
  // decoder.openDec(&h, input.c_str());
  // bm_image* img = decoder.grab();


  
}
