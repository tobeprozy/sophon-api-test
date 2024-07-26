import sophon.sail as sail
 
def main():
    input_file_path = 'rtsp://172.26.13.138:8554/stream0'  
    dev_id = 0
    handle = sail.Handle(dev_id)
    decoder = sail.Decoder(input_file_path, True, dev_id)
    image = sail.BMImage()
    bmcv=sail.Bmcv(handle)
    img_num=0;
    while True:
        ret = decoder.read(handle, image)
        bmcv.imwrite('1.jpg', image)
        img_num+=1
        # pts=decoder.get_stamp(0)
        # dts=decoder.get_stamp(1)
        # print("pts:",pts)
        # print("dts:",dts)
    
 
if __name__ == '__main__':
    main()

