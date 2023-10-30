#include<iostream>

int main(){
    int *p =new int(10);
    std::cout<<*p<<std::endl;
}


// sudo apt update
// sudo apt install valgrind
// valgrind --leak-check=full ./test