import numpy as np


# NMS工具
def multiclass_nms(bmcv, boxes, scores, nms_thr, score_thr, class_agnostic):
    if class_agnostic:
        return multiclass_nms_class_agnostic(bmcv, boxes, scores, nms_thr, score_thr)
    else:
        return multiclass_nms_class_aware(bmcv, boxes, scores, nms_thr, score_thr)


# 将所有类别的检测框合并在一起进行NMS处理
def multiclass_nms_class_agnostic(bmcv, boxes, scores, nms_thr, score_thr):
    """
        Args:
            boxes (numpy.ndarray:float32): 目标框的坐标信息, 例如：(8400, 4)::[目标框个数, 目标框的坐标信息]
            scores (numpy.ndarray:float32): 各个类别的置信度, 例如：(8400, 80)::[目标框个数, 各个类别的置信度]
            score_thr (list) --> (float): 各个类别的置信度阈值, 例如：[0.4, 0.5, ..., 0.2]
            nms_thr (float): 非极大抑制阈值

        Returns:
            dets  (numpy.ndarray:float32): 目标框信息, 例如：(20, 6)::[目标框个数, (目标框的坐标信息,目标框类别ID,该类别的置信度)]
    """
    cls_inds = scores.argmax(1)
    cls_scores = scores[np.arange(len(cls_inds)), cls_inds]
    valid_score_mask = cls_scores > score_thr[0]
    if valid_score_mask.sum() == 0:
        return None
    valid_scores = cls_scores[valid_score_mask]
    valid_boxes = boxes[valid_score_mask]
    valid_cls_inds = cls_inds[valid_score_mask]
    input_nms = np.concatenate([valid_boxes, np.expand_dims(valid_scores, 1)], 1)
    output = bmcv.nms(input_nms, nms_thr)
    matches = (input_nms[:, None] == output).all(-1)  # 选出入选目标框的类别ID
    indices = np.argmax(matches, axis=0)  # 找到每个output行在input_nms中的索引
    matched_categories = valid_cls_inds[indices]  # 获取匹配的类别
    dets = np.concatenate([output[:, :4], output[:, 4:5], np.expand_dims(matched_categories, 1)], 1)  # 拼接成一个ndarray形式

    return dets


# 对每个类别分别进行NMS处理，并保留每个类别的有效检测框
def multiclass_nms_class_aware(bmcv, boxes, scores, nms_thr, score_thr):
    """
        Args:
            boxes (numpy.ndarray:float32): 目标框的坐标信息, 例如：(8400, 4)::[目标框个数, 目标框的坐标信息]
            scores (numpy.ndarray:float32): 各个类别的置信度, 例如：(8400, 80)::[目标框个数, 各个类别的置信度]
            score_thr (list) --> (float): 各个类别的置信度阈值, 例如：[0.4, 0.5, ..., 0.2]
            nms_thr (float): 非极大抑制阈值

        Returns:
            result  (numpy.ndarray:float32): 目标框信息, 例如：(20, 6)::[目标框个数, (目标框的坐标信息,目标框类别ID,该类别的置信度)]
    """
    num_classes = scores.shape[1]
    result = []
    for cls_ind in range(num_classes):
        cls_scores = scores[:, cls_ind]
        cls_score_thr = score_thr[cls_ind]
        valid_score_mask = cls_scores > cls_score_thr
        if valid_score_mask.sum() == 0:
            continue
        else:
            valid_scores = cls_scores[valid_score_mask]
            valid_boxes = boxes[valid_score_mask]
            input_nms = np.concatenate([valid_boxes, np.expand_dims(valid_scores, 1)], 1)
            output = bmcv.nms(input_nms, nms_thr)
            dets = np.concatenate([output[:, :4], np.ones_like(a=output[:, 4:5], dtype=output[:, 4:5].dtype) * cls_ind, output[:, 4:5]], 1)
            result.append(dets)
    return np.concatenate(result, 0)
