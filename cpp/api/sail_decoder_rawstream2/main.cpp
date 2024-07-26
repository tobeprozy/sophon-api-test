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

    // const char *inputFile ="/home/zhiyuanzhang/sophon/sophon_api_test/datasets/videos/elevator-1080p-25fps-4000kbps.h264";
    // const char *inputFile = "../20240701_192813_275020_5193-71161.h264";
    const char *inputFile = "../20240702_150921_149369_6934-67356";
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
         decoder_rawStream.read_(bs_buffer,numBytes,image,false);
         // Generate output filename, e.g. "output/out_0001.bmp",
         string out = "output/out_" +to_string(frameCount) + ".bmp"; 
         bm_image_write_to_bmp(image, out.c_str()); frameCount++; // Increment frame count
    }

    av_free(bs_buffer);
    return 0;

}

// extern "C" {
// #include <libavcodec/avcodec.h>
// #include <libavformat/avformat.h>
// #include <libavutil/imgutils.h>
// #include <libavutil/opt.h>
// #include <libswscale/swscale.h>
// }
// // #include <unistd.h>
// // #include <sys/stat.h>
// #include <fstream>
// #include <iostream>

// #include "ff_decode.hpp"
// using namespace std;

// typedef struct {
//         uint8_t* start;
//         int      size;
//         int      pos;
//     } bs_buffer_t;

// int read_buffer(void *opaque, uint8_t *buf, int buf_size){
//             bs_buffer_t* bs = (bs_buffer_t*)opaque;

//             int r = bs->size - bs->pos;
//             if (r <= 0) {
//                 cout << "EOF of AVIO." << endl;
//                 return AVERROR_EOF;
//             }

//             uint8_t* p = bs->start + bs->pos;
//             int len = (r >= buf_size) ? buf_size : r;
//             memcpy(buf, p, len);
//             //cout << "read " << len << endl;
//             bs->pos += len;
//             return len;
// }

// void decode_packet(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame, int *frame_count) {
//     int ret;

//     ret = avcodec_send_packet(dec_ctx, pkt);
//     if (ret < 0) {
//         std::cerr << "Error sending a packet for decoding\n";
//         return;
//     }

//     while (ret >= 0) {
//         ret = avcodec_receive_frame(dec_ctx, frame);
//         if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
//             return;
//         } else if (ret < 0) {
//             std::cerr << "Error during decoding\n";
//             return;
//         }
//         (*frame_count)++;
//     }
// }

// int main(int argc, char **argv) {
//     if (argc < 2) {
//         std::cerr << "Usage: " << argv[0] << " <input file>\n";
//         return -1;
//     }

//     const char *input_file = argv[1];
//     FILE *file = fopen(input_file, "rb");
//     if (!file) {
//         std::cerr << "Failed to open file for reading\n";
//         return -1;
//     }

//     fseek(file, 0, SEEK_END);
//     size_t numBytes = ftell(file);
//     fseek(file, 0, SEEK_SET);

//     uint8_t *bs_buffer = (uint8_t *)av_malloc(numBytes);
//     if (!bs_buffer) {
//         std::cerr << "Failed to allocate buffer\n";
//         fclose(file);
//         return -1;
//     }

//     fread(bs_buffer, 1, numBytes, file);
//     fclose(file);

//     bs_buffer_t bs_obj = { bs_buffer, numBytes, 0 };

//     const int aviobuf_size = 32*1024;
//     // uint8_t *aviobuffer = (uint8_t *)av_malloc(aviobuf_size);
//     // if (!aviobuffer) {
//     //     std::cerr << "Failed to allocate buffer\n";
//     //     av_free(bs_buffer);
//     //     return -1;
//     // }
//     uint8_t *aviobuffer = nullptr;
   
   
//     AVInputFormat *iformat = av_find_input_format("h264");
//     if (!iformat) {
//         std::cerr << "Could not find input format\n";
//         av_free(aviobuffer);
//         av_free(bs_buffer);
//         return -1;
//     }


//     AVCodec *codec = avcodec_find_decoder_by_name("h264_bm");
//        if (!codec) {
//         std::cerr << "Failed to find codec\n";
//         av_free(aviobuffer);
//         av_free(bs_buffer);
//         return -1;
//     }

//     AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
//     if (!codec_ctx) {
//         std::cerr << "Failed to allocate codec context\n";
//         av_free(aviobuffer);
//         av_free(bs_buffer);
//         return -1;
//     }

//     if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
//         std::cerr << "Failed to open codec\n";
//         avcodec_free_context(&codec_ctx);
//         av_free(aviobuffer);
//         av_free(bs_buffer);
//         return -1;
//     }

    
//     AVFormatContext *fmt_ctx = avformat_alloc_context();
//     if (!fmt_ctx) {
//         std::cerr << "Failed to allocate format context\n";
//         av_free(aviobuffer);
//         av_free(bs_buffer);
//         return -1;
//     }


//     AVPacket *pkt = av_packet_alloc();
//     if (!pkt) {
//         std::cerr << "Failed to allocate packet\n";
//         avcodec_free_context(&codec_ctx);
//         avformat_close_input(&fmt_ctx);
//         av_free(aviobuffer);
//         av_free(bs_buffer);
//         return -1;
//     }

//     AVFrame *frame = av_frame_alloc();
//     if (!frame) {
//         std::cerr << "Failed to allocate frame\n";
//         av_packet_free(&pkt);
//         avcodec_free_context(&codec_ctx);
//         avformat_close_input(&fmt_ctx);
//         av_free(aviobuffer);
//         av_free(bs_buffer);
//         return -1;
//     }

//              AVIOContext *avio_ctx = avio_alloc_context(aviobuffer, aviobuf_size, 0, &bs_obj, read_buffer, NULL, NULL);
//     if (!avio_ctx) {
//         std::cerr << "avio_alloc_context failed\n";
//         av_free(aviobuffer);
//         av_free(bs_buffer);
//         return -1;
//     }

//     fmt_ctx->pb = avio_ctx;
    
//     if (avformat_open_input(&fmt_ctx, NULL, iformat, NULL) < 0) {
//         std::cerr << "Could not open input\n";
//         av_free(aviobuffer);
//         av_free(bs_buffer);
//         avformat_free_context(fmt_ctx);
//         return -1;
//     }

//     int frame_count = 0;
//     int got_picture=0;
//     while (av_read_frame(fmt_ctx, pkt) >= 0) {



        
//     //     // 发送数据包到解码器
//     // send_packet:
//     //     int ret = avcodec_send_packet(codec_ctx, pkt);
//     //     if (ret < 0) {
//     //         if (ret == AVERROR(EAGAIN)) {
//     //             // 如果解码器需要更多数据，则继续读取并发送数据包
//     //             break;
//     //         } else {
//     //             std::cerr << "Error sending a packet for decoding\n";
//     //             break;
//     //         }
//     //     }

//     //     // 接收解码后的帧
//     // receive_frame:
//     //     ret = avcodec_receive_frame(codec_ctx, frame);
//     //     if (ret == AVERROR(EAGAIN)) {
//     //         av_packet_unref(pkt);
//     //         // 如果解码器需要更多数据，则跳回发送数据包
//     //         goto send_packet;
//     //     } else if (ret == AVERROR_EOF) {
//     //         break;
//     //     } else if (ret < 0) {
//     //         std::cerr << "Error during decoding\n";
//     //         break;
//     //     }
// receive_frame:
//         std::cout<<"receive_frame"<<std::endl;
//         int ret=avcodec_decode_video2(codec_ctx, frame, &got_picture, pkt);
//         if(ret>0){

//             av_packet_unref(pkt);
//             goto receive_frame;
//         }
        
//         std::cout << "width: " << frame->width << ", height: " << frame->height << "\n";
 
//         // decode_packet(codec_ctx, pkt, frame,&frame_count);
//         // av_packet_unref(pkt);
//         // decode_packet(codec_ctx, pkt, frame,&frame_count);
//     }

//     // decode_packet(codec_ctx, pkt, frame, &frame_count);

//     std::cout << "Decoding finished, total frames: " << frame_count << "\n";

//     avcodec_free_context(&codec_ctx);
//     avformat_close_input(&fmt_ctx);
//     av_packet_free(&pkt);
//     av_frame_free(&frame);
//     av_free(aviobuffer);
//     av_free(bs_buffer);

//     return 0;
// }
