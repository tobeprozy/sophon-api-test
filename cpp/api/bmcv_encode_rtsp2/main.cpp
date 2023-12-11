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
#define USE_OPENCV_DECODE 1

#define USE_FFMPEG 1


#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>



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

  BMNNHandlePtr handletop = make_shared<BMNNHandle>(dev_id);
  bm_handle_t handle = handletop->handle();

  int output_type=1;

  AVPixelFormat pix_fmt_;
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
  bool opened;
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

  std::vector<std::string> elems(std::sregex_token_iterator(enc_params.begin(), enc_params.end(), reg1, -1),
                                  std::sregex_token_iterator());
  for (auto param : elems)
  {
      std::vector<std::string> key_value_(std::sregex_token_iterator(param.begin(), param.end(), reg2, -1),
                                      std::sregex_token_iterator());

      std::string temp_key = key_value_[0];
      std::string temp_value = key_value_[1];

      params_map[temp_key] = std::stoi(temp_value);
  }


   if (pix_fmt == "I420")
    {
        pix_fmt = AV_PIX_FMT_YUV420P;
    }
    else if (pix_fmt == "NV12")
    {
        pix_fmt = AV_PIX_FMT_NV12;
    }
    else
    {
        throw std::runtime_error("Not support encode pix format.");
    }


    enc_ctx->codec_id      =   encoder->id;
    enc_ctx->pix_fmt       =   pix_fmt_;
    enc_ctx->width         =   params_map["width"];
    enc_ctx->height        =   params_map["height"];
    enc_ctx->gop_size      =   params_map["gop"];
    enc_ctx->time_base     =   (AVRational){1, params_map["framerate"]};
    enc_ctx->framerate     =   (AVRational){params_map["framerate"], 1};
    if(-1 == params_map["qp"])
    {
        enc_ctx->bit_rate_tolerance = params_map["bitrate"]*1000;
        enc_ctx->bit_rate      =   (int64_t)params_map["bitrate"]*1000;
    }else{
        av_dict_set_int(&enc_dict, "qp", params_map["qp"], 0);
    }

    av_dict_set_int(&enc_dict, "sophon_idx", dev_id, 0);
    av_dict_set_int(&enc_dict, "gop_preset", params_map["gop_preset"], 0);
    // av_dict_set_int(&enc_dict_, "mb_rc",      params_map_["mb_rc"],      0);    0);
    // av_dict_set_int(&enc_dict_, "bg",         params_map_["bg"],         0);
    // av_dict_set_int(&enc_dict_, "nr",         params_map_["nr"],         0);
    // av_dict_set_int(&enc_dict_, "weightp",    params_map_["weightp"],    0);
    av_dict_set_int(&enc_dict, "is_dma_buffer", 1, 0);

    // open encoder
    int ret = avcodec_open2(enc_ctx, encoder, &enc_dict);
    if(ret < 0){
        throw std::runtime_error("avcodec_open failed.");
    }
    av_dict_free(&enc_dict);

    // new stream
    out_stream = avformat_new_stream(enc_format_ctx, encoder);
    out_stream->time_base      = enc_ctx->time_base;
    out_stream->avg_frame_rate = enc_ctx->framerate;
    out_stream->r_frame_rate   = out_stream->avg_frame_rate;

    ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    if(ret < 0)
    {
        throw std::runtime_error("avcodec_parameters_from_context failed.");
    }

    if (!(enc_format_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&enc_format_ctx->pb, output_path.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
  
            throw std::runtime_error("avio_open2 failed.");
        }
    }

    ret = avformat_write_header(enc_format_ctx, NULL);
        if (ret < 0) {
            throw std::runtime_error("avformat_write_header failed.");
        }
    opened = true;


    bm_image image;
    AVFrame *frame = av_frame_alloc();
    ret = bm_image_to_avframe(handle, &image, frame);







}
