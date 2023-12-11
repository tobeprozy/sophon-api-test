import sophon.sail as sail
import numpy as np
import cv2
dev_id=0
handle=sail.Handle(dev_id)
bmcv=sail.Bmcv(handle)

decoder = sail.Decoder("test.jpg", True, 0)
bimg = sail.BMImage()
ret = decoder.read(handle, bimg)

rgb_planar_img = sail.BMImage(handle, bimg.height(), bimg.width(),
                                          sail.Format.FORMAT_RGB_PLANAR, sail.DATA_TYPE_EXT_1N_BYTE)
bmcv.convert_format(bimg, rgb_planar_img)

# cvimg=bimg.asmat()

crop_bmimg = bmcv.crop(rgb_planar_img, 200, 200, 18, 18)

crop_img = crop_bmimg.asmat()

print(crop_img)
cv2.imwrite('result/cvimg.jpg', crop_img)