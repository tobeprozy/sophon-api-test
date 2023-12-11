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

#include "bmnn_utils.h"
#include "ff_decode.hpp"
#include "json.hpp"
#include "opencv2/opencv.hpp"
#include "utils.hpp"
#include <unistd.h>

using json = nlohmann::json;
using namespace std;
#define USE_OPENCV_DECODE 1

#define USE_FFMPEG 1

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <time.h>
}

#include "bm_wrapper.hpp"

static inline int map_bmformat_to_avformat(int bmformat) {
  int format;
  switch (bmformat) {
    case FORMAT_YUV420P:
      format = AV_PIX_FMT_YUV420P;
      break;
    case FORMAT_YUV422P:
      format = AV_PIX_FMT_YUV422P;
      break;
    case FORMAT_YUV444P:
      format = AV_PIX_FMT_YUV444P;
      break;
    case FORMAT_NV12:
      format = AV_PIX_FMT_NV12;
      break;
    case FORMAT_NV16:
      format = AV_PIX_FMT_NV16;
      break;
    case FORMAT_GRAY:
      format = AV_PIX_FMT_GRAY8;
      break;
    case FORMAT_RGBP_SEPARATE:
      format = AV_PIX_FMT_GBRP;
      break;
    default:
      printf("unsupported image format %d\n", bmformat);
      return -1;
  }
  return format;
}

typedef struct {
  bm_image* bmImg;
  uint8_t* buf0;
  uint8_t* buf1;
  uint8_t* buf2;
} transcode_t;

static inline void bmBufferDeviceMemFree(void* opaque, uint8_t* data) {
  if (opaque == NULL) {
    printf("parameter error\n");
  }
  transcode_t* testTranscoed = (transcode_t*)opaque;
  av_freep(&testTranscoed->buf0);
  testTranscoed->buf0 = NULL;

  int ret = 0;
  ret = bm_image_destroy(*(testTranscoed->bmImg));
  if (testTranscoed->bmImg) {
    free(testTranscoed->bmImg);
    testTranscoed->bmImg = NULL;
  }
  if (ret != 0) printf("bm_image destroy failed\n");
  free(testTranscoed);
  testTranscoed = NULL;
  return;
}

static inline void bmBufferDeviceMemFree2(void* opaque, uint8_t* data) {
  return;
}

static inline bm_status_t bm_image_to_avframe(bm_handle_t &bm_handle,bm_image *in,AVFrame *out){
    transcode_t *ImgOut  = NULL;
    ImgOut = (transcode_t *)malloc(sizeof(transcode_t));
    ImgOut->bmImg = in;
    bm_image_format_info_t image_info;
    int idx       = 0;
    int plane     = 0;
    if(in == NULL || out == NULL){
        free(ImgOut);
        return BM_ERR_FAILURE;
    }

    if(ImgOut->bmImg->image_format == FORMAT_NV12){
        plane = 2;
    }
    else if(ImgOut->bmImg->image_format == FORMAT_YUV420P){
        plane = 3;
    }
    else{
        free(ImgOut);
        free(in);
        return BM_ERR_FAILURE;
    }

    out->format = (AVPixelFormat)map_bmformat_to_avformat(ImgOut->bmImg->image_format);
    out->height = ImgOut->bmImg->height;
    out->width = ImgOut->bmImg->width;

    if(ImgOut->bmImg->width > 0 && ImgOut->bmImg->height > 0
        && ImgOut->bmImg->height * ImgOut->bmImg->width <= 8192*4096) {
        ImgOut->buf0 = (uint8_t*)av_malloc(ImgOut->bmImg->height * ImgOut->bmImg->width * 3 / 2);
        ImgOut->buf1 = ImgOut->buf0 + (unsigned int)(ImgOut->bmImg->height * ImgOut->bmImg->width);
        if(plane == 3){
            ImgOut->buf2 = ImgOut->buf0 + (unsigned int)(ImgOut->bmImg->height * ImgOut->bmImg->width * 5 / 4);
        }
    }

    out->buf[0] = av_buffer_create(ImgOut->buf0,ImgOut->bmImg->width * ImgOut->bmImg->height,
        bmBufferDeviceMemFree,ImgOut,AV_BUFFER_FLAG_READONLY);
    out->buf[1] = av_buffer_create(ImgOut->buf1,ImgOut->bmImg->width * ImgOut->bmImg->height / 2 /2 ,
        bmBufferDeviceMemFree2,NULL,AV_BUFFER_FLAG_READONLY);
    out->data[0] = ImgOut->buf0;
    out->data[1] = ImgOut->buf0;

    if(plane == 3){
        out->buf[2] = av_buffer_create(ImgOut->buf2,ImgOut->bmImg->width * ImgOut->bmImg->height / 2 /2 ,
            bmBufferDeviceMemFree2,NULL,AV_BUFFER_FLAG_READONLY);
        out->data[2] = ImgOut->buf0;
    }

    if(plane == 3 && !out->buf[2]){
        av_buffer_unref(&out->buf[0]);
        av_buffer_unref(&out->buf[1]);
        av_buffer_unref(&out->buf[2]);
        free(ImgOut);
        free(in);
        return BM_ERR_FAILURE;
    }
    else if(plane == 2 && !out->buf[1]){
        av_buffer_unref(&out->buf[0]);
        av_buffer_unref(&out->buf[1]);
        free(ImgOut);
        free(in);
        return BM_ERR_FAILURE;
    }

    bm_device_mem_t mem_tmp[3];
    if(bm_image_get_device_mem(*(ImgOut->bmImg),mem_tmp) != BM_SUCCESS ){
        free(ImgOut);
        free(in);
        return BM_ERR_FAILURE;
    }
    if(bm_image_get_format_info(ImgOut->bmImg, &image_info) != BM_SUCCESS ){
        free(ImgOut);
        free(in);
        return BM_ERR_FAILURE;
    }
    for (idx=0; idx< plane; idx++) {
        out->data[4+idx]     = (uint8_t *)mem_tmp[idx].u.device.device_addr;
        out->linesize[idx]   = image_info.stride[idx];
        out->linesize[4+idx] = image_info.stride[idx];
    }
    return BM_SUCCESS;
}

// 获取系统的当前时间，单位微秒(us)
double get_current_time_us() {
#ifdef _WIN32
// 从1601年1月1日0:0:0:000到1970年1月1日0:0:0:000的时间(单位100ns)
#define EPOCHFILETIME (116444736000000000UL)
  FILETIME ft;
  LARGE_INTEGER li;
  double tt = 0;
  GetSystemTimeAsFileTime(&ft);
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;
  // 从1970年1月1日0:0:0:000到现在的微秒数(UTC时间)
  tt = (li.QuadPart - EPOCHFILETIME) / 10;
  return tt;
#else
  timeval tv;
  gettimeofday(&tv, 0);
  return (double)tv.tv_sec * 1000000 + (double)tv.tv_usec;
#endif  // _WIN32
  return 0;
}

using namespace std;

int main(int argc, char* argv[]) {
  cout << "nihao!!" << endl;

  int dev_id = 0;
  BMNNHandlePtr handletop = make_shared<BMNNHandle>(dev_id);
  bm_handle_t handle = handletop->handle();

  std::string filename =
      "../../../../datasets/videos/elevator-1080p-25fps-4000kbps.h264";
  string output_path = "rtsp://127.0.0.1:8554/mystream";
  string enc_fmt = "h264_bm";
  string pix_fmt = "NV12";


  AVDictionary* dict = NULL;
  AVFormatContext* enc_format_ctx;
  AVCodec* encoder;
  AVCodecContext* enc_ctx;
  AVPixelFormat pix_fmt_;
  AVDictionary* enc_dict = nullptr;

  AVStream* out_stream;

  avformat_alloc_output_context2(&enc_format_ctx, NULL, "rtsp",
                                 output_path.c_str());
  encoder = avcodec_find_encoder_by_name(enc_fmt.c_str());

  enc_ctx = avcodec_alloc_context3(encoder);
  if (enc_format_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (pix_fmt == "I420") {
    pix_fmt_ = AV_PIX_FMT_YUV420P;
  } else if (pix_fmt == "NV12") {
    pix_fmt_ = AV_PIX_FMT_NV12;
  } else {
    ;
  }

  enc_ctx->codec_id = encoder->id;
  enc_ctx->pix_fmt = pix_fmt_;
  enc_ctx->width = 1920;
  enc_ctx->height = 1080;
  enc_ctx->gop_size = 32;
  enc_ctx->time_base = (AVRational){1, 25};
  enc_ctx->framerate = (AVRational){25, 1};

  enc_ctx->bit_rate_tolerance = 2000 * 1000;
  enc_ctx->bit_rate = (int64_t)2000 * 1000;

  av_dict_set_int(&enc_dict, "sophon_idx", dev_id, 0);
  av_dict_set_int(&enc_dict, "gop_preset", 3, 0);
  av_dict_set_int(&enc_dict, "is_dma_buffer", 1, 0);

  int ret = avcodec_open2(enc_ctx, encoder, &enc_dict);
  av_dict_free(&enc_dict);

  // new stream
  out_stream = avformat_new_stream(enc_format_ctx, encoder);
  out_stream->time_base = enc_ctx->time_base;
  out_stream->avg_frame_rate = enc_ctx->framerate;
  out_stream->r_frame_rate = out_stream->avg_frame_rate;

  ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);

  if (!(enc_format_ctx->oformat->flags & AVFMT_NOFILE)) {
    ret = avio_open(&enc_format_ctx->pb, output_path.c_str(), AVIO_FLAG_WRITE);
  }
  ret = avformat_write_header(enc_format_ctx, NULL);

  VideoDecFFM decoder;
  decoder.openDec(&handle, filename.c_str());

  int got_output = 0;
  int64_t frame_interval = 1 * 1000 * 1000 / 25;
  double start_time, current_time;
  AVFrame* frame = av_frame_alloc();

  while (true) {
    AVPacket* enc_pkt = av_packet_alloc();

    bm_image* img = decoder.grab();
    bm_image_write_to_bmp(*img, "bmimg.bmp");
    std::cout << "write bmimg bmp" << std::endl;
    bm_image img2;
    bm_image_create(handle, img->height, img->width, FORMAT_NV12,
                  img->data_type, &img2);
    bmcv_image_storage_convert(handle, 1, img, &img2);
    
    ret = bm_image_to_avframe(handle, &img2, frame);

    ret = avcodec_encode_video2(enc_ctx, enc_pkt, frame, &got_output);
   

    av_packet_rescale_ts(enc_pkt, enc_ctx->time_base, out_stream->time_base);
    ret = av_interleaved_write_frame(enc_format_ctx, enc_pkt);
    av_packet_free(&enc_pkt);

  }
  av_frame_free(&frame);
}
