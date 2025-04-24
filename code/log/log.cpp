#include "log.h"

// 构造函数
Log::Log() {
    fp_ = nullptr;
    deque_ = nullptr;
    writeThread_ = nullptr;
    lineCount_ = 0;
    toDay_ = 0;
    isAsync_ = false;
}

Log::~Log() {
    while (!deque_->empty()) {
        deque_->flush(); // 当队列不为空，需要不断唤醒消费者处理完剩余任务
    }
    deque_->Close(); // 关闭队列
    writeThread_->join(); // 等待写线程完成任务
    if (fp_) { // 冲洗文件缓冲区，关闭文件描述符
        lock_guard<mutex> locker(mtx_);
        flush(); // 清空缓冲区中的数据
        fclose(fp_); // 关闭日志文件
    }
}

// 唤醒阻塞队列消费者，开始写日志
void Log::flush() {
    if (isAsync_) {
        deque_->flush(); // 只有异步才需要用到deque
    }
    fflush(fp_); // 清空缓冲区
}

// 懒汉模式 局部静态变量法（这种方法不需要加锁和解锁操作）
Log* Log::Instance() {
    static Log log;
    return &log;
}

// 异步日志的写线程函数
void Log::FlushLogThread() {
    Log::Instance()->Log::AsyncWrite_();
}

// 写线程真正的执行函数
void Log::AsyncWrite_() {

}

// 初始化日志实例
void Log::init(int level, const char* path, const char* suffix, int maxQueCapacity) {

}

void Log::write(int level, const char *format, ...) {

}

// 添加日志等级
void Log::AppendLogLevelTitle_(int level) {

}

int Log::GetLevel() {

}

void Log::SetLevel(int level) {

}