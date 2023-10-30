#include"taskpool.h"
#include <functional> //bind

TaskPool::TaskPool():m_bRunning(false){}

TaskPool::~TaskPool(){removeAllTasks();}

void TaskPool::init(int threadNum ){
    if(threadNum<=0) threadNum=5;
    m_bRunning=true;//线程池一旦初始化后，标记状态
    for (size_t i = 0; i < threadNum; i++)
    {
        std::shared_ptr<std::thread> spThread;//创建一个线程指针
        spThread.reset(new std::thread(std::bind(&TaskPool::threadFun,this)));//让这个线程指针绑定某个函数
        m_threads.push_back(spThread);//把这个线程指针，放入线程容器
    }
    
}

void TaskPool::threadFun(){
    std::shared_ptr<Task> spTask;
    while(true){//持续从任务队列里取Task，然后执行，执行完后马上析构。当m_bRunning关闭的时候，才跳出
        {
            std::unique_lock<std::mutex> guard(m_mutexList);
            while(m_taskList.empty()){//任务队列为空
                if(!m_bRunning){
                    break;//没有运行
                }
                //如果获得了互斥锁，但是条件不满足,m_cv调用将释放锁，挂起当前线程，不会往下执行
                //当发生变化后，条件满足时，m_cv又会唤起挂起的线程且获得锁
                m_cv.wait(guard);
            }
            if(!m_bRunning) break;

            //如果都满足，那么就从任务队列取任务，并且从任务队列中弹出这个任务
            spTask=m_taskList.front();
            m_taskList.pop_front();
        }

        if(spTask==nullptr) continue;//如果获取的这个任务为空，那就不执行了
        {
        std::unique_lock<std::mutex> guard(m_mutexList);//保护里面的静态变量
        spTask->doTask();//否则就执行这个任务
        }
        spTask.reset();
    }
    std::cout<<"exit thread,threadID:"<<std::this_thread::get_id()<<std::endl;
}


void TaskPool::stop(){
    m_bRunning=false;//结束任务
    m_cv.notify_all();
    for (auto &iter:m_threads)
    {   //希望等待所有线程完成它们当前正在执行的任务（或者等待它们退出）后，再继续执行程序的后续部分
        if(iter->joinable()) iter->join();
    }
    
}

void TaskPool::addTask(Task* task){
    std::shared_ptr<Task> spTask; 
    spTask.reset(task);//指向外面新传进来的task对象
    {
        std::lock_guard<std::mutex> gurad(m_mutexList);
        m_taskList.push_back(spTask);
        std::cout<<"add a task!!!"<<std::endl;
    }
    m_cv.notify_one();
}

void TaskPool::removeAllTasks(){
    {
        std::lock_guard<std::mutex> guard(m_mutexList);
        for (auto &iter:m_taskList)
        {
            iter.reset();//释放对象
        }
        m_taskList.clear();
    }
}
