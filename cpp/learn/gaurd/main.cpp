#include <mutex>
#include <iostream>

namespace my_space {

    template <typename T>
    class my_lock_guard {
    public:
        my_lock_guard(T& mutex) : mutex_(mutex) {
            mutex_.lock();
        }
        ~my_lock_guard() {
            mutex_.unlock();
        }
    private:
        my_lock_guard(const my_lock_guard&);
        my_lock_guard& operator=(const my_lock_guard&);
        T& mutex_;
    };

};

int main() {

    std::mutex mutex;

    my_space::my_lock_guard<std::mutex> my_guard(mutex);
    std::cout<<"my_lock_guard!!"<<std::endl;
}
// g++ -g -o test main.cpp


int main() {

    std::mutex mutex;
    mutex.lock();
    std::cout<<"my_lock_guard!!"<<std::endl;
    mutex.unlock();
}
