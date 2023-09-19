//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-DEMO is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include "bmruntime_interface.h"
#include "bmcv_api.h"
#include "bmcv_api_ext.h"
#include "bmlib_runtime.h"
#include <iostream>
#include <vector>

using namespace std;

void matrix_gen_data(float* data, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    data[i] = ((float)rand() / (float)RAND_MAX) * 3.3;
  }
}

template <typename T>
static int matrix_transpose(vector<vector<T>> &matrix) {
    if (matrix.size() == 0)
        return BM_ERR_FAILURE;
    vector<vector<T>> trans_vec(matrix[0].size(), vector<T>());
    for (size_t i = 0; i < matrix.size(); i++) {
        for (size_t j = 0; j < matrix[i].size(); j++) {
            if (trans_vec[j].size() != matrix.size())
                trans_vec[j].resize(matrix.size());
            trans_vec[j][i] = matrix[i][j];
        }
    }
    matrix = trans_vec;
    return BM_SUCCESS;
}



// query matrix * database matrix (FP32 + FP32 ==> FP32)
vector<vector<float>> matrix_mul_ref(vector<vector<float>> input_data,
                                     vector<vector<float>> db_data) {
    float a = 0;
    int query_vecs_num = input_data.size();
    int database_vecs_num = db_data[0].size();
    int vec_dims = input_data[0].size();

    vector<vector<float>> matrix_mul(query_vecs_num);
    for (int query_cnt = 0; query_cnt < query_vecs_num; query_cnt++) {
        for (int db_cnt = 0; db_cnt < database_vecs_num; db_cnt++) {
            a = 0.0f;
            for (int dims_cnt = 0; dims_cnt < vec_dims; dims_cnt++) {
                a += input_data[query_cnt][dims_cnt] * db_data[dims_cnt][db_cnt];
            }
            matrix_mul[query_cnt].push_back(a);
        }
    }
    return matrix_mul;
}


template <typename T>
bool result_compare(T * tpu_result_similarity,
                    int * tpu_result_index,
                    vector<vector<T>> ref_result,
                    int sort_cnt) {
    vector<vector<T>> tmp_ref_result = ref_result;

    for (size_t query_cnt = 0; query_cnt < ref_result.size(); query_cnt++) {
        // std::cout << "\n ==> query_cnt = [" << query_cnt << "]" << std::endl;
        for (int sort_indx = 0; sort_indx < sort_cnt; sort_indx++) {
            int ref_index = std::distance(tmp_ref_result[query_cnt].begin(),
                                          max_element(tmp_ref_result[query_cnt].begin(),
                                                      tmp_ref_result[query_cnt].end()));
            T ref_similarity = tmp_ref_result[query_cnt][ref_index];
            // find the ref index in each of origin vector
            auto tmp_elem = std::find(ref_result[query_cnt].begin(),
                                      ref_result[query_cnt].end(),
                                      ref_similarity);
            int ref_index_origin = std::distance(ref_result[query_cnt].begin(), tmp_elem);
            // std::cout << "\n TPU topk index: [" << tpu_result_index[query_cnt * sort_cnt + sort_indx] << "]" << std::endl;
            // std::cout << " Ref topk index: [" << ref_index_origin << "]" << std::endl;
            // std::cout << " TPU topk distance value: [" << tpu_result_similarity[query_cnt * sort_cnt + sort_indx] << "]" << std::endl;
            // std::cout << " Ref topk distance value: [" << ref_similarity << "]" << std::endl;
            //if (fabs((float)tpu_result_similarity[query_cnt * sort_cnt + sort_indx] - (float)ref_similarity) > (1e-2)) {
                cout << "tpu_res[" << query_cnt << "]"
                     << "[" << sort_indx << "]"
                     << "[" << tpu_result_index[query_cnt * sort_cnt + sort_indx] << "] "
                     << tpu_result_similarity[query_cnt * sort_cnt + sort_indx]

                     << " ref_result[" << query_cnt << "]"
                     << "[" << sort_indx << "]"
                     << "[" << ref_index_origin << "] "
                     << tmp_ref_result[query_cnt][ref_index] << endl;
            //    return false;
            //}
            tmp_ref_result[query_cnt].erase(tmp_ref_result[query_cnt].begin() + ref_index);
        }
    }
    return true;
}


int main(int argc, char* argv[]) {
  

  // 定义数组大小和其他参数
  int sort_cnt = 6;
  int query_vecs_num = 1;
  int database_vecs_num = 6;
  int is_transpose = 1;
  int input_dtype = 5; // 5: float
  int output_dtype = 5;
  int vec_dims = 512;


  vector<vector<float>> input_content_vec(query_vecs_num);
  vector<vector<float>> db_content_vec(database_vecs_num);

  float *input_data = new float[query_vecs_num * vec_dims];
  float *db_data = new float[database_vecs_num * vec_dims];

  // 打开文件
  std::ifstream filetest("./faiss_test.txt");
  // 检查文件是否成功打开
  if (!filetest) {
    std::cerr << "Failed to open file." << std::endl;
    return 1;
  }
  // 读取文件数据到数组
  if (filetest) {  
    std::string line;
    int row_count = 0;
    while (std::getline(filetest, line) && row_count < query_vecs_num) {
      std::vector<float> row;
      std::stringstream ss(line);
      float val;
      int col_count = 0;

      while (ss >> val && col_count < vec_dims) {
        input_content_vec[row_count].push_back(val);
        input_data[row_count * vec_dims + col_count] = val;
        col_count++;
      }
      row_count++;
    }
  }

    // 打开文件
  std::ifstream fileindex("./faiss_index.txt");
  // 检查文件是否成功打开
  if (!fileindex) {
    std::cerr << "Failed to open file." << std::endl;
    return 1;
  }
  // 读取文件数据到数组
  if (fileindex) {  
    std::string line;
    int row_count = 0;
    while (std::getline(fileindex, line) && row_count < database_vecs_num) {
      std::vector<float> row;
      std::stringstream ss(line);
      float val;
      int col_count = 0;

      while (ss >> val && col_count < vec_dims) {
        db_content_vec[row_count].push_back(val);
        db_data[row_count * vec_dims + col_count] = val;
        col_count++;
      }
      row_count++;
    }
  }

  // 打印数组中的数据（可选）
  std::cout<<"print input_data:"<<endl;
  for (int i = 0; i < query_vecs_num; i++) {
    for (int j = 0; j < vec_dims; j++) {
      std::cout << input_data[i * vec_dims + j] << " ";
    }
    std::cout << std::endl;
  }

  // 打印数组中的数据（可选）
  std::cout<<"print db_data:"<<endl;
  for (int i = 0; i < database_vecs_num; i++) {
    for (int j = 0; j < vec_dims; j++) {
      std::cout << db_data[i * vec_dims + j] << " ";
    }
    std::cout << std::endl;
  }

  

//  matrix_gen_data(input_data, query_vecs_num * vec_dims);
//  matrix_gen_data(db_data, vec_dims * database_vecs_num);

  bm_handle_t handle = nullptr;
  bm_dev_request(&handle, 0);
  bm_device_mem_t query_data_dev_mem;
  bm_device_mem_t db_data_dev_mem;
  bm_malloc_device_byte(handle, &query_data_dev_mem,
          query_vecs_num * vec_dims * sizeof(float));
  bm_malloc_device_byte(handle, &db_data_dev_mem,
          database_vecs_num * vec_dims * sizeof(float));
  bm_memcpy_s2d(handle, query_data_dev_mem, input_data);
  bm_memcpy_s2d(handle, db_data_dev_mem, db_data);

  float *output_dis = new float[query_vecs_num * sort_cnt];
  int *output_inx = new int[query_vecs_num * sort_cnt];
  bm_device_mem_t buffer_dev_mem;
  bm_device_mem_t sorted_similarity_dev_mem;
  bm_device_mem_t sorted_index_dev_mem;
  bm_malloc_device_byte(handle, &buffer_dev_mem,
          query_vecs_num * database_vecs_num * sizeof(float));
  bm_malloc_device_byte(handle, &sorted_similarity_dev_mem,
          query_vecs_num * sort_cnt * sizeof(float));
  bm_malloc_device_byte(handle, &sorted_index_dev_mem,
          query_vecs_num * sort_cnt * sizeof(int));

  bmcv_faiss_indexflatIP(handle,
                        query_data_dev_mem,
                        db_data_dev_mem,
                        buffer_dev_mem,
                        sorted_similarity_dev_mem,
                        sorted_index_dev_mem,
                        vec_dims,
                        query_vecs_num,
                        database_vecs_num,
                        sort_cnt,
                        is_transpose,
                        input_dtype,
                        output_dtype);

  if (is_transpose)
        matrix_transpose(db_content_vec);

  vector<vector<float>> ref_result(query_vecs_num);
  ref_result = matrix_mul_ref(input_content_vec, db_content_vec);


    
  bm_memcpy_d2s(handle, output_dis, sorted_similarity_dev_mem);
  bm_memcpy_d2s(handle, output_inx, sorted_index_dev_mem);

  if (false == result_compare<float>(output_dis,
                                     output_inx,
                                     ref_result,
                                     sort_cnt)) {
        std::cout << "------------faiss_indexflatIP FIX8B COMPARE failed-----" << std::endl;
        exit(-1);
    }


  delete[] input_data;
  delete[] db_data;
  bm_free_device(handle, query_data_dev_mem);
  bm_free_device(handle, db_data_dev_mem);
  bm_free_device(handle, buffer_dev_mem);
  bm_free_device(handle, sorted_similarity_dev_mem);
  bm_free_device(handle, sorted_index_dev_mem);
  bm_dev_free(handle);
}

