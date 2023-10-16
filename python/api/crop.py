import sophon.sail as sail
import numpy as np
dev_id=0
handle=sail.Handle(dev_id)
bmcv=sail.Bmcv(handle)

decoder = sail.Decoder("test.jpg", True, 0)

bimg = sail.BMImage()
ret = decoder.read(handle, bimg)

crop_bmimg = bmcv.crop(bimg, 200, 200, 500, 500)

crop_img = crop_bmimg.asmat()


rgb_transpose=crop_img.transpose(2,0,1)
# 形状为：[1, 3, 512, 1024]
rgb_transpose_expand = np.expand_dims(rgb_transpose, axis=0)

rgb_transpose_tensor=sail.Tensor(handle,rgb_transpose_expand,False)
rgb_transpose_bmimg=bmcv.tensor_to_bm_image(rgb_transpose_tensor)



bmcv.imwrite('1.jpg', crop_bmimg)
bmcv.imwrite('2.jpg', rgb_transpose_bmimg)