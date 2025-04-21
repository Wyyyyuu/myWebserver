#include "buffer.h"

// 初始化
Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {}

// 剩余可写的空间
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

// 剩余可读的空间
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

// 剩余预留空间
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

// 用来返回读指针在vector中的具体位置
const char* Buffer::Peek() const {
    return &buffer_[readPos_];
}

// 确保可写空间是否充足
void Buffer::EnsureWritable(size_t len) {
    if (len > WritableBytes()) {
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}

// 移动写下标
void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

// 移动读下标
void Buffer::Retrieve(size_t len) {
    readPos_ += len;
}

// 读取到end位置
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

// 取出所有数据，buffer归零，读写下标归零,在别的函数中会用到
void Buffer::RetrieveAll() {
    bzero(&buffer_, buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

// 取出剩余可读的str
std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

// 写指针的位置
const char* Buffer::BeginWriteConst() const {
    return &buffer_[writePos_];
}

char* Buffer::BeginWrite() {
    return &buffer_[writePos_];
}

// 添加str到缓冲区
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWritable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len); // 移动写下标
}

void Buffer::Append(const std::string& str) {
    size_t len = str.length();
    EnsureWritable(len);
    std::copy(str.begin(), str.end(), BeginWrite());
    HasWritten(len);
}

void Append(const void* data, size_t len) {
    Append(static_cast<const char*>(data), len);
}

// 将buffer中的读下标的地方放到该buffer中的写下标位置
void Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

// 将fd的内容读到缓冲区，即writable的位置
ssize_t Buffer::ReadFd(int fd, int* Errno) {
    char buff[65536]; // 临时栈
    struct iovec iov[2];
    size_t writable = WritableBytes();

    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    ssize_t ret = readv(fd, iov, 2);
    if (ret < 0) {
        *Errno = errno;
    } else if (static_cast<size_t>(ret) <= writable) { // 说明用不到栈空间
        HasWritten(static_cast<size_t>(ret));
    } else {
        writePos_ = buffer_.size();
        Append(buff, static_cast<size_t>(ret - writable));
    }
    return ret;
}

// 将buffer中可读的区域写入fd中
ssize_t Buffer::WriteFd(int fd, int* Errno) {
    ssize_t ret = write(fd, Peek(), ReadableBytes());
    if (ret < 0) {
        *Errno = errno;
        return ret;
    }
    Retrieve(ret);
    return ret;
}

// 起始位置
char* Buffer::BeginPtr_() {
    return &buffer_[0];
}

const char* Buffer::BeginPtr_() const{
    return &buffer_[0];
}

// 扩展空间
void Buffer::MakeSpace_(size_t len) {
    if (len <= WritableBytes() + PrependableBytes()) {
        size_t readBytes = ReadableBytes();
        std::copy(Peek(), Peek() + ReadableBytes(), BeginPtr_());
        readPos_ = 0;
        writePos_ = readBytes;
    } else {
        buffer_.resize(writePos_ + len + 1);
    }
}
