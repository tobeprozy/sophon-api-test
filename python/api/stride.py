import cv2
import numpy as np

# 创建一个简单的图像（假设是 32 位对齐）
height, width = 100, 200
image = np.zeros((height, width, 3), dtype=np.uint8)

# 保存没有 64 位对齐的图像
cv2.imwrite('unaligned_image.png', image)

# 获取图像数据
image_data = image.tobytes()

# 计算 64 位对齐所需的填充字节数
alignment = 64  # 假设 64 位对齐
bytes_per_row = width * 3  # 每行的字节数（假设 3 通道图像）
padding = (alignment - (bytes_per_row % alignment)) % alignment

# 计算填充后的图像宽度
padded_width = width + padding

# 创建填充后的图像数据
padded_data = b'\x00' * (padding * height) + image_data

# 创建对齐后的图像
aligned_image = np.frombuffer(padded_data, dtype=np.uint8).reshape(height, padded_width, 3)

# 保存对齐后的图像
cv2.imwrite('aligned_image.png', aligned_image)

print("已创建并保存对齐后的图像。")
