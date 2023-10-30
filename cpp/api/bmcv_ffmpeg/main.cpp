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
  av_register_all();

  AVCodec* pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
  AVCodecContext* pContext = avcodec_alloc_context3(pCodec);
  AVDictionary* opts = NULL;
  int refcount = 1;
  av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
  int sophon_idx = 0;
  av_dict_set_int(&opts, "sophon_idx", sophon_idx, 0);
  av_dict_set_int(&opts, "extra_frame_buffer_num", 5,
                  0);  // if we use dma_buffer mode

  if (avcodec_open2(pContext, pCodec, &opts) < 0) {
    cerr << "Failed to open codec" << endl;
    return -1;
  }

  std::string input =
      "../../../../datasets/videos/elevator-1080p-25fps-4000kbps.h264";
  std::ifstream file(input, std::ios::binary);
  if (!file) {
    std::cout << "Failed to open input file" << std::endl;
    return -1;
  }
  std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});


  // 分配解码后的帧数据存储空间
  AVFrame* pFrame = av_frame_alloc();

  int index = 0;
  while (true) {
    AVPacket packet;
    av_init_packet(&packet);

    // 设置Packet的数据和大小
    packet.data = buffer.data() + index;
    packet.size = buffer.size() - index;
    int frameFinished = 0;
    // 解码视频帧
    int ret = avcodec_decode_video2(pContext, pFrame, &frameFinished, &packet);
    if (ret < 0) {
      std::cout << "Error decoding frame: " << av_err2str(ret) << std::endl;
      return -1;
    }
    index ++;
    // 释放Packet内部资源
    av_packet_unref(&packet);

  }
}
