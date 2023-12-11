import sophon.sail as sail
import cv2 


if __name__ == "__main__":

    handle= sail.Handle(0)
    bmcv =sail.Bmcv(handle)
    output_name = ".rtmp::/localhost:1935/zjyn"
    enc_fmt="h264_bm"
    pix_fmt="I4200"
    enc_params ="width=1280:height=720:gop=30:gop_preset=3:framerate=25:bitrate=2000"

    i =0 
    while True:
        encoder =sail.Encoder(output_name,handle,enc_fmt,pix_fmt,enc_params)
        encoder.release()
        i+=1
        print(f"encoder.release() time {i}")
        if(i==1000):
            break
            