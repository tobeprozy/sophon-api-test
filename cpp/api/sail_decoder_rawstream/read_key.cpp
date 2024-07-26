    class Decoder_RawStream::Decoder_RawStream_CC{
    public:
        explicit Decoder_RawStream_CC(int tpu_id,
            string  decformt);
        ~Decoder_RawStream_CC();

        int read_(uint8_t* data, int data_size, bm_image &image,bool continueFrame);
        int read(uint8_t* data, int data_size, sail::BMImage &image,bool continueFrame);
        void release();

        #ifdef PYTHON
            int read_(pybind11::bytes data_bytes, bm_image& image, bool continueFrame);
            int read(pybind11::bytes data_bytes, BMImage& image, bool continueFrame);
        #endif

        typedef struct {
        uint8_t* start;
        int      size;
        int      pos;
        } bs_buffer_t;

        //控制流向
        static int read_buffer(void *opaque, uint8_t *buf, int buf_size)
        {
            bs_buffer_t* bs = (bs_buffer_t*)opaque;

            int r = bs->size - bs->pos;
            if (r <= 0) {
                cout << "EOF of AVIO." << endl;
                return AVERROR_EOF;
            }

            uint8_t* p = bs->start + bs->pos;
            int len = (r >= buf_size) ? buf_size : r;
            memcpy(buf, p, len);
            //cout << "read " << len << endl;
            bs->pos += len;
            return len;
        }

    private:
    
    AVFormatContext *pFormatCtx = nullptr;
    AVCodecContext  *dec_ctx = nullptr;
    AVFrame         *pFrame = nullptr;
    AVPacket         pkt;
    AVDictionary    *dict = nullptr;
    AVIOContext     *avio_ctx = nullptr;
    AVCodec         *pCodec = nullptr;
    AVInputFormat   *iformat = nullptr;

    int got_picture;
    int ret;
    sail::Handle handle;

    uint8_t* bs_buffer = nullptr;

    bs_buffer_t bs_obj = {0, 0, 0};

    int      aviobuf_size = 32*1024; // 32K
    uint8_t *aviobuffer = nullptr;

    // Select the correct decoder based on the input format
    const char* iformat_name;
    const char* decoder_input;    

    };
    
Decoder_RawStream::Decoder_RawStream_CC::Decoder_RawStream_CC(int tpu_id,string  decformt){
        
        handle=sail::Handle(tpu_id);
        if (decformt == "h264") {
            // If the input format is h264, set the format name and decoder input accordingly
            iformat_name = "h264";
            decoder_input = "h264_bm";
        } else if (decformt == "h265") {
            // If the input format is h265, set the format name and decoder input accordingly
            iformat_name = "hevc";
            decoder_input = "hevc_bm";
        } else {
            // If the input format is neither h264 nor h265, throw an exception
            throw std::invalid_argument("Invalid decoder_input");
        }

        /* h264/h265 */
        iformat = av_find_input_format(iformat_name);
        if (iformat == NULL) {
            cout << "av_find_input_format failed." << endl;
            ret = AVERROR_DEMUXER_NOT_FOUND;
            throw std::invalid_argument("av_find_input_format failed.");
        }

        /* HW h264/H265 decoder: h264/H264*/
        pCodec = avcodec_find_decoder_by_name(decoder_input);
        if (pCodec == NULL) {
            cout << "Codec not found." << endl;
            ret = AVERROR_DECODER_NOT_FOUND;
            throw std::invalid_argument("Codec not found.");

        }

        dec_ctx = avcodec_alloc_context3(pCodec);
        if (dec_ctx == NULL) {
            cout << "Could not allocate video codec context!" << endl;
            ret = AVERROR(ENOMEM);
            throw std::invalid_argument("Could not allocate video codec context!");
        }

        ret = avcodec_open2(dec_ctx, pCodec, &dict);
        if (ret < 0) {
            cout << "Could not open codec." << endl;
            ret = AVERROR_UNKNOWN;
            throw std::invalid_argument("Could not open codec.");

        } 

        pFrame = av_frame_alloc();
        if (pFrame == nullptr) {
            cout << "av frame malloc failed" << endl;  
            throw std::invalid_argument("av frame malloc failed");
        }   

        pFormatCtx = avformat_alloc_context();

        AVPacket *pkt = av_packet_alloc();
        if (!pkt) {
            throw std::invalid_argument("Failed to allocate packet.");
        }

    }


// Function to read from the decoder
int  Decoder_RawStream::Decoder_RawStream_CC::read_(uint8_t* data, int data_size, bm_image &image, bool continueFrame) {

        bs_buffer = data;
        if (bs_buffer == nullptr) {
            cout << "Invalid h264 data" << endl;
            throw std::invalid_argument("Invalid h264 data");
        }
    
        bs_obj.start = bs_buffer;
        bs_obj.size  = data_size;
        //读取一帧
        if (continueFrame==false)
        {
            bs_obj.pos =0;
        }       
        if (bs_obj.pos ==0)//连续读帧，调用一次读一次，但是只能初始化一次
        {       
            uint8_t *aviobuffer = (uint8_t *)av_malloc(aviobuf_size);
            if (!aviobuffer) {
                std::cerr << "Could not allocate buffer\n";
                avformat_free_context(pFormatCtx);
                return -1;
            }
            avio_ctx = avio_alloc_context(aviobuffer, aviobuf_size, 0,
                                        (void*)(&bs_obj), read_buffer, NULL, NULL);
            if (avio_ctx == NULL)
            {
                cout << "avio_alloc_context failed" << endl;
                ret = AVERROR(ENOMEM);
                throw std::invalid_argument("Invalid h264 data");
            }
            pFormatCtx->pb = avio_ctx;
            /* Open an input stream */
            ret = avformat_open_input(&pFormatCtx, NULL, iformat, NULL);
            if (ret != 0) {
                cout << "Couldn't open input stream.\n" << endl;
                throw std::invalid_argument("Couldn't open input stream.");
            }

            if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
                std::cerr << "Could not find stream information\n";
                return -1;
            }
        }
        
        if (av_read_frame(pFormatCtx, &pkt) >= 0) {
            ret=avcodec_send_packet(dec_ctx,&pkt);

            if (ret < 0) {
                fprintf(stderr, "Error sending packet for decoding.\n");
                return -1;
            }
                      
            ret = avcodec_receive_frame(dec_ctx, pFrame);
            if(ret < 0) {
                    fprintf(stderr, "Error during decoding.\n");
                    return -1;
            }
                // Process the decoded frame (e.g., save it, display it, etc.)
            printf("Decoded frame: %d\n", pFrame->coded_picture_number);
            avframe_to_bm_image(handle,*pFrame, image);
        }
        return 0; // Or return other values as needed
}