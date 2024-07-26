# import sophon.sail as sail
 
# def main():
#     dev_id = 0
#     decformat = "h264"
#     h264_decoder = sail.Decoder_RawStream(dev_id, decformat)
 
#     handle = sail.Handle(dev_id)
#     bmcv = sail.Bmcv(handle)
 
#     # 读取H.264数据并处理图像
#     with open("20240702_150921_149369_6934-67356", "rb") as h264_file:
#         h264_data = h264_file.read()
#         image = sail.BMImage()  # 创建bm_image对象
#         continue_frame = True
#     while(True):
#         result = h264_decoder.read(h264_data,image, continue_frame)
        
#         if result == 0:
#             print("成功处理H.264数据。")
#         else:
#             print("处理H.264数据时出错。")
 
#         bmcv.imwrite("save_path.jpg", image)
 
# if __name__ == '__main__':
#     main()


# rgb_planar_img = sail.BMImage(handle, image.height(), image.width(),
#                                     sail.Format.FORMAT_RGB_PLANAR, sail.DATA_TYPE_EXT_1N_BYTE)
# bmcv.convert_format(image, rgb_planar_img)

# rgb_mat = rgb_planar_img.asmat()
# nv21_mat=image.asmat()
        

import threading
import sophon.sail as sail
import os

def decode_and_save(dev_id, decformat, h264_data, save_dir):
    h264_decoder = sail.Decoder_RawStream(dev_id, decformat)
    handle = sail.Handle(dev_id)
    bmcv = sail.Bmcv(handle)
    thread_id = threading.get_ident()
    if not os.path.exists(save_dir):
        os.makedirs(save_dir)

    image = sail.BMImage()  # 创建bm_image对象
    continue_frame = True
    frame_count = 0

    while continue_frame:
        result = h264_decoder.read(h264_data, image, continue_frame)
        
        if result == 0:
            print(f"Thread {thread_id}: 成功处理H.264数据。{frame_count}")
            save_path = os.path.join(save_dir, f"frame_{frame_count}.jpg")
            bmcv.imwrite(save_path, image)
            frame_count += 1
        else:
            print(f"Thread {thread_id}: 处理H.264数据时出错。")
            continue_frame = False

def main():
    dev_id_1 = 0
    decformat = "h264"

    save_dir_1 = "output_folder_1"
    save_dir_2 = "output_folder_2"

    # 创建保存目录
    os.makedirs(save_dir_1, exist_ok=True)
    os.makedirs(save_dir_2, exist_ok=True)


    with open("20240702_150921_149369_6934-67356", "rb") as h264_file:
        h264_data = h264_file.read()

    # 创建两个线程
    thread1 = threading.Thread(target=decode_and_save, args=(dev_id_1, decformat, h264_data, save_dir_1))
    thread2 = threading.Thread(target=decode_and_save, args=(dev_id_1, decformat, h264_data, save_dir_2))

    # 启动线程
    thread1.start()
    thread2.start()

    # 等待两个线程完成
    thread1.join()
    thread2.join()

if __name__ == '__main__':
    main()
