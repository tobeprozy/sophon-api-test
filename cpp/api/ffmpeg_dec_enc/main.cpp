/**
*************** FFMPEG视频编码流程 *******************
​
* 01、av_register_all()：注册FFmpeg所有编解码器;
* 02、avformat_alloc_output_context2()：初始化输出码流的AVFormatContext;
* 03、avio_open()：打开输出文件;
* 04、av_new_stream()：创建输出码流的AVStream;
* 05、avcodec_find_encoder()：查找编码器;
* 06、avcodec_open2()：打开编码器;
* 07、avformat_write_header()：写文件头(对于某些没有文件头的封装格式，不需要此函数。比如说MPEG2TS);
* 08、不停地从码流中提取出YUV数据，进行编码;
* avcodec_encode_video2()：编码一帧视频。即将AVFrame(存储YUV像素数据)编码为AVPacket(存储H.264等格式的码流数据);
* av_write_frame()：将编码后的视频码流写入文件;
* 09、flush_encoder()：输入的像素数据读取完成后调用此函数。用于输出编码器中剩余的AVPacket;
* 10、av_write_trailer()：写文件尾(对于某些没有文件头的封装格式，不需要此函数。比如说MPEG2TS);
*/
​
#include <stdio.h>
#define __STDC_CONSTANT_MACROS
#ifdef _WIN32
    //Windows
    extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
    };
#else
// Linux...
#ifdef __cplusplus
    extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#ifdef __cplusplus
    };
#endif
#endif
// 输入的像素数据读取完成后调用此函数，用于输出编码器中剩余的AVPacket
int flush_encoder(AVFormatContext* fmt_ctx, unsigned int stream_index) {
  int ret;
  int got_frame;
  AVPacket enc_pkt;
  if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
        CODEC_CAP_DELAY))
    return 0;
  while (1) {
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);
    // 编码一帧视频。即将AVFrame（存储YUV像素数据）编码为AVPacket（存储H.264等格式的码流数据）。
    ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
                                NULL, &got_frame);
    av_frame_free(NULL);
    if (ret < 0) break;
    if (!got_frame) {
      ret = 0;
      break;
    }
    printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n",
           enc_pkt.size);
    /* mux encoded frame */
    ret = av_write_frame(fmt_ctx, &enc_pkt);
    if (ret < 0) break;
  }
  return ret;
}

int main(int argc, char* argv[]) {
  AVFormatContext*pFormatCtx;  // 封装格式上下文结构体，也是统领全局的结构体，保存了视频文件封装
                   // 格式相关信息。
  AVOutputFormat*fmt;  // AVOutputFormat 结构体主要用于muxer，是音视频文件的一个封装器。
  AVStream* video_st;  // AVStream是存储每一个视频/音频流信息的结构体。
  AVCodecContext*pCodecCtx;  // 编码器上下文结构体，保存了视频（音频）编解码相关信息。
  AVCodec* pCodec;  // AVCodec是存储编解码器信息的结构体。
  AVPacket pkt;  // AVPacket是存储压缩编码数据相关信息的结构体
  uint8_t* picture_buf;
  AVFrame* pFrame;  // AVFrame是包含码流参数较多的结构体
  int picture_size;
  int y_size;
  int framecnt = 0;
  // FILE *in_file = fopen("src01_480x272.yuv", "rb"); // 输入原始YUV数据
  FILE* in_file = fopen("../ds_480x272.yuv", "rb");  // 输入原始YUV数据
  int in_w = 480, in_h = 272;  // 输入数据的宽度和高度
  int framenum = 100;          // 要编码的帧
  // const char* out_file = "src01.h264";              // 输出文件路径
  // const char* out_file = "src01.ts";
  // const char* out_file = "src01.hevc";
  const char* out_file = "ds.h264";
  av_register_all();  // 注册ffmpeg所有编解码器
  // 方法1.
  pFormatCtx =
      avformat_alloc_context();  // 初始化 pFormatCtx。 AVFormatContext 用
                                 // avformat_alloc_context() 进行初始化
  // Guess Format
  fmt = av_guess_format(
      NULL, out_file,
      NULL);  // av_guess_format
              // 这是一个决定视频输出时封装方式的函数，其中有三个参数，写任何一个参数，都会自动匹配相应的封装方式。
  pFormatCtx->oformat = fmt;
  // 方法2.
  // avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file); //
  // 初始化输出码流的AVFormatContext fmt = pFormatCtx->oformat; Open output
  // URL
  if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) <
      0) {  // avio_open 打开输出文件
    printf("Failed to open output file! \n");
    return -1;
  }

  video_st = avformat_new_stream(pFormatCtx, 0);  // 创建输出码流的AVStream
  video_st->time_base.num = 1;                    // num 分子
  video_st->time_base.den = 25;                   // den 分母
  if (video_st == NULL) {
    return -1;
  }
  // 必须设置的参数
  pCodecCtx = video_st->codec;
  // pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
  pCodecCtx->codec_id = fmt->video_codec;
  pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
  pCodecCtx->pix_fmt = PIX_FMT_YUV420P;
  pCodecCtx->width = in_w;
  pCodecCtx->height = in_h;
  pCodecCtx->time_base.num = 1;
  pCodecCtx->time_base.den = 25;
  pCodecCtx->bit_rate = 400000;
  pCodecCtx->gop_size = 250;
  // H264
  // pCodecCtx->me_range = 16;
  // pCodecCtx->max_qdiff = 4;
  // pCodecCtx->qcompress = 0.6;
  pCodecCtx->qmin = 10;
  pCodecCtx->qmax = 51;
  // 可选参数
  pCodecCtx->max_b_frames = 3;
  // 设置选项
  AVDictionary* param = 0;
  // H.264
  if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
    av_dict_set(&param, "preset", "slow", 0);
    av_dict_set(&param, "tune", "zerolatency", 0);
    // av_dict_set(&param, "profile", "main", 0);
  }
  // H.265
  if (pCodecCtx->codec_id == AV_CODEC_ID_H265) {
    av_dict_set(&param, "preset", "ultrafast", 0);
    av_dict_set(&param, "tune", "zero-latency", 0);
  }
  // Show some Information
  av_dump_format(
      pFormatCtx, 0, out_file,
      1);  // av_dump_format()是一个手工调试的函数，能使我们看到pFormatCtx->streams里面有什么内容。
  pCodec = avcodec_find_encoder(pCodecCtx->codec_id);  // 查找编码器
  if (!pCodec) {
    printf("Can not find encoder! \n");
    return -1;
  }
  if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {  // 打开编码器
    printf("Failed to open encoder! \n");
    return -1;
  }
  pFrame =
      av_frame_alloc();  // AVFrame结构，av_frame_alloc申请内存，av_frame_free释放内存
  picture_size = avpicture_get_size(
      pCodecCtx->pix_fmt, pCodecCtx->width,
      pCodecCtx->height);  // 计算这个格式的图片，需要多少字节来存储
  picture_buf = (uint8_t*)av_malloc(picture_size);

  // 这个函数是为已经分配的空间的结构体AVPicture挂上一段用于保存数据的空间
  avpicture_fill((AVPicture*)pFrame, picture_buf, pCodecCtx->pix_fmt,
                 pCodecCtx->width, pCodecCtx->height);

  // 写文件头（对于某些没有文件头的封装格式，不需要此函数。比如说MPEG2TS）。
  avformat_write_header(pFormatCtx, NULL);
  av_new_packet(&pkt, picture_size);  // 分配数据包的有效size并初始化
  y_size = pCodecCtx->width * pCodecCtx->height;

  // 一帧一帧循环操作
  for (int i = 0; i < framenum; i++) {
    // Read raw YUV data
    if (fread(picture_buf, 1, y_size * 3 / 2, in_file) <=
        0) {  // fread函数，从文件流中读取数据，如果不成功或读到文件末尾返回 0
      printf("Failed to read raw data! \n");
      return -1;
    } else if (feof(in_file)) {  // 判断文件是否结束
      break;
    }
    pFrame->data[0] = picture_buf;                   // Y
    pFrame->data[1] = picture_buf + y_size;          // U
    pFrame->data[2] = picture_buf + y_size * 5 / 4;  // V
    // PTS
    pFrame->pts =
        i;  // pts : 以时间为基本单位的表示时间戳（应该向用户显示帧的时间）。
    int got_picture = 0;

    // 编码一帧视频。即将AVFrame（存储YUV像素数据）编码为AVPacket（存储H.264等格式的码流数据）。
    // 成功时返回0，失败时返回负错误代码 失败时返回错误返回码
    int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
    if (ret < 0) {
      printf("Failed to encode! \n");
      return -1;
    }
    if (got_picture == 1) {
      printf("Succeed to encode frame: %5d\tsize:%5d\n", framecnt, pkt.size);
      framecnt++;
      pkt.stream_index = video_st->index;
      ret = av_write_frame(pFormatCtx, &pkt);  // 将编码后的视频码流写入文件,
      av_free_packet(&pkt);                    // free
    }
  }
  // Flush Encoder
  int ret = flush_encoder(
      pFormatCtx,
      0);  // 输入的像素数据读取完成后调用此函数，用于输出编码器中剩余的AVPacket
  if (ret < 0) {
    printf("Flushing encoder failed\n");
    return -1;
  }

  // 写文件尾(对于某些没有文件头的封装格式，不需要此函数。比如说MPEG2TS)
  av_write_trailer(pFormatCtx);

  // Clean
  if (video_st) {
    avcodec_close(video_st->codec);
    av_free(pFrame);
    av_free(picture_buf);
  }
  avio_close(pFormatCtx->pb);
  avformat_free_context(pFormatCtx);
  fclose(in_file);
  return 0;
}