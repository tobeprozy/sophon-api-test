#!/bin/bash
nohup ffmpeg -benchmark -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -i video_h264_1080p.mp4 -f null /dev/null 2>&1 >> dec264log1.txt &
nohup ffmpeg -benchmark -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -i video_h264_1080p.mp4 -f null /dev/null 2>&1 >> dec264log2.txt &
nohup ffmpeg -benchmark -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -i video_h264_1080p.mp4 -f null /dev/null 2>&1 >> dec264log3.txt &
nohup ffmpeg -benchmark -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -i video_h264_1080p.mp4 -f null /dev/null 2>&1 >> dec264log4.txt &
nohup ffmpeg -benchmark -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -i video_h264_1080p.mp4 -f null /dev/null 2>&1 >> dec264log5.txt &
nohup ffmpeg -benchmark -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -i video_h264_1080p.mp4 -f null /dev/null 2>&1 >> dec264log6.txt &
nohup ffmpeg -benchmark -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -i video_h264_1080p.mp4 -f null /dev/null 2>&1 >> dec264log7.txt &
nohup ffmpeg -benchmark -hwaccel bmcodec -hwaccel_device 0 -c:v h264_bm -cbcr_interleave 0 -i video_h264_1080p.mp4 -f null /dev/null 2>&1 >> dec264log8.txt &
wait
echo "\n Num 1:"
cat dec264log1.txt|grep time

echo "\n Num 2:"
cat dec264log2.txt|grep time

echo "\n Num 3:"
cat dec264log3.txt|grep time

echo "\n Num 4:"
cat dec264log4.txt|grep time

echo "\n Num 5:"
cat dec264log5.txt|grep time

echo "\n Num 6:"
cat dec264log6.txt|grep time

echo "\n Num 7:"
cat dec264log7.txt|grep time

echo "\n Num 8:"
cat dec264log8.txt|grep time

echo "\nfinish\n"
rm dec264log*.txt
