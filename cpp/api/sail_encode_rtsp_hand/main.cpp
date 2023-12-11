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
#include <encoder.h>

using json = nlohmann::json;
using namespace std;

#define USE_FFMPEG  1
#define USE_BMCV  1

int encode_rtsp(bm_handle_t handle,int dev_id){
    bm_dev_request(&handle,dev_id);
    bm_get_chipid(handle,&dev_id)
}

int main(int argc, char *argv[]){

    const char *inputFile = "/home/zhiyuanzhang/sophon/sophon_api_test/datasets/videos/dance_1080P.mp4";
    
    int dev_id=0;

    auto handle = sail::Handle(dev_id);
    sail::Bmcv bmcv(handle);

    
    sail::Decoder decoder((const string)inputFile, true, dev_id);


    string output="rtsp://127.0.0.1:8554/mystream";
    string enc_fmt="h264_bm";
    string pix_fmt="NV12";
    string enc_params="width=1920:height=1080:gop=32:gop_preset=3:framerate=25:bitrate=2000";


    
    sail::Encoder  encoder(output, handle,enc_fmt,pix_fmt,enc_params);

    while(true){
        sail::BMImage bmimg;
        int ret=decoder.read(handle, bmimg);
        
        sleep(0.01);
        cout<<bmimg.width()<<" "<<bmimg.height()<<endl;
        cout<<bmimg.format()<<endl;
        // bmcv.imwrite("bmimg.jpg",bmimg.data());

        ret =encoder.is_opened();
        
        ret=encoder.video_write(bmimg);
        
    }
    encoder.release();
    return 0;
}