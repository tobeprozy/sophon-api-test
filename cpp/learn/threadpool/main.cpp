#include"taskpool.h"
#include<chrono>

int Task::id=0;
int main(){
    TaskPool threadPool;

    threadPool.init();
    Task* task=nullptr;
    

    for (size_t i = 0; i < 20; i++)
    {
        task=new Task();
        threadPool.addTask(task);
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    threadPool.stop();
    return 0;
}

//g++ -g main.cpp taskpool.cpp -o test -lpthread
