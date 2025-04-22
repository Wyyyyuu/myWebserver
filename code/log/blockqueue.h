# ifndef BLOCKQUEUE_H
# define BLOCKQUEUE_H

#include <deque>
#include <condition_variable>
#include <mutex>
#include <sys/time.h>
using namespace std;

template<typename T>
class BlockQueue {
public:
    explicit BlockQueue(size_t maxsize = 1000);
    ~BlockQueue();
    bool empty();
    bool full();
    void push_back(const T& item);
    void push_front(const T& item); 
    bool pop(T& item);  // 弹出的任务放入item
    bool pop(T& item, int timeout);  // 等待时间
    void clear();
    T front();
    T back();
    size_t capacity();
    size_t size();

    void flush();
    void Close();

private:
    deque<T> deq_; // 底层数据结构
    mutex mtx_; // 锁
    bool isClose_; // 关闭标志
    size_t capacity_; // 最大容量
    condition_variable condConsumer_; // 消费者条件变量
    condition_variable condProducer_; // 生产者条件变量
};

template<typename T>
BlockQueue<T>::BlockQueue(size_t maxsize) : capacity_(maxsize) {
    assert(maxsize > 0);
    isClose_ = false;
}

template<typename T>
BlockQueue<T>::~BlockQueue() {
    Close();
}

// 关闭连接
template<typename T>
void BlockQueue<T>::Close() {
    clear();
    isClose_ = true;
    condConsumer_.notify_all();
    condProducer_.notify_all();

}

// 清空deque
template<typename T>
void BlockQueue<T>::clear() {
    lock_guard<mutex> locker(mtx_);
    deq_.clear();
}

// 是否为空
template<typename T>
bool BlockQueue<T>::empty() {
    lock_guard<mutex> locker(mtx_);
    return deq_.empty();
}

// 是否大于等于容量
template<typename T>
bool BlockQueue<T>::full() {
    lock_guard<mutex> locker(mtx_);
    return deq_.size() >= capacity_;
}

// push在队列的最后
template<typename T>
void BlockQueue<T>::push_back(const T& item) {
    // 条件变量需要搭配使用unique_lock，因为unique_lock允许在期间释放锁，条件变量需要在等待期间释放锁，被唤醒时再获得锁
    unique_lock<mutex> locker(mtx_);
    while (deq_.size() >= capacity_ && !isClose_) {  // 队列满了，需要等待
        condProducer_.wait(locker); // 等待消费者唤醒生产者条件变量
    }
    deq_.push_back(item);
    condConsumer_.notify_one(); // 唤醒消费者
}

// push在队列的最前
template<typename T>
void BlockQueue<T>::push_front(const T& item) {
    unique_lock<mutex> locker(mtx_);
    while (deq_.size() >= capacity_ && !isClose_) {
        condProducer_.wait(locker);
    }
    deq_.push_front(item);
    condConsumer_.notify_one();
}

// 把最前面的pop出去，并且把pop的元素赋给item
template<typename T>
bool BlockQueue<T>::pop(T& item) {
    unique_lock<mutex> locker(mtx_);
    while (deq_.empty() && !isClose_) { // 队列为空，需要等待
        condConsumer_.wait(locker);
    }
    if (deq_.empty() && isClose_) {
        return false;
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

// 设置超时时间
template<typename T>
bool BlockQueue<T>::pop(T &item, int timeout) {
    unique_lock<mutex> locker(mtx_);
    while (deq_.empty() && !isClose_) {
        auto status = condConsumer_.wait_for(locker, std::chrono::seconds(timeout));
        if (status == std::cv_status::timeout) {
            if (deq_.empty()) {
                return false;
            }
        }
        if (isClose_) {
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<typename T>
T BlockQueue<T>::front() {
    lock_guard<mutex> locker(mtx_);
    return deq_.front();
}

template<typename T>
T BlockQueue<T>::back() {
    lock_guard<mutex> locker(mtx_);
    return deq_.back();
}

template<typename T>
size_t BlockQueue<T>::capacity() {
    lock_guard<mutex> locker(mtx_);
    return capacity_;
}

template<typename T>
size_t BlockQueue<T>::size() {
    lock_guard<mutex> locker(mtx_);
    return deq_.size();
}

// 唤醒消费者
template<typename T>
void BlockQueue<T>::flush() {
    condConsumer_.notify_one();
}
# endif