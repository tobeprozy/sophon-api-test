import numpy as np

class Corners:
    def __init__(self):
        self.left_top = Point()
        self.left_bottom = Point()
        self.right_top = Point()
        self.right_bottom = Point()

class Point:
    def __init__(self):
        self.x = 0
        self.y = 0

def calc_corners(H, src):
    corners = Corners()

    v2 = np.array([0, 0, 1], dtype=np.float64)  # Left top
    v1 = np.empty((3,), dtype=np.float64)  # Transformed coordinates
    V2 = np.reshape(v2, (3, 1))  # Column vector

    V1 = np.dot(H, V2)
    v1[0] = V1[0, 0]
    v1[1] = V1[1, 0]
    v1[2] = V1[2, 0]
    corners.left_top.x = v1[0] / v1[2]
    corners.left_top.y = v1[1] / v1[2]

    v2[0] = 0
    v2[1] = src.shape[0]
    v2[2] = 1
    V2 = np.reshape(v2, (3, 1))
    V1 = np.dot(H, V2)
    v1[0] = V1[0, 0]
    v1[1] = V1[1, 0]
    v1[2] = V1[2, 0]
    corners.left_bottom.x = v1[0] / v1[2]
    corners.left_bottom.y = v1[1] / v1[2]

    v2[0] = src.shape[1]
    v2[1] = 0
    v2[2] = 1
    V2 = np.reshape(v2, (3, 1))
    V1 = np.dot(H, V2)
    v1[0] = V1[0, 0]
    v1[1] = V1[1, 0]
    v1[2] = V1[2, 0]
    corners.right_top.x = v1[0] / v1[2]
    corners.right_top.y = v1[1] / v1[2]

    v2[0] = src.shape[1]
    v2[1] = src.shape[0]
    v2[2] = 1
    V2 = np.reshape(v2, (3, 1))
    V1 = np.dot(H, V2)
    v1[0] = V1[0, 0]
    v1[1] = V1[1, 0]
    v1[2] = V1[2, 0]
    corners.right_bottom.x = v1[0] / v1[2]
    corners.right_bottom.y = v1[1] / v1[2]

    return corners
