
#define USE_FFMPEG 1
#define USE_BMCV 1
#define USE_OPENCV 1

#include <sail/cvwrapper.h>
#include <stdio.h>

#include <iostream>
#include <string>

using namespace std;

void get_frame(int dev_id, string input_video) {
  sail::Decoder decoder(input_video, true, dev_id);
  if (!decoder.is_opened()) {
    printf("video[%s] read error", input_video.c_str());
    exit(1);
  }
  sail::Handle handle(dev_id);
  sail::Bmcv bmcv(handle);
  while (true) {
    sail::BMImage img = decoder.read(handle);
    bmcv.imwrite("test.jpg", img);
  }
}

void get_batch_frame(int dev_id, string input_video) {
  sail::Decoder decoder(input_video, true, dev_id);
  if (!decoder.is_opened()) {
    printf("video[%s] read error", input_video.c_str());
    exit(1);
  }
  sail::Handle handle(dev_id);
  sail::Bmcv bmcv(handle);
  sail::BMImageArray<4> imgs;

  while (true) {
    for (size_t i = 0; i < 4; i++) {
      decoder.read_(handle, imgs[i]);
    }
    bmcv.imwrite_("batch_0.jpg", imgs[0]);
    bmcv.imwrite_("batch_1.jpg", imgs[1]);
    bmcv.imwrite_("batch_2.jpg", imgs[2]);
    bmcv.imwrite_("batch_3.jpg", imgs[3]);
  }
}

int main(int argc, char* argv[]) {
  string input_video = "../../../../datasets/videos/dance_1080P.mp4";
  int dev_id = 0;

  for (int i = 1; i < argc; ++i) {
    string arg = argv[i];
    if (arg == "--input" && i + 1 < argc) {
      input_video = argv[++i];
    } else if (arg == "--dev_id" && i + 1 < argc) {
      dev_id = atoi(argv[++i]);
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "--input [input_video] --dev_id [dev_id]" << std::endl;
      return 0;
    } else {
      std::cout << "unknown arg: " << arg << std::endl;
      return 1;
    }
  }
//   get_frame(dev_id, input_video);make
  
  get_batch_frame(dev_id, input_video);
  return 0;
}