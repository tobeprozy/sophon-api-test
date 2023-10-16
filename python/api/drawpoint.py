import sophon.sail as sail
 
def main():
    filepath="/home/zhiyuanzhang/sophon/sophon_api_test/datasets/images/1920x1080_yuvj420.jpg"
    dev_id = 0
    decoder = sail.Decoder(filepath, True, dev_id)
    bmimg = sail.BMImage()
    handle=sail.Handle(dev_id)
    bmcv=sail.Bmcv(handle)

    ret = decoder.read(handle, bmimg)
    bmcv.imwrite("test.jpg", bmimg)


    bmcv.drawsquare(bmimg,300,300,30,(255,0,0));
    bmcv.imwrite("drawsquare.jpg", bmimg)
    
 
if __name__ == '__main__':
    main()