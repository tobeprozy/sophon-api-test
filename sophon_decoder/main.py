import importlib
import threading

import sophon.sail as sail
import yaml


def get_inference(entrance, config):
    inference_file = importlib.import_module(entrance)
    inference = inference_file.Inference(config)
    return inference


def process(stream_url):
    config_file = './models/yolox_bmodel.yaml'

    with open(f'{config_file}', errors='ignore') as f:
        model_config = yaml.safe_load(f)

    inference = get_inference(model_config['entrance'], model_config)

    decoder = sail.Decoder(stream_url, False, 0)
    BMimg = sail.BMImage()
    while True:
        ret = decoder.read(inference.handle, BMimg)
        if not ret:
            infer_outputs, timestamps = inference.infer(image=BMimg)
            # print(infer_outputs.shape)



stream1 = "rtsp://admin:wpkjipc01@172.16.0.201:554/ch1/main/av_stream"
stream2 = "rtsp://admin:wpkjipc01@172.16.0.201:554/ch1/main/av_stream"

t1 = threading.Thread(target=process, args=(stream1))
t2 = threading.Thread(target=process, args=(stream2))
# logger.info(5)

t1.start()
t2.start()
t1.join()
t2.join()




