//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-DEMO is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//
/*
 * This is a wrapper header of BMruntime & BMCV, aiming to simplify user's program.
 */
#include <iostream>
#include <queue>
#include <mutex>
#include <pthread.h>
#include "bmruntime_interface.h"
#include "bmcv_api.h"
#include "bmcv_api_ext.h"
#include "bmlib_runtime.h"
#include "opencv2/opencv.hpp"
#include <opencv2/core.hpp>
#include "libyuv.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#define QUEUE_MAX_SIZE 5
#define EXTRA_FRAME_BUFFER_NUM 5
#define USEING_MEM_HEAP2 4
#define USEING_MEM_HEAP1 2

typedef struct {
    uint8_t* start;
    int      size;
    int      pos;
} bs_buffer_t;

int read_buffer(void *opaque, uint8_t *buf, int buf_size);

/**
 * @brief convert avframe pix format to bm_image pix format.
 * @return bm_image_format_ext.
 */
int map_avformat_to_bmformat(int avformat);

/**
 * @brief convert avformat to bm_image.
 */
bm_status_t avframe_to_bm_image(bm_handle_t &handle, AVFrame *in, bm_image *out, bool is_jpeg);

/**
 * @brief picture decode. support jpg and png
 */
bm_status_t picDec(bm_handle_t &handle, const char *path, bm_image &img);
bm_status_t jpgDec(bm_handle_t& handle, uint8_t* bs_buffer, int numBytes, bm_image& img);
bm_status_t miscDec(bm_handle_t& handle, uint8_t* bs_buffer, int numBytes, int type, bm_image& img);

/**
 * video decode class
 * support video file and rtsp stream.
 *
 * VideoDecFFM create a thread to decode, convert AVFrame to bm_image, push bm_image into the cache queue.
 * When the queue is full, for video file, the decode thread will sleep. For rtsp stream, the decode thread
 * will pop the front element of the queue.
 *
 */
class VideoDecFFM
{
public:
    VideoDecFFM();
    ~VideoDecFFM();

    /* open video decoder, decode, convert avFrame to bm_image, push it into the cache queue  */
    int openDec(bm_handle_t *dec_handle, const char *input);

    /* grab a bm_image from the cache queue*/
    bm_image *grab();

private:
    bool quit_flag = false;

    int is_rtsp;
    int width;
    int height;
    int pix_fmt;
    bool data_on_device_mem = true;

    int video_stream_idx;
    int refcount;

    AVFrame *frame;
    AVPacket pkt;
    AVFormatContext *ifmt_ctx;
    AVCodec *decoder;
    AVCodecContext *video_dec_ctx;
    AVCodecParameters *video_dec_par;

    bm_handle_t *handle;
    std::mutex lock;
    std::queue<bm_image *> queue;

    int openCodecContext(int *stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx,
                         enum AVMediaType type, int sophon_idx);

    void *vidPushImage();
    AVFrame *grabFrame();
    void closeDec();
};
