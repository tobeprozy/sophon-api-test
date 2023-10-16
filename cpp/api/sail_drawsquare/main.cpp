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


int main(int argc, char *argv[]){

    cout<<"nihao!!"<<endl;

    string img_file="../../../../datasets/images/zidane.jpg";
    int dev_id=0;

    auto handle = sail::Handle(dev_id);
    sail::Bmcv bmcv(handle);

    sail::BMImage bmimg;
    sail::Decoder decoder((const string)img_file, true, dev_id);
    int ret = decoder.read(handle, bmimg);
    // bmcv.imwrite("bmimg.jpg",bmimg.data());
    
    std::tuple<int, int, int> color = std::make_tuple(255, 0, 0); // 红色
    bmcv_point_t coord = {300, 300};
    ret=bm_image_is_attached(bmimg.data());
    bmcv_image_draw_point(handle.data(), bmimg.data(), 1, &coord, 10, 255, 0, 0);

    bmcv.drawsquare_(bmimg.data(),300,300,10,color);
    bmcv.imwrite("bmimg.jpg",bmimg.data());
}