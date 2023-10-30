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
#include <regex>
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

int encode_rtsp(bm_handle_t handle,int dev_id,string output_path,int output_type){
   
}



int main(int argc, char* argv[]) {
  cout << "nihao!!" << endl;

  string img_file = "../../../../datasets/images/demo.png";
  int dev_id = 0;

  string output_path="rtsp://127.0.0.1:8554/mystream";
  string enc_fmt="h264_bm";
  string pix_fmt="NV12";
  string enc_params="width=1920:height=1080:gop=32:gop_preset=3:framerate=25:bitrate=2000";

  bm_handle_t handle;
  int output_type=1;

  AVPixelFormat pix_fmt;
    // AVFrame *frame_;
  AVCodec *encoder;
  AVDictionary *enc_dict;
  AVIOContext *avio_ctx;
  AVFormatContext *enc_format_ctx;
  AVOutputFormat *enc_output_fmt;
  AVCodecContext *enc_ctx;
  AVStream *out_stream;
  AVPacket *pkt;
  bool is_rtsp;

  std::map<std::string, int> params_map;

  bm_dev_request(&handle,dev_id);

  unsigned int chip_id=0x1684;
  bm_get_chipid(handle, &(chip_id));

    //先写一个，rtsp
  switch(output_type){
      case 1:   
            avformat_alloc_output_context2(&enc_format_ctx, NULL, "rtsp", output_path.c_str());
            is_rtsp = true;
            break;
  }
  if(!enc_format_ctx){
    throw std::runtime_error("Could not create output context");
  }

  encoder = avcodec_find_encoder_by_name(enc_fmt.c_str());
  if(!encoder){
    throw std::runtime_error("Could not find encoder");
  }

  enc_ctx = avcodec_alloc_context3(encoder);
  if(!enc_ctx){
    throw std::runtime_error("Could not allocate video codec context");
  }

  if(enc_format_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


  //解析参数
  params_map.insert(std::pair<std::string, int>("width", 1920));
  params_map.insert(std::pair<std::string, int>("height", 1080));
  params_map.insert(std::pair<std::string, int>("framerate", 25));
  params_map.insert(std::pair<std::string, int>("bitrate", 2000));
  params_map.insert(std::pair<std::string, int>("gop", 32));
  params_map.insert(std::pair<std::string, int>("gop_preset", 3));
  params_map.insert(std::pair<std::string, int>("mb_rc", 0));
  params_map.insert(std::pair<std::string, int>("qp", -1));
  params_map.insert(std::pair<std::string, int>("bg", 0));
  params_map.insert(std::pair<std::string, int>("nr", 0));
  params_map.insert(std::pair<std::string, int>("weightp", 0));


  std::string s1;
  s1.append(1, ':');
  std::regex reg1(s1);

  std::string s2;
  s2.append(1, '=');
  std::regex reg2(s2);

  std::vector<std::string> elems(std::sregex_token_iterator(enc_params.begin(), enc_params_.end(), reg1, -1),
                                  std::sregex_token_iterator());
  for (auto param : elems)
  {
      std::vector<std::string> key_value_(std::sregex_token_iterator(param.begin(), param.end(), reg2, -1),
                                      std::sregex_token_iterator());

      std::string temp_key = key_value_[0];
      std::string temp_value = key_value_[1];

      params_map[temp_key] = std::stoi(temp_value);
  }


}
