import sophon.sail as sail
import numpy as np
dev_id=0
handle=sail.Handle(dev_id)
bmcv=sail.Bmcv(handle)

decoder = sail.Decoder("test.jpg", True, 0)

bimg = sail.BMImage()
ret = decoder.read(handle, bimg)

crop_bmimg = bmcv.crop(bimg, 200, 200, 18, 18)

crop_img = crop_bmimg.asmat()


bmcv.imwrite('result/1.jpg', crop_bmimg)