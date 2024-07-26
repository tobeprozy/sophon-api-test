
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <iostream>
#include <fstream>


void decode_packet(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame,int *frame_count) {
    int ret;

    // 发送数据包到解码器
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        std::cerr << "Error sending a packet for decoding\n";
        return;
    }

    // 接收解码后的帧
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            std::cerr << "Error during decoding\n";
            return;
        }
        (*frame_count)++;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input file>\n";
        return -1;
    }

    const char *input_file = argv[1];

    // 注册所有编解码器和格式
    av_register_all();

    AVFormatContext *fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, input_file, nullptr, nullptr) < 0) {
        std::cerr << "Could not open source file " << input_file << "\n";
        return -1;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream information\n";
        return -1;
    }

    // av_dump_format(fmt_ctx, 0, input_file, 0);

    // 查找视频流
    int video_stream_index = -1;
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        std::cerr << "Could not find video stream in the input file\n";
        return -1;
    }

    AVCodecParameters *codecpar = fmt_ctx->streams[video_stream_index]->codecpar;
    AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << "Failed to find codec\n";
        return -1;
    }

    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "Failed to allocate codec context\n";
        return -1;
    }

    if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
        std::cerr << "Failed to copy codec parameters to codec context\n";
        return -1;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Failed to open codec\n";
        return -1;
    }

    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        std::cerr << "Failed to allocate packet\n";
        return -1;
    }

    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Failed to allocate frame\n";
        return -1;
    }

    // 创建 RGB 帧
    AVFrame *rgb_frame = av_frame_alloc();
    if (!rgb_frame) {
        std::cerr << "Failed to allocate RGB frame\n";
        return -1;
    }


    int frame_count = 0;
    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        if (pkt->stream_index == video_stream_index) {
            decode_packet(codec_ctx, pkt, frame,&frame_count);
        }
        av_packet_unref(pkt);
    }

    // Flush the decoder
    decode_packet(codec_ctx, nullptr, frame, &frame_count);

    std::cout << "Decoding finished, total frames: " << frame_count << "\n";

    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    av_packet_free(&pkt);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    // sws_freeContext(sws_ctx);

    return 0;
}
