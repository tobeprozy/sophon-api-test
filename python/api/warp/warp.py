



import cv2
import numpy as np
import sophon.sail as sail


filepath="123.jpg"
dev_id=0

decoder = sail.Decoder(filepath, True, dev_id)

handle = sail.Handle(dev_id)

bmcv = sail.Bmcv(handle)

crop_img = sail.BMImage()
ret = decoder.read(handle, crop_img)    
print(crop_img.format(), crop_img.dtype())


rgb_planar_img=sail.BMImage(handle, crop_img.height(), crop_img.width(),
                                          sail.Format.FORMAT_RGB_PLANAR, sail.DATA_TYPE_EXT_1N_BYTE)
bmcv.convert_format(crop_img, rgb_planar_img)


crop_img_w = rgb_planar_img.width()    # crop_img  为旋转原图
crop_img_h = rgb_planar_img.height()      # crop_img  为旋转原图

rotate_matrix = cv2.getRotationMatrix2D((crop_img_w / 2, crop_img_h / 2), angle=90, scale=1)
# print(rotate_matrix)
cos = np.abs(rotate_matrix[0, 0])
sin = np.abs(rotate_matrix[0, 1])
nW = int((crop_img_h * sin) + (crop_img_w * cos))
nH = int((crop_img_h * cos) + (crop_img_w * sin))
rotate_matrix[0, 2] += (nW / 2) - crop_img_w / 2
rotate_matrix[1, 2] += (nH / 2) - crop_img_h / 2

rotate_matrix33 = np.insert(rotate_matrix, 2, [0, 0, 1], axis=0)
rotate_invmatrix = np.zeros((3, 3), dtype=np.float64)
cv2.invert(rotate_matrix33, rotate_invmatrix, flags=cv2.DECOMP_LU)


output_img=sail.BMImage(handle,crop_img.width(),crop_img.height(),
                                          sail.Format.FORMAT_RGB_PLANAR, sail.DATA_TYPE_EXT_1N_BYTE)
print(output_img.format(), output_img.dtype())
print(output_img.width(), output_img.height())


bmcv.warp(rgb_planar_img,output_img,rotate_invmatrix[0:2, :])

print(output_img.format(), output_img.dtype())
print(output_img.width(), output_img.height())

bmcv.imwrite("output.jpg", output_img)