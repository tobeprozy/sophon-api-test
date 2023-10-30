import cv2
import os
import numpy as np


def _imread(pic_path):
    img = cv2.imdecode(np.fromfile(pic_path, dtype=np.uint8), -1)
    if img is None:
        raise ValueError("Failed to read image: {}".format(pic_path))
    img = cv2.cvtColor(img, cv2.COLOR_RGB2BGR)
    return img


script_dir = os.path.dirname(os.path.abspath(__file__))
pic_path = os.path.join(script_dir, "../../../datasets/images/test1")
result_dir=os.path.join(script_dir,"results")

size = (640, 360)

if not os.path.exists(pic_path) or not os.path.isdir(pic_path):
    raise ValueError("Invalid picture directory: {}".format(pic_path))
if not os.path.exists(result_dir):
    os.mkdir(result_dir)

fps=25
videowriter = cv2.VideoWriter(result_dir+"/test.mp4", cv2.VideoWriter_fourcc('M', 'J', 'P', 'G'), fps, size)

for file_name in os.listdir(pic_path):
    file_path = os.path.join(pic_path, file_name)
    if not os.path.isfile(file_path):
        continue
    try:
        img = _imread(file_path)
        img = cv2.resize(img, size)
        output_path = os.path.join(result_dir, file_name)
        cv2.imwrite(output_path, img)
        videowriter.write(img)
        print(output_path)
    except Exception as e:
        print("Failed to process image {}: {}".format(file_path, str(e)))