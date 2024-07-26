import cv2
import os
import time
import logging

# 假设我们有一个包含RTSP URL的列表
rtsp_streams = ['rtsp://172.28.3.201:5544/vod/123/out264-20700.264',
                 'rtsp://172.28.3.201:5544/vod/123/out264-20700.264', 
                 'rtsp://172.28.3.201:5544/vod/123/out264-20700.264',
                  'rtsp://172.28.3.201:5544/vod/123/out264-20700.264',
                   'rtsp://172.28.3.201:5544/vod/123/out264-20700.264',
                    'rtsp://172.28.3.201:5544/vod/123/out264-20700.264']

# 你可以自己指定每个视频解码的帧数
frames_to_capture = 1000

# 输出目录
output_dir = 'results'

# 确保输出目录存在
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

# 为每个RTSP流创建VideoCapture和VideoWriter对象
caps = []
outs = []
for i,stream in enumerate(rtsp_streams):
    cap = cv2.VideoCapture(stream)
    if not cap.isOpened():
        raise Exception("Cannot open the video stream: " + stream)

    fourcc = cv2.VideoWriter_fourcc(*'XVID')
    fps = cap.get(cv2.CAP_PROP_FPS)
    size = (int(cap.get(cv2.CAP_PROP_FRAME_WIDTH)), int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT)))

    output_video_path = os.path.join(output_dir, 'output_' + os.path.basename(stream)+str(i) + '.avi')
    out = cv2.VideoWriter(output_video_path, fourcc, fps, size)

    caps.append(cap)
    outs.append(out)

# 解码并保存视频
for frame_count in range(frames_to_capture):
    for i, cap in enumerate(caps):
        print(f"Decoding frame {frame_count} from stream {rtsp_streams[i]}")
        ret, frame = cap.read()
        if not ret or frame is None:
            logging.warning("Frame not received from stream: " + rtsp_streams[i])
            continue
        res_frame = frame
        outs[i].write(res_frame)

# 释放所有资源
for cap in caps:
    cap.release()
for out in outs:
    out.release()
