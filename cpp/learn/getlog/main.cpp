#include <iostream>
#include <cstdio>
#include <memory>
#include <array>
		
std::string executeCommand(const std::string& cmd) {
    char buffer[128];
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        std::cout << "popen() failed!!!" << std::endl;
        return "";
    }
    while (!feof(pipe.get())) {
        if (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
            result += buffer;
        }
    }
    return result;
}

   
int main() {
   
    std::string output=executeCommand("ifconfig");
    std::cout<<"ipconfig output:"<<std::endl;
    std::cout<<output<<std::endl;

    return 0;
}
