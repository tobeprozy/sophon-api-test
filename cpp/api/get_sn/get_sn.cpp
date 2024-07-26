#include <iostream>
#include "bmruntime_interface.h"
#include "bmlib_runtime.h"

int main() {
    bm_handle_t m_handle;
    bm_status_t status;
    int dev_id = 0; // 假设设备ID为0，可以根据实际情况修改

    // 初始化设备句柄
    status = bm_dev_request(&m_handle, dev_id);
    if (status != BM_SUCCESS) {
        std::cerr << "Failed to initialize device handle, error code: " << status << std::endl;
        return -1;
    }

    // 分配内存来存储序列号
    char device_sn[100];

    // 调用 bm_get_sn 函数
    status = bm_get_sn(m_handle, device_sn);
    if (status == BM_SUCCESS) {
        printf("Device SN: %s\n", device_sn);
    } else {
        printf("Failed to get device SN, error code: %d\n", status);
    }

    // 释放设备句柄
    bm_dev_free(m_handle);

    return 0;
}
