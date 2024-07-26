#!/usr/bin/env python3
# -*- coding:utf-8 -*-

import os
import sys
sys.path.append(f'{os.path.dirname(__file__)}')

import time
import numpy as np
import sophon.sail as sail
from BM_NMS import multiclass_nms

class Inference():
    def __init__(self, yaml_dict):
        print("---- Inference PySail Runtime ... ----")
        self.config = yaml_dict

        # load bmodel
        self.model = self.load_model(yaml_dict)
        self.handle = self.model.get_handle()
        self.bmcv = sail.Bmcv(self.handle)
        self.graph_name = self.model.get_graph_names()[0]

        # get input
        self.input_name = self.model.get_input_names(self.graph_name)[0]
        self.input_dtype = self.model.get_input_dtype(self.graph_name, self.input_name)
        self.img_dtype = self.bmcv.get_bm_image_data_format(self.input_dtype)
        self.input_scale = self.model.get_input_scale(self.graph_name, self.input_name)
        self.input_shape = self.model.get_input_shape(self.graph_name, self.input_name)
        self.input_shapes = {self.input_name: self.input_shape}

        # get output
        self.output_names = self.model.get_output_names(self.graph_name)
        if len(self.output_names) not in [1, 3]:
            raise ValueError('only suport 1 or 3 outputs, but got {} outputs bmodel'.format(len(self.output_names)))
        self.output_tensors = {}
        self.output_scales = {}
        for output_name in self.output_names:
            output_shape = self.model.get_output_shape(self.graph_name, output_name)
            output_dtype = self.model.get_output_dtype(self.graph_name, output_name)
            output_scale = self.model.get_output_scale(self.graph_name, output_name)
            output = sail.Tensor(self.handle, output_shape, output_dtype, True, True)
            self.output_tensors[output_name] = output
            self.output_scales[output_name] = output_scale

        # check batch size
        self.batch_size, self.c, self.height, self.width = self.model.get_input_shape(self.graph_name, self.input_name)
        suppoort_batch_size = [1, 2, 3, 4, 8, 16, 32, 64, 128, 256]
        if self.batch_size not in suppoort_batch_size:
            raise ValueError('batch_size must be {} for bmcv, but got {}'.format(suppoort_batch_size, self.batch_size))
        self.input_size = [self.width, self.height]

        # init preprocess
        self.use_resize_padding = True
        self.use_vpp = False
        self.ab = [x * self.input_scale / 1. for x in [1, 0, 1, 0, 1, 0]]

        # init postprocess

        self.tsize = yaml_dict["tsize"]
        self.name_classes_list = yaml_dict["name_classes"]
        self.num_classes = len(yaml_dict["name_classes"])
        self.conf_thre = yaml_dict["conf_thre"]
        self.nms_thre = yaml_dict["nms_thre"]
        self.is_time_record = yaml_dict["is_time_record"]

    def load_model(self, yaml_dict):
        print("----------------------------------------:", yaml_dict)

        model_file = f"{os.path.dirname(__file__)}/{yaml_dict['model_path']}"

        assert os.path.isfile(model_file) is True, "The model_path must be provided !"
        print("Reading engine from file {}".format(model_file))
        return sail.Engine(model_file, 0, sail.IOMode.SYSIO)

    def pysail_preprocess(self, bmimg):
        """
            Args:
                bmimg (sail.BMImage): original image, (H,W,C)

            Returns:
                padded_img (sail.BMImage): (img_C,resize_img_H,resize_img_W)
        """

        bgr_planar_img = sail.BMImage(self.handle, bmimg.height(), bmimg.width(), sail.Format.FORMAT_BGR_PLANAR, sail.DATA_TYPE_EXT_1N_BYTE)
        self.bmcv.convert_format(bmimg, bgr_planar_img)
        resized_img_bgr, ratio, txy = self.resize_bmcv(bgr_planar_img)
        preprocessed_bmimg = sail.BMImage(self.handle, self.height, self.width, sail.Format.FORMAT_BGR_PLANAR, self.img_dtype)
        self.bmcv.convert_to(resized_img_bgr, preprocessed_bmimg, ((self.ab[0], self.ab[1]), (self.ab[2], self.ab[3]), (self.ab[4], self.ab[5])))

        return preprocessed_bmimg, ratio, txy

    def resize_bmcv(self, bmimg):
        img_w = bmimg.width()
        img_h = bmimg.height()
        if self.use_resize_padding:
            r_w = self.width / img_w
            r_h = self.height / img_h
            if r_h > r_w:
                tw = self.width
                th = int(r_w * img_h)
                tx1 = tx2 = 0
                ty1 = 0
                ty2 = int(self.height - th)
            else:
                tw = int(r_h * img_w)
                th = self.height
                tx1 = 0
                tx2 = int((self.width - tw))
                ty1 = ty2 = 0

            ratio = min(r_w, r_h)
            txy = (tx1, ty1)
            attr = sail.PaddingAtrr()
            attr.set_stx(tx1)
            attr.set_sty(ty1)
            attr.set_w(tw)
            attr.set_h(th)
            attr.set_r(114)
            attr.set_g(114)
            attr.set_b(114)

            preprocess_fn = self.bmcv.vpp_crop_and_resize_padding if self.use_vpp else self.bmcv.crop_and_resize_padding
            resized_img_bgr = preprocess_fn(bmimg, 0, 0, img_w, img_h, self.width, self.height, attr, resize_alg=sail.bmcv_resize_algorithm.BMCV_INTER_LINEAR)
        else:
            r_w = self.width / img_w
            r_h = self.height / img_h
            ratio = (r_w, r_h)
            txy = (0, 0)
            preprocess_fn = self.bmcv.vpp_resize if self.use_vpp else self.bmcv.resize
            resized_img_bgr = preprocess_fn(bmimg, self.width, self.height)
        return resized_img_bgr, ratio, txy

    def pysail_postprocess(self, predictions, tsize, p6=False):
        """
            Args:
                predictions (numpy.ndarray:float32): 目标检测头的输出, 例如：(1, 8400, 85)::[batch_size, 预选框个数, (预选框的数值,预选框的概率,类别1的概率,类别2的概率,...,类别n的概率)]
                tsize (list): 原图像resize的大小
                p6:

            Returns:
                predictions (numpy.ndarray:float32): 目标检测头的输出, 例如：(1, 8400, 85)::[batch_size, 预选框个数, (预选框的数值,预选框的概率,类别1的概率,类别2的概率,...,类别n的概率)]
        """
        grids = []
        expanded_strides = []
        strides = [8, 16, 32] if not p6 else [8, 16, 32, 64]

        hsizes = [tsize[0] // stride for stride in strides]
        wsizes = [tsize[1] // stride for stride in strides]

        for hsize, wsize, stride in zip(hsizes, wsizes, strides):
            xv, yv = np.meshgrid(np.arange(wsize), np.arange(hsize))
            grid = np.stack((xv, yv), 2).reshape(1, -1, 2)
            grids.append(grid)
            shape = grid.shape[:2]
            expanded_strides.append(np.full((*shape, 1), stride))

        grids = np.concatenate(grids, 1)
        expanded_strides = np.concatenate(expanded_strides, 1)

        predictions[..., :2] = (predictions[..., :2] + grids) * expanded_strides
        predictions[..., 2:4] = np.exp(predictions[..., 2:4]) * expanded_strides

        return predictions

    def non_maximum_suppression(self, detections, conf_thre, nms_thre, class_agnostic=False):
        """
            Args:
                detections (numpy.ndarray:float32): 目标检测头的输出，例如：(8400, 85)::[目标框个数, (目标框的坐标信息,目标框的概率,类别1的概率,类别2的概率,...,类别n的概率)]
                conf_thre (float): 置信度阈值
                nms_thre (float): 非极大抑制阈值
                class_agnostic (bool): 是否进行类无关nms

            Returns:
                dets (numpy.ndarray:float32): nms后的目标信息，例如：(20, 6)::[目标框个数, (目标框的坐标信息,目标框类别ID,该类别的置信度)]
        """
        boxes = detections[:, :4]
        scores = detections[:, 4:5] * detections[:, 5:]

        boxes_xyxy = np.ones_like(boxes)
        boxes_xyxy[:, 0] = boxes[:, 0] - boxes[:, 2] / 2.  # boxes_x_min
        boxes_xyxy[:, 1] = boxes[:, 1] - boxes[:, 3] / 2.  # boxes_y_min
        boxes_xyxy[:, 2] = boxes[:, 0] + boxes[:, 2] / 2.  # boxes_x_max
        boxes_xyxy[:, 3] = boxes[:, 1] + boxes[:, 3] / 2.  # boxes_y_max

        dets = multiclass_nms(bmcv=self.bmcv, boxes=boxes_xyxy, scores=scores, nms_thr=nms_thre, score_thr=conf_thre, class_agnostic=class_agnostic)

        return dets

    def infer(self, image):
        """
            Args:
                image (sophon.sail.bm_image):

            Returns:
                NMS_outputsdets (list) --> (numpy.ndarray:float32): nms后的目标信息，例如：(20, 6)::[num_bboxes, (x1,y1,x2,y2,id_class,conf_class)]
                timestamps (tuple): (5,)::[start, preprocess_end, infer_end, postprocess_end, NMS_end]
        """
        # ###步骤 2.1 对数据进行预处理###
        start = time.time()
        preprocessed_bmimg, ratio, txy = self.pysail_preprocess(bmimg=image)
        np_img = preprocessed_bmimg.asmat()
        preprocess_end = time.time()

        # ###步骤 2.2 推理###
        # 将 preprocessed_output 转为推理所需要的格式
        input_tensor = sail.Tensor(self.handle, self.input_shape, self.input_dtype, False, False)
        self.bmcv.bm_image_to_tensor(preprocessed_bmimg, input_tensor)
        np_input = input_tensor.asnumpy()
        input_tensors = {self.input_name: input_tensor}
        self.model.process(self.graph_name, input_tensors, self.input_shapes, self.output_tensors)  # 接口形式2：输入输出都是sail.Tensor格式
        pysail_output = self.output_tensors[self.output_names[0]].asnumpy()
        # 将 pysail_output 转为后处理所需要的格式
        infer_end = time.time()

        # ###步骤 2.3 对输出进行后处理###
        postprocessed_output = self.pysail_postprocess(predictions=pysail_output, tsize=self.tsize)
        postprocess_end = time.time()

        # ###步骤 2.4 对后处理结果进行NMS###
        NMS_outputs = self.non_maximum_suppression(detections=postprocessed_output[0], conf_thre=self.conf_thre, nms_thre=self.nms_thre, class_agnostic=True)
        if NMS_outputs is not None:
            NMS_outputs[:, 0:4] /= ratio
        NMS_end = time.time()

        timestamps = (start, preprocess_end, infer_end, postprocess_end, NMS_end)

        NMS_outputs = [] if NMS_outputs is None else NMS_outputs
        return NMS_outputs, timestamps
