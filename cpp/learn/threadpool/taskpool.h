#include<thread>
#include<mutex>
#include<condition_variable>
#include<list>
#include<vector>
#include<memory>
#include<iostream>

class Task{
    public:
        virtual void doTask(){
            std::cout<<"do a task!!! id is: "<<id<<std::endl;
            id++;
            std::cout<<"the threadID:"<<std::this_thread::get_id()<<std::endl;
        }

        virtual ~Task(){
            std::cout<<"the task deconstructed!!!"<<std::endl;
        }
    static int id;
};

class TaskPool final{
    public:
        TaskPool();
        ~TaskPool();
        TaskPool(const TaskPool &rhs)=delete;//禁止拷贝构造
        TaskPool &operator =(const TaskPool &rhs)=delete;//禁止赋值构造

        void init(int threadNum=5);//初始化线程池
        void stop();
        void  addTask(Task* task);
        void removeAllTasks();
    
    private:
        void threadFun();

        std::list<std::shared_ptr<Task>> m_taskList; //任务队列，存储各个新创建的的Task
        std::mutex m_mutexList; //互斥锁，用于保护这个m_tasklist
        std::condition_variable m_cv;//条件变量，用于线程同步
        bool m_bRunning;//状态变量，用于表示线程池运行状态
        std::vector<std::shared_ptr<std::thread>> m_threads;//存储线程对象
}; 