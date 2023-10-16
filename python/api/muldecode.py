import sophon.sail as sail

dev_id=0
handle=sail.Handle(dev_id)
bmcv=sail.Bmcv(handle)
muldecode=sail.MultiDecoder(20,0,0)

channel_num=muldecode.add_channel("dance_1080P.mp4",0)

bimg = sail.BMImage()

ret = muldecode.read(0, bimg, 1)
print(ret)
print(bimg)
image = bimg.asmat()
print(image)