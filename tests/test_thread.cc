#include "sylar/sylar.h"

int count = 0;
//sylar::RWMutex s_mutex;
sylar::Mutex s_mutex;

void func1(){
    MYLOG_INFO(SYLAR_LOG_ROOT()) << "name=" << sylar::Thread::GetName()
                                 << " this.name=" << sylar::Thread::GetThis()->getName()
                                 << " id=" << sylar::getThreadId()
                                 << " this.id=" << sylar::Thread::GetThis()->getId();

    for(int i = 0; i < 100000; i++)
    {
//        sylar::RWMutex::WriteLock lock(s_mutex);
        sylar::Mutex::Lock lock(s_mutex);   // 进入下一次循环之后自动析构函数，触发RAII机制，所以会自动解锁 (如果你要中途释放锁,就用{}括起来)
        count++;
    }
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
    MYLOG_INFO(SYLAR_LOG_ROOT()) << " count = " << count;
    return 0;
}
