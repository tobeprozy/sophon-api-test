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

  string left_img = "../Left.png";
  string right_img = "../Right.png";

  int dev_id = 0;
  BMNNHandlePtr handle = make_shared<BMNNHandle>(dev_id);
  bm_handle_t h = handle->handle();
  bm_image src_img[2];
  // picDec(h, left_img.c_str(), src_img[0]);
  // picDec(h, right_img.c_str(), src_img[1]);


  //decode
  cv::Mat left_img_mat = cv::imread(left_img);
  cv::Mat right_img_mat = cv::imread(right_img);

  cv::bmcv::toBMI(left_img_mat, &src_img[0],true);
  cv::bmcv::toBMI(right_img_mat, &src_img[1],true);

  // vpss
  //dwa
  
  // stitch
  int input_num = 2;
  bmcv_rect_t dst_crop[input_num];
  bmcv_rect_t src_crop[input_num];

  int src_crop_stx = 0;
  int src_crop_sty = 0;
  int src_crop_w = src_img[0].width;
  int src_crop_h = src_img[0].height;

  int dst_w = src_img[0].width+src_img[1].width;
  int dst_h = max(src_img[0].height,src_img[1].height);
  int dst_crop_w = dst_w;
  int dst_crop_h = dst_h;

  src_crop[0].start_x = 0 ;
  src_crop[0].start_y = 0;
  src_crop[0].crop_w = src_img[0].width;
  src_crop[0].crop_h = src_img[0].height;

  src_crop[1].start_x = 0;
  src_crop[1].start_y = 0;
  src_crop[1].crop_w = src_img[1].width;
  src_crop[1].crop_h = src_img[1].height;

  dst_crop[0].start_x = 0 ;
  dst_crop[0].start_y = 0;
  dst_crop[0].crop_w = src_img[0].width;
  dst_crop[0].crop_h = src_img[0].height;

  dst_crop[1].start_x = src_img[0].width;
  dst_crop[1].start_y = 0;
  dst_crop[1].crop_w = src_img[1].width;
  dst_crop[1].crop_h = src_img[1].height;
  


  bm_image dst_img;
  bm_image_format_ext src_fmt=src_img[0].image_format;
  bm_image_create(h,dst_h,dst_w,src_fmt,DATA_TYPE_EXT_1N_BYTE,&dst_img);

  auto start =std::chrono::system_clock::now();
  bmcv_image_vpp_stitch(h, input_num, src_img, dst_img, dst_crop, src_crop);
  auto end =std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  std::cout<<"elapsed time: "<<elapsed_seconds.count()<<""<<std::endl;

  bm_image_write_to_bmp(dst_img,"dst.png");

  AVFrame frame;
  bm_image_to_avframe(dst_img,&frame);
  
   // 使用FFmpeg发送图像
  av_register_all();

  AVFormatContext* formatContext = nullptr;
  AVOutputFormat* outputFormat = nullptr;
  AVStream* stream = nullptr;
  AVCodecContext* codecContext = nullptr;
  AVCodec* codec = nullptr;
  AVFrame* frame = nullptr;
  AVPacket packet;

  string outputUrl = "rtsp://172.25.92.230/your_stream_name";

  avformat_alloc_output_context2(&formatContext, nullptr, "rtsp", outputUrl.c_str());
  if (!formatContext) {
    cerr << "Failed to allocate output context" << endl;
    return -1;
  }

  outputFormat = formatContext->oformat;

  stream = avformat_new_stream(formatContext, nullptr);
  if (!stream) {
    cerr << "Failed to create new stream" << endl;
    return -1;
  }

  codecContext = stream->codec;
  codecContext->codec_id = outputFormat->video_codec;
  codecContext->codec_type = AVMEDIA_TYPE_VIDEO;
  codecContext->pix_fmt = AV_PIX_FMT_BGR24;
  codecContext->width = dst_w;
  codecContext->height = dst_h;
  codecContext->time_base = {1, 25}; // 设置帧率为25fps

  codec = avcodec_find_encoder(codecContext->codec_id);
  if (!codec) {
    cerr << "Codec not found" << endl;
    return -1;
  }

  if (avcodec_open2(codecContext, codec, nullptr) < 0) {
    cerr << "Failed to open codec" << endl;
    return -1;
  }

  frame = av_frame_alloc();
  if (!frame) {
    cerr << "Failed to allocate frame" << endl;
    return -1;
  }

  frame->format = codecContext->pix_fmt;
  frame->width = codecContext->width;
  frame->height = codecContext->height;

  if (av_frame_get_buffer(frame, 0) < 0) {
    cerr << "Failed to allocate frame buffer" << endl;
    return -1;
  }

  if (avformat_write_header(formatContext, nullptr) < 0) {
    cerr << "Failed to write header" << endl;
    return -1;
  }

  // 将dst图像数据复制到AVFrame中
  av_image_fill_arrays(frame->data, frame->linesize, dst_img.data, AV_PIX_FMT_BGR24, dst_w, dst_h, 1);

  // 发送图像数据
  av_init_packet(&packet);
  packet.data = nullptr;
  packet.size = 0;

  if (avcodec_send_frame(codecContext, frame) < 0) {
    cerr << "Failed to send frame" << endl;
    return -1;
  }

  while (avcodec_receive_packet(codecContext, &packet) >= 0) {
    av_packet_rescale_ts(&packet, codecContext->time_base, stream->time_base);
    packet.stream_index = stream->index;

    av_interleaved_write_frame(formatContext, &packet);
    av_packet_unref(&packet);
  }

  av_write_trailer(formatContext);

  // 释放资源
  avcodec_free_context(&codecContext);
  av_frame_free(&frame);
  avformat_free_context(formatContext);

  return 0;


}
