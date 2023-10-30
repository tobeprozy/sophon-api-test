import cv2
import numpy as np
import math

class FishEye:
    def __init__(self, img):
        self.mImg = img
        self.mDesImg = np.zeros_like(img)
        self.mFishMapImg = img  # Replace with your actual fish map image

    def ImgExpand2(self):
        if self.mImg.size == 0:
            return self

        circleCoord = (self.mImg.shape[1] // 2 + 2, self.mImg.shape[0] // 2 + 5)
        R = self.mImg.shape[1] // 2

        for i in range(self.mDesImg.shape[1]):
            for j in range(self.mDesImg.shape[0]):
                pointDst = self.Shpere2Fish(i, j)
                cols = int((pointDst[0] + 1) * self.mImg.shape[1] / 2)
                rows = int((pointDst[1] + 1) * self.mImg.shape[0] / 2)
                if 0 <= rows < self.mImg.shape[0] and 0 <= cols < self.mImg.shape[1]:
                    color_value = self.mImg[rows, cols]
                    self.mDesImg[j, i] = color_value

        return self

    def Shpere2Fish(self, x, y):
        normalCoord = ((x * 2.0 / self.mFishMapImg.shape[1]) - 1, (y * 2.0 / self.mFishMapImg.shape[0]) - 1)
        longitude = normalCoord[0] * math.pi
        latitude = normalCoord[1] * (math.pi / 2)

        coordP = (
            math.cos(latitude) * math.cos(longitude),
            math.cos(latitude) * math.sin(longitude),
            math.sin(latitude)
        )

        r = 2 * math.atan2(math.sqrt(coordP[0] ** 2 + coordP[2] ** 2), coordP[1]) / 3.83
        theta = math.atan2(coordP[2], coordP[0])

        fishCoord = (r * math.cos(theta), r * math.sin(theta))
        return fishCoord

# Assuming you have loaded an image and created a FishEye object
input_image = cv2.imread('example2.png')
fisheye = FishEye(input_image)
result_image = fisheye.ImgExpand2().mDesImg

target_size = (600, 300)
result_image = cv2.resize(result_image, target_size)


cv2.imwrite("unwarped.png",result_image)
# cv2.imshow('Original Image', input_image)
# cv2.imshow('Unwarped Image', result_image)
# cv2.waitKey(0)
# cv2.destroyAllWindows()
