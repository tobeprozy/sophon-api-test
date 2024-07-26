extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <iostream>

int main() {
    avformat_network_init();

    // 打开 RTSP 视频流
    AVFormatContext *formatContext = avformat_alloc_context();
	AVDictionary *options = nullptr;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
	av_dict_set(&options, "keep_rtsp_timestamp", "1", 0);

    const char *url = "rtsp://192.168.1.151:9954/cam_passby_chn5";

    if (avformat_open_input(&formatContext, url, nullptr, &options) != 0) {
        std::cerr << "Error: Could not open RTSP stream." << std::endl;
        return -1;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        std::cerr << "Error: Could not find stream info." << std::endl;
        avformat_close_input(&formatContext);
        return -1;
    }

    // 查找视频流
    int videoStreamIndex = -1;
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        std::cerr << "Error: Could not find video stream." << std::endl;
        avformat_close_input(&formatContext);
        return -1;
    }

    AVPacket packet;
    while (av_read_frame(formatContext, &packet) >= 0) {
        if (packet.stream_index == videoStreamIndex) {
            // 获取时间戳（单位是 AVStream.time_base）
            int64_t pts = packet.pts;
            int64_t dts = packet.dts;
            AVRational time_base = formatContext->streams[videoStreamIndex]->time_base;

            // 转换时间戳为秒
            double pts_seconds = pts * av_q2d(time_base);
            double dts_seconds = dts * av_q2d(time_base);

            std::cout << "PTS: " << pts << " (" << pts_seconds << " seconds), "
                      << "DTS: " << dts << " (" << dts_seconds << " seconds)" << std::endl;
        }
        av_packet_unref(&packet);
    }

    avformat_close_input(&formatContext);
    avformat_network_deinit();

    return 0;
}