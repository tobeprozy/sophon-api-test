import cv2
import numpy as np
from Corner import *

def optimize_seam(img1, trans, dst, corners):
    start = min(int(corners.left_top.x), int(corners.left_bottom.x))  # Start position, left boundary of the overlapping area
    process_width = img1.shape[1] - start  # Width of the overlapping area
    rows = min(dst.shape[0],img1.shape[0])
    cols = img1.shape[1]  # Note: number of columns * number of channels
    alpha = 1  # Weight of pixels from img1

    for i in range(rows):
        for j in range(start, cols):
            if j + 2 >= cols:
                break
            if trans[i][j] == 0 and trans[i][j + 1] == 0 and trans[i][j + 2] == 0:
                alpha = 1
            else:
                alpha = (process_width - (j - start)) / process_width
            # import pdb;pdb.set_trace()
            dst[i][j] = int(img1[i][j] * alpha + trans[i][j] * (1 - alpha))
            dst[i][j + 1] = int(img1[i][j + 1] * alpha + trans[i][j + 1] * (1 - alpha))
            dst[i][j + 2] = int(img1[i][j + 2] * alpha + trans[i][j + 2] * (1 - alpha))
    # import pdb;pdb.set_trace()
    return dst

# 读取图像
image2 = cv2.imread('Left.png', cv2.IMREAD_GRAYSCALE)
image1 = cv2.imread('Right.png', cv2.IMREAD_GRAYSCALE)

# 创建SURF特征检测器
detector = cv2.xfeatures2d.SURF_create(2000)

# 检测特征点
keyPoint1 = detector.detect(image1)
keyPoint2 = detector.detect(image2)

# 计算特征点描述符
descriptor = cv2.xfeatures2d.SURF_create()
keypoints1, imageDesc1 = descriptor.compute(image1, keyPoint1)
keypoints2, imageDesc2 = descriptor.compute(image2, keyPoint2)

# 创建FLANN匹配器
matcher = cv2.FlannBasedMatcher()

# 添加训练数据并训练匹配器
matcher.add([imageDesc1])
matcher.train()

# 使用knnMatch进行特征点匹配
matchePoints = matcher.knnMatch(imageDesc2, 2)

# 使用Lowe's算法获取优秀匹配点
GoodMatchePoints = []
for m, n in matchePoints:
    if m.distance < 0.4 * n.distance:
        GoodMatchePoints.append(m)

# 绘制优秀匹配点
first_match = cv2.drawMatches(image2, keypoints2, image1, keypoints1, GoodMatchePoints, None)
cv2.imwrite("first_match.jpg", first_match)

imagePoints1 = []
imagePoints2 = []

for match in GoodMatchePoints:
    imagePoints2.append(keypoints2[match.queryIdx].pt)
    imagePoints1.append(keypoints1[match.trainIdx].pt)

# 获取图像1到图像2的投影映射矩阵
homo, _ = cv2.findHomography(np.array(imagePoints1), np.array(imagePoints2), cv2.RANSAC)
print("变换矩阵为：\n", homo, "\n")

corners = calc_corners(homo, image1)
# 图像配准
imageTransform1 = cv2.warpPerspective(image1, homo, (max(int(corners.right_top.x), int(corners.right_bottom.x)), image2.shape[0]))

# cv2.imshow("直接经过透视矩阵变换", imageTransform1)
# cv2.imwrite("trans1.jpg", imageTransform1)

dst_width = imageTransform1.shape[1]  # Taking the width of the rightmost point as the width of the stitched image
dst_height = image2.shape[0]

dst = np.zeros((dst_height, dst_width), dtype=np.uint8)

dst[0:imageTransform1.shape[0], 0:imageTransform1.shape[1]] = imageTransform1
dst[0:image2.shape[0], 0:image2.shape[1]] = image2

# cv2.imshow("b_dst", dst)
# cv2.waitKey(0)
# cv2.destroyAllWindows()

# Assuming img1, trans, dst, and corners are already defined
res = optimize_seam(image2, imageTransform1, dst, corners)
print(np.sum(res - dst))
cv2.imshow("Processed.png", res)
cv2.waitKey(0)
cv2.destroyAllWindows()