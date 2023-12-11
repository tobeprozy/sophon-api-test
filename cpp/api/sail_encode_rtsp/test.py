import sophon.sail as sail
 
def main():
    dev_id = 0
    decformat = "h264"
    h264_decoder = sail.Decoder_RawStream(dev_id, decformat)
 
    handle = sail.Handle(dev_id)
    bmcv = sail.Bmcv(handle)
 
 
    # 读取H.264数据并处理图像
    with open("/home/zhiyuanzhang/sophon/sophon_api_test/datasets/videos/elevator-1080p-25fps-4000kbps.h264", "rb") as h264_file:
        h264_data = h264_file.read()
        image = sail.BMImage()  # 创建bm_image对象
        continue_frame = True
    while(True):
        result = h264_decoder.read(h264_data,image, continue_frame)
 
        if result == 0:
            print("成功处理H.264数据。")
        else:
            print("处理H.264数据时出错。")
 
        bmcv.imwrite("save_path.jpg", image)
 
if __name__ == '__main__':
    main()