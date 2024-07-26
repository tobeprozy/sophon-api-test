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


using json = nlohmann::json;
using namespace std;


int main(int argc, char *argv[]){

    cout<<"nihao!!"<<endl;

    string img_file="rtsp://172.26.13.138:8554/stream0";
    int dev_id=0;

    auto handle = sail::Handle(dev_id);
    sail::Bmcv bmcv(handle);

    sail::BMImage bmimg;
    sail::Decoder decoder((const string)img_file, true, dev_id);
    int ret=0;
    while(ret==0){
        ret = decoder.read(handle, bmimg);
        std::cout<<"pts:"<<decoder.get_stamp(0)<<"ms"<<std::endl;
        std::cout<<"dts:"<<decoder.get_stamp(1)<<"ms"<<std::endl;
        bmcv.imwrite("bmimg.jpg",bmimg.data());
    }
    

    cv::Mat cvmat2;
    cvmat2=cv::imread(img_file);


}