import cv2
import sophon.sail as sail
import numpy as np

handle = sail.Handle(0)
bmcv = sail.Bmcv(handle)

mask = np.full((100, 100, 3), 255, np.uint8)
mask_bmimg = sail.Tensor(handle, np.expand_dims(mask.transpose(2, 0, 1), 0), False)
mask_bmimg = bmcv.tensor_to_bm_image(mask_bmimg)
mask_bmimg_rgb = sail.BMImage(handle, mask_bmimg.height(), mask_bmimg.width(),
                                        sail.Format.FORMAT_RGB_PLANAR, sail.DATA_TYPE_EXT_1N_BYTE)
bmcv.convert_format(mask_bmimg, mask_bmimg_rgb)

# img_file = "./bus.jpg"
# decoder = sail.Decoder(img_file, True, 0)
# draw_frame = sail.BMImage()
# ret = decoder.read(handle, draw_frame)

image_np = np.zeros((640, 640, 3), np.uint8)
draw_frame = sail.Tensor(handle, np.expand_dims(image_np.transpose(2, 0, 1), 0), False)
draw_frame = bmcv.tensor_to_bm_image(draw_frame)
draw_frame_rgb = sail.BMImage(handle, draw_frame.height(), draw_frame.width(),
                                        sail.Format.FORMAT_RGB_PLANAR, sail.DATA_TYPE_EXT_1N_BYTE)
bmcv.convert_format(draw_frame, draw_frame_rgb)

bmcv.imwrite("./test1.jpg", draw_frame)


bmcv.image_copy_to(mask_bmimg_rgb, draw_frame_rgb, 100, 100)
# bmcv.image_copy_to_padding(mask_bmimg_rgb, draw_frame_rgb, 0, 0, 0, 100, 100)
bmcv.imwrite("./test2.jpg", draw_frame_rgb)

