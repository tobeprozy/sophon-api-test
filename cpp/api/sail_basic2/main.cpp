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

    string img_file="../../../../datasets/images/demo.png";
    int dev_id=0;
    // create handle
    auto handle = sail::Handle(dev_id);
    sail::Bmcv bmcv(handle);

    sail::BMImage img_input;
    sail::Decoder decoder((const string)img_file, true, dev_id);
    int ret = decoder.read(handle, img_input);

    // creat handle
    BMNNHandlePtr handle2 = make_shared<BMNNHandle>(dev_id);
    cout << "set device id: " << dev_id << endl;
    bm_handle_t h = handle2->handle();
    
    // 获取当前时间点
    auto start_time = std::chrono::high_resolution_clock::now();
    bmcv.imwrite("./img_input1.jpg",img_input);
    // 获取当前时间点
    auto end_time = std::chrono::high_resolution_clock::now();
    // 计算执行时间（毫秒）
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "bmcv.imwrite代码执行时间: " << duration << " 毫秒" << std::endl;
    
    
    bm_image frame;
    bm_image_create(h, img_input.height(), img_input.width(), FORMAT_YUV420P, DATA_TYPE_EXT_1N_BYTE, &frame);
    bmcv_image_storage_convert(h, 1, &img_input.data(), &frame);


    // save image
    void *jpeg_data = NULL;
    size_t out_size = 0;
    
    // 获取当前时间点
    start_time = std::chrono::high_resolution_clock::now();
    ret = bmcv_image_jpeg_enc(h, 1, &frame, &jpeg_data, &out_size);
    if (ret == BM_SUCCESS)
    {
            string img_file = "./img_input3.jpg";
            FILE *fp = fopen(img_file.c_str(), "wb");
            fwrite(jpeg_data, out_size, 1, fp);
            fclose(fp);
    }
    free(jpeg_data);
    // 获取当前时间点
    end_time = std::chrono::high_resolution_clock::now();
    // 计算执行时间（毫秒）
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    std::cout << "bmcv_image_jpeg_enc代码执行时间: " << duration << " 毫秒" << std::endl;


}