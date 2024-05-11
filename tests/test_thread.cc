#include "sylar/sylar.h"

void func1(){
    MYLOG_INFO(SYLAR_LOG_ROOT()) << "name=" << sylar::Thread::GetName()
                                 << "this.name=" << sylar::Thread::GetThis()->getName()
                                 << "id=" << sylar::getThreadId()
                                 << "this.id=" << sylar::Thread::GetThis()->getId();
}
void func2(){

}

int main(int argc, char** argv){
    MYLOG_INFO(SYLAR_LOG_ROOT()) << " test thread begin--";
    std::vector<sylar::Thread::ptr> threads;
    for(int i = 0; i < 10; i++){
        sylar::Thread::ptr thrd(new sylar::Thread(&func1, "name_" + std::to_string(i)));
        threads.push_back(thrd);
    }

    for(int i = 0; i < 10; i++){
        threads[i]->join();
    }
    MYLOG_INFO(SYLAR_LOG_ROOT()) << " test thread end--";
    return 0;
}
