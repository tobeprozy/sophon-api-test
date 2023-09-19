#include <stdio.h>
#include <queue>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/log.h>
}

class H264Decoder
{
public:
    enum DecodeMode
    {
        VIDEO_DECODE_MODE_IPB,
        VIDEO_DECODE_MODE_I // Add a decode mode for only I frames
    };

    H264Decoder()
    {
        avcodec_register_all();
    }
    int Init()
    {
        // Initialize FFmpeg components
        AVDictionary *opts = NULL;
        av_log_set_level(AV_LOG_VERBOSE);

        // Find H.264 decoder
        decoder = find_bm_decoder((AVCodecID)0, "h264_bm", 1, AVMEDIA_TYPE_VIDEO);
        if (!decoder)
        {
            printf("find codec failed %s \n", "h264_bm");
            return -1;
        }

        parser = av_parser_init(decoder->id);
        if (!parser)
        {
            printf("parser not found \n");
            return -1;
        }
        video_dec_ctx = avcodec_alloc_context3(decoder);
        if (!video_dec_ctx)
        {
            printf("avcodec_alloc_context3 failed \n");
            return -1;
        }

        av_dict_set_int(&opts, "zero_copy", 0, 0);
        av_dict_set_int(&opts, "sophon_idx", 0, 0);
        av_dict_set_int(&opts, "output_format", 0, 18); // non-compressed format
        av_dict_set_int(&opts, "extra_frame_buffer_num", 2, 0);

        if (avcodec_open2(video_dec_ctx, decoder, &opts) < 0)
        {
            av_dict_free(&opts);
            printf("avcodec_open2 failed \n");
            return -1;
        }

        av_dict_free(&opts);

        return 0;
    }

    int DecodePacket(uint8_t *data, int size)
    {
        int ret = 0;

        do{
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;
        frame = av_frame_alloc();
        }while(false);
        
        
        if (!frame)
        {
            printf("sophon av_frame_alloc failed \n");
            return -1;
        }
        printf("sophon video decode start success\n");

        if (!_first_i)
        {
            if ((data[4] & 0x1f) != 7)
            {
                printf("first is not i frame, skipped %d\n", data[4] & 0x1f);
                av_frame_free(&frame); // Free the frame
                return 0;
            }
            _first_i = true;
        }

        if (_decode_type == VIDEO_DECODE_MODE_I && ((data[4] & 0x1f) != 7))
        {
            printf("config to decode i. skip p frame\n");
            av_frame_free(&frame); // Free the frame
            return 0;
        }

        //
        pkt.size = size;
        pkt.data = data;

        if ((data[4] & 0x1f) == 7)
        {
            pkt.flags |= AV_PKT_FLAG_KEY;
        }

        ret = avcodec_send_packet(video_dec_ctx, &pkt);
        if (ret < 0)
        {
            printf("Error sending a packet for decoding\n");
            return -1;
        }

        // Print width, height, and pixel format after decoding the first frame
        if (_frame_count == 0)
        {
            printf("sophon open success, width=%d height=%d pix_fmt=%d \n",
                   video_dec_ctx->width, video_dec_ctx->height, video_dec_ctx->pix_fmt);
        }

        ret = avcodec_receive_frame(video_dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return 0;
        }
        else if (ret < 0)
        {
            printf("Error during decoding\n");
            return -1;
        }
        _frame_count++;

        ProcessDecodedFrame(frame);

        return 0;
    }
    AVFrame *GetDecodedFrame()
    {
        if (!decodedFrames.empty())
        {
            AVFrame *frame = decodedFrames.front();
            decodedFrames.pop();
            return frame;
        }
        return nullptr;
    }

    void Cleanup()
    {
        if (frame)
        {
            av_frame_free(&frame);
            frame = NULL;
        }
        if (video_dec_ctx)
        {
            avcodec_free_context(&video_dec_ctx);
            video_dec_ctx = NULL;
        }
    }

private:
    AVCodec *find_bm_decoder(AVCodecID dec_id, const char *name, int codec_name_flag, enum AVMediaType type)
    {
        /* find video decoder for the stream */
        AVCodec *codec = NULL;
        if (codec_name_flag && type == AVMEDIA_TYPE_VIDEO)
        {
            const AVCodecDescriptor *desc;
            const char *codec_string = "decoder";

            codec = avcodec_find_decoder_by_name(name);
            if (!codec && (desc = avcodec_descriptor_get_by_name(name)))
            {
                codec = avcodec_find_decoder(desc->id);
            }

            if (!codec)
            {
                av_log(NULL, AV_LOG_FATAL, "Unknown %s '%s'\n", codec_string, name);
                exit(1);
            }
            if (codec->type != type)
            {
                av_log(NULL, AV_LOG_FATAL, "Invalid %s type '%s'\n", codec_string, name);
                exit(1);
            }
        }
        else
        {
            codec = avcodec_find_decoder(dec_id);
        }

        if (!codec)
        {
            fprintf(stderr, "Failed to find %s codec\n", av_get_media_type_string(type));
            exit(1);
        }
        return codec;
    }

private:
    DecodeMode _decode_type = VIDEO_DECODE_MODE_IPB;
    AVCodec *decoder;
    std::queue<AVFrame *> decodedFrames;

    // Sophon ffmpeg
    AVCodecContext *video_dec_ctx;
    AVCodecParameters *video_dec_par;
    int width;
    int height;
    int pix_fmt;
    int data_size;

    AVCodecParserContext *parser;

    int video_stream_idx;
    AVFrame *frame;
    AVPacket pkt;

    // record
    uint32_t _frame_count = 0;
    bool _start = false;
    bool _first_i = false;

    void ProcessDecodedFrame(AVFrame *frame)
    {
        // Example: Push frame to the queue
        decodedFrames.push(frame);
    }
};

int main()
{
    H264Decoder decoder;
    if (decoder.Init() != 0)
    {
        return -1;
    }

    // Open H.264 file for reading
    const char *inputFile = "../elevator-1080p-25fps-4000kbps.h264";
    FILE *file = fopen(inputFile, "rb");
    if (!file)
    {
        fprintf(stderr, "Failed to open file for reading\n");
        return -1;
    }

    // Read and decode H.264 packets from file
    uint8_t packetData[4096 * 8]; // Adjust buffer size as needed
    while (true)
    {
        size_t bytesRead = fread(packetData, 1, sizeof(packetData), file);
        if (bytesRead <= 0)
        {
            break; // End of file or error
        }
        decoder.DecodePacket(packetData, bytesRead);

        // Process and display/save the decoded frames
        AVFrame *decodedFrame;
        while ((decodedFrame = decoder.GetDecodedFrame()) != nullptr)
        {
            // Free the frame after processing
            av_frame_free(&decodedFrame);
        }
    }

    // Close the file
    fclose(file);

    // Cleanup
    decoder.Cleanup();

    return 0;
}
