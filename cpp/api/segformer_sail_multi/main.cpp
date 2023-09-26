//===----------------------------------------------------------------------===//
//
// Copyright (C) 2023 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-DEMO is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//
#include <iostream>
#include <fstream>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "json.hpp"
#include "opencv2/opencv.hpp"
#include "ff_decode.hpp"
#include "segformer.hpp"
#include <unordered_map>
#include <iostream>

using json = nlohmann::json;
using namespace std;
#define USE_OPENCV_DECODE 0

using namespace std;
void readFilesRecursive(const std::string &directory, std::vector<std::string> &files_vector);
std::string getRelativePath(const std::string &filepath, const std::string &basepath);
std::string replaceSuffix(const std::string &relative_path, const std::string &img_suffix, const std::string &seg_map_suffix);
std::string removePrefix(const std::string &path, const std::string &prefix);
void ReadImages(vector<string>& files_vector, vector<sail::BMImage>& all_imgs, vector<string>& batch_names, int dev_id, TimeStamp* ts, sail::Handle& handle);

std::atomic<bool> has_images_to_process{ true }; // Initialize to true since there are images at the beginning.


std::unordered_map<std::string, std::string> data = {
    {"data_root", "datasets/cityscapes_small"},
    {"img_dir", "leftImg8bit/val"},
    {"ann_dir", "gtFine/val"},
    {"img_suffix", "_leftImg8bit.png"},
    {"seg_map_suffix", "_gtFine_labelTrainIds.png"}};

int main(int argc, char *argv[])
{
    cout.setf(ios::fixed);
    cout << "===========================+++++++++++++++++++++++++++" << endl;
    cout << "hello,i am going to get params...." << endl;
    // get params
    const char *keys =
        "{bmodel | models/BM1684/segformer.b0.512x1024.city.160k_fp32_1b.bmodel | bmodel file path}"
        "{dev_id | 0 | TPU device id}"
        "{input | datasets/cityscapes_small | input path, images direction or video file path}"
        "{palette | cityscapes | Color palette used for segmentation map}"
        "{help | 0 | print help information.}";
    cv::CommandLineParser parser(argc, argv, keys);
    if (parser.get<bool>("help"))
    {
        parser.printMessage();
        return 0;
    }

    // parms parse
    string bmodel_file = parser.get<string>("bmodel");
    string input = parser.get<string>("input");
    string palette = parser.get<string>("palette");
    int dev_id = parser.get<int>("dev_id");

    // check params
    struct stat info;

    if (stat(bmodel_file.c_str(), &info) != 0)
    {
        cout << "Cannot find valid model file." << endl;
        exit(1);
    }
    if (stat(input.c_str(), &info) != 0)
    {
        cout << "Cannot find input path." << endl;
        exit(1);
    }

    // create handle
    auto handle = sail::Handle(dev_id);
    sail::Bmcv bmcv(handle);
    cout << "set device id: " << dev_id << endl;


    // create save path
    if (access("results", 0) != F_OK)
        mkdir("results", S_IRWXU);

    // test images
    if (info.st_mode & S_IFDIR)
    {
        // 设置 data
        data["data_root"] = input;
        // 构建 img_dir 和 ann_dir
        string img_dir = data["data_root"] + "/" + data["img_dir"];
        string ann_dir = data["data_root"] + "/" + data["ann_dir"];

        if (access("results/images", 0) != F_OK)
            mkdir("results/images", S_IRWXU);

        std::string res_dir = "../../datasets/result_cl";
        if (access(res_dir.c_str(), 0) != F_OK)
            mkdir(res_dir.c_str(), S_IRWXU);

        // get files
        std::vector<std::string> files_vector;
        readFilesRecursive(img_dir, files_vector);

        // 初始化batch参数
        vector<sail::BMImage> all_imgs;
        vector<string> batch_names;
        //预处理后的图片
        std::deque<sail::BMImage> pre_imgs;


        int cn = files_vector.size();
        int id = 0;
        json results_json;
        results_json["data_root"] = removePrefix(data["data_root"], "../../");
        results_json["img_dir"] = removePrefix(img_dir, "../../");
        results_json["ann_dir"] = removePrefix(ann_dir, "../../");


        int pre_img_id=0;
        int pro_img_id=0;
        int post_img_id=0;  
        
        // load bmodel 
        SegFormer segformer1(dev_id, bmodel_file);
        SegFormer segformer2(dev_id, bmodel_file);

        // profiling
        TimeStamp segformer_ts;
        TimeStamp *ts = &segformer_ts;
        segformer1.enableProfile(&segformer_ts);
        segformer2.enableProfile(&segformer_ts);
        

        // Create a thread to implement ReadImage
        std::thread readImageThread(ReadImages, std::ref(files_vector), std::ref(all_imgs), std::ref(batch_names), dev_id, ts, std::ref(handle));
        // Check if the thread has started
        if(readImageThread.joinable()){
            readImageThread.join();
        } else {
            std::cout << "Failed to start thread." << std::endl;
        }


        std::mutex mtx;
        std::condition_variable cv;
    
         // Create threads to initialize parameters
        std::thread initThread1([&]() {         
            CV_Assert(0 == segformer1.Init());
            segformer1.palette = palette;
            // get batch_size
            int batch_size1 = segformer1.batch_size();
            std::cout << "Thread 2: Initializing parameters" << std::endl;
            std::cout << "Thread 2: Thread ID is " << std::this_thread::get_id() << std::endl;

        });

        std::thread initThread2([&]() {         
            CV_Assert(0 == segformer2.Init());
            segformer2.palette = palette;
            // get batch_size
            int batch_size2 = segformer2.batch_size();
            std::cout << "Thread 3: Initializing parameters" << std::endl;
            std::cout << "Thread 3: Thread ID is " << std::this_thread::get_id() << std::endl;

        });

       initThread1.join();
       initThread2.join();
        
        // Create threads to pre-process images
        std::thread preProcessThread1([&]() {
            while (!all_imgs.empty()) {
                
                auto input_image = std::move(all_imgs.back());
                all_imgs.pop_back();
               
                sail::BMImage output;
                segformer1.pre_process(input_image,output);
                pre_imgs.push_back(std::move(output));  // Change push_front to push_back
                pre_img_id++;
                std::cout << "pre_imgs size: " << pre_imgs.size() << std::endl;
                

                std::cout << "Thread 4: Pre-processing images" << std::endl;
                std::cout << "Thread 4: Thread ID is " << std::this_thread::get_id() << std::endl;
                
                // 唤醒processThread1
                // Check if there are more images to process or not
                if (all_imgs.empty()) {
                    has_images_to_process.store(false);
                    cv.notify_all(); // Notify processThread1 in case it is waiting
                }
            }
         
        });

        std::thread preProcessThread2([&]() {
            while (!all_imgs.empty()) {
                
                auto input_image = std::move(all_imgs.back());
                all_imgs.pop_back();  
                sail::BMImage output;
                segformer2.pre_process(input_image,output);
                pre_imgs.push_back(std::move(output));  // Change push_front to push_back

                pre_img_id++;
                std::cout << "pre_imgs size: " << pre_imgs.size() << std::endl;
                
                
                std::cout << "Thread 5: Pre-processing images" << std::endl;
                std::cout << "Thread 5: Thread ID is " << std::this_thread::get_id() << std::endl;
                
                // Check if there are more images to process or not
                if (all_imgs.empty()) {
                    has_images_to_process.store(false);
                    cv.notify_all(); // Notify processThread1 in case it is waiting
                }
            }
         
        });


        int img_num=files_vector.size();
        // Create threads to infer from preprocessed images
        std::thread processThread1([&]() {
           while (true) {
                // Wait for images only if there are images to process
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&]() { return !has_images_to_process || !pre_imgs.empty(); });

                if (!has_images_to_process && pre_imgs.empty()) {
                    break;
                }

                auto convert_img = std::move(pre_imgs.front());
                pre_imgs.pop_front();
                lock.unlock();

                segformer1.SendInputTensor(convert_img);
                std::cout << "convert_img.height(): " << convert_img.height() << std::endl;
                std::cout << "convert_img.width(): " << convert_img.width() << std::endl;
                std::cout << "pre_imgs size: " << pre_imgs.size() << std::endl;

                segformer1.engine->process(segformer1.graph_names[0], segformer1.input_tensors, segformer1.output_tensors);
                std::cout << "Thread 6: process done" << std::endl;
                cv::Mat color_seg_mat;
                segformer1.post_process(color_seg_mat);

                lock.lock();
                img_num--;
                int post_img_id = img_num; // Assuming post_img_id is declared somewhere to keep track of image count
                lock.unlock();

                cv::imwrite("results/images/image_" + std::to_string(post_img_id) + ".png", color_seg_mat);
                std::cout << "Thread 6: Thread ID is " << std::this_thread::get_id() << std::endl;

                // If there are no more images to process, break the loop and exit the thread
                if (!has_images_to_process && pre_imgs.empty()) {
                    break;
                }
            }
        });

        std::thread processThread2([&]() {
           while (true) {
                // Wait for images only if there are images to process
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&]() { return !has_images_to_process || !pre_imgs.empty(); });

                if (!has_images_to_process && pre_imgs.empty()) {
                    break;
                }

                auto convert_img = std::move(pre_imgs.front());
                pre_imgs.pop_front();
                lock.unlock();

                segformer2.SendInputTensor(convert_img);
                std::cout << "convert_img.height(): " << convert_img.height() << std::endl;
                std::cout << "convert_img.width(): " << convert_img.width() << std::endl;
                std::cout << "pre_imgs size: " << pre_imgs.size() << std::endl;

                segformer2.engine->process(segformer2.graph_names[0], segformer2.input_tensors, segformer2.output_tensors);
                std::cout << "Thread 7: process done" << std::endl;
                cv::Mat color_seg_mat;
                segformer2.post_process(color_seg_mat);

                lock.lock();
                img_num--;
                int post_img_id = img_num; // Assuming post_img_id is declared somewhere to keep track of image count
                lock.unlock();

                cv::imwrite("results/images/image_" + std::to_string(post_img_id) + ".png", color_seg_mat);
                std::cout << "Thread 7: Thread ID is " << std::this_thread::get_id() << std::endl;

                // If there are no more images to process, break the loop and exit the thread
                if (!has_images_to_process && pre_imgs.empty()) {
                    break;
                }
            }
        });


        //Join threads
        preProcessThread1.join();
        preProcessThread2.join();
        processThread1.join();
        processThread2.join();

    }

    // Wait for all threads to exit
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}



void ReadImages(vector<string>& files_vector, vector<sail::BMImage>& all_imgs, vector<string>& batch_names, int dev_id, TimeStamp* ts, sail::Handle& handle) {
            int cn = files_vector.size();
            int id = 0;
            for (vector<string>::iterator iter = files_vector.begin(); iter != files_vector.end(); iter++)
            {

                string img_file = *iter;
                cout << id << "/" << cn << ", img_file: " << img_file << endl;
                sail::BMImage bmimg;
                ts->save("read image");
                sail::Decoder decoder((const string)img_file, true, dev_id);
                ts->save("read image");
                int ret = decoder.read(handle, bmimg);
                if (ret != 0)
                {
                    cout << "read failed" << endl;
                }

                size_t index = img_file.rfind("/");
                string img_name = img_file.substr(index + 1);
                all_imgs.push_back(std::move(bmimg));
                batch_names.push_back(img_name);

                std::cout << "Thread 1: ReadImages ..." << std::endl;
                cout<<"all_imgs.size(): "<<all_imgs.size()<<endl;
                cout<<"batch_names.size(): "<<batch_names.size()<<endl;
                id++;
        }
}


std::string removePrefix(const std::string &path, const std::string &prefix)
{
    if (path.compare(0, prefix.length(), prefix) == 0)
    {
        return path.substr(prefix.length());
    }
    else
    {
        return path;
    }
}

void readFilesRecursive(const std::string &directory, std::vector<std::string> &files_vector)
{
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;

    dir = opendir(directory.c_str());
    if (dir == nullptr)
    {
        std::cerr << "Failed to open directory: " << directory << std::endl;
        return;
    }

    while ((entry = readdir(dir)) != nullptr)
    {
        std::string entryName = entry->d_name;
        std::string fullPath = directory + "/" + entryName;

        if (entryName != "." && entryName != "..")
        {
            if (stat(fullPath.c_str(), &fileStat) == 0)
            {
                if (S_ISDIR(fileStat.st_mode))
                {
                    // 如果是子目录，递归读取
                    readFilesRecursive(fullPath, files_vector);
                }
                else
                {
                    // 如果是文件，检查文件扩展名为图片格式
                    std::string extension = entryName.substr(entryName.find_last_of(".") + 1);
                    if (extension == "jpg" || extension == "png" || extension == "bmp")
                    {
                        files_vector.push_back(fullPath);
                    }
                }
            }
            else
            {
                std::cerr << "Failed to get file stat for: " << fullPath << std::endl;
            }
        }
    }

    closedir(dir);
}

std::string getRelativePath(const std::string &filepath, const std::string &basepath)
{
    size_t base_len = basepath.length();

    // 检查 basepath 结尾是否有斜杠
    if (base_len > 0 && (basepath[base_len - 1] == '/' || basepath[base_len - 1] == '\\'))
    {
        base_len--;
    }

    // 检查 filepath 是否以 basepath 开头
    if (filepath.compare(0, base_len, basepath) == 0)
    {
        // 返回 filepath 中去掉 basepath 部分的相对路径，并删除开头的斜杠
        return filepath.substr(base_len + 1);
    }
    else
    {
        // basepath 不是 filepath 的前缀，返回原始 filepath
        return filepath;
    }
}

std::string replaceSuffix(const std::string &relative_path, const std::string &img_suffix, const std::string &seg_map_suffix)
{
    std::string replaced_path = relative_path;

    // 找到最后一个出现的 img_suffix
    size_t suffix_pos = replaced_path.rfind(img_suffix);
    if (suffix_pos != std::string::npos)
    {
        // 替换 img_suffix 为 seg_map_suffix
        replaced_path.replace(suffix_pos, img_suffix.length(), seg_map_suffix);
    }

    return replaced_path;
}