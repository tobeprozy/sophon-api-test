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

    const char *inputFile = "/home/zhiyuanzhang/sophon/sophon_api_test/datasets/videos/elevator-1080p-25fps-4000kbps.h264";
    FILE *file = fopen(inputFile, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file for reading\n");
        return -1;
    }
 
    if(access("output",0)!=F_OK){
        mkdir("output",S_IRWXU);
    }
 
    fseek(file, 0, SEEK_END);
    int numBytes = ftell(file);
    cout << "infile size: " << numBytes << endl;
    fseek(file, 0, SEEK_SET);
 
    uint8_t *bs_buffer = (uint8_t *)av_malloc(numBytes);
    if (bs_buffer == nullptr) {
        cout << "av malloc for bs buffer failed" << endl;
        fclose(file);
        return -1;
    }
 
    fread(bs_buffer, sizeof(uint8_t), numBytes, file);
    fclose(file);
    file = nullptr;
 
    // create handle
    int dev_id=0;
    auto handle = sail::Handle(dev_id);
    bm_image  image;
    // sail::Decoder decoder((const string)inputFile, true, dev_id);
    // sail::BMImage  image;
 
    sail::Decoder_RawStream decoder_rawStream(dev_id,"h264");
     
    int frameCount =0;
    while(true){
         //第四个参数是考虑到，假如传进来一个包含很多帧的视频，那我输出又只有一个image，那么就让指针自动往后移动，调用一次，读一张图
         //如果传进来一帧,下一帧传进的是一个新的地址bs_buffer，那么必须调用一次，初始化一次avio_ctx，
         decoder_rawStream.read_(bs_buffer,numBytes,image,true);
         // Generate output filename, e.g. "output/out_0001.bmp", "output/out_0002.bmp" etc.
         string out = "output/out_" + to_string(frameCount) + ".bmp";
         bm_image_write_to_bmp(image, out.c_str());
         frameCount++; // Increment frame count
    }
    
    av_free(bs_buffer);
    return 0;


}