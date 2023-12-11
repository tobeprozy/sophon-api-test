from multiprocessing import Process, Queue
import multiprocessing
import sophon.sail as sail
import os

decoder_count=0
encoder_count=0


def process_video(input_path, output_path, stop_event):
    handle = sail.Handle(0)
    bmcv = sail.Bmcv(handle)
    decoder = sail.Decoder(input_path, True, 0)
    bimg = sail.BMImage()

    enc_fmt = "h264_bm"
    pix_fmt = "NV12"
    enc_params = "width=1280:height=720:gop=30:gop_preset=3:framerate=25:bitrate=2000"
    encoder = sail.Encoder(output_path, handle, enc_fmt, pix_fmt, enc_params)

    while not stop_event.is_set():
        ret = decoder.read(handle, bimg)
        if ret != 0:
            break
        ret = encoder.video_write(bimg)
    
    decoder.release()
    print('decoder over, process id:', os.getpid())
    encoder.release()
    print('encoder over, process id:', os.getpid())

if __name__ == "__main__":
    to_main_queue = multiprocessing.Queue()
    from_main_queue = multiprocessing.Queue()
    video_info = [
        {"input_path": "dmq1.mp4", "output_path": "rtmp://localhost:1935/hls/1"},
        {"input_path": "dmq1.mp4", "output_path": "rtmp://localhost:1935/hls/2"},
        {"input_path": "dmq1.mp4", "output_path": "rtmp://localhost:1935/hls/3"},
        {"input_path": "dmq1.mp4", "output_path": "rtmp://localhost:1935/hls/4"},
        {"input_path": "dmq1.mp4", "output_path": "rtmp://localhost:1935/hls/5"},
        {"input_path": "dmq1.mp4", "output_path": "rtmp://localhost:1935/hls/6"},
        {"input_path": "dmq1.mp4", "output_path": "rtmp://localhost:1935/hls/7"}
    ]

    handle = sail.Handle(0)
    bmcv = sail.Bmcv(handle)
    processes = []
    
    for i in range(15):
        stop_event = multiprocessing.Event()
        for info in video_info:
            process = Process(target=process_video, args=(info["input_path"], info["output_path"], stop_event))
            processes.append(process)
            process.start()
        
        # Wait for all processes to finish
        for process in processes:
            process.join()

        print("Program terminated.")
