#ifndef AV_QUEUE_H
#define AV_QUEUE_H
#include <mutex>
#include <memory>
template<typename T>
class AVList {
public:
    AVList() : head_(nullptr), tail_(nullptr), size_(0) {}
    ~AVList() { clear(); }
    void push(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        Node* node = new Node(value);
        if (tail_) {
            tail_->next = node;
            node->prev = tail_;
            tail_ = node;
        } else {
            head_ = tail_ = node;
        }
        ++size_;
    }

    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!head_) {
            return false;
        }
        Node* node = head_;
        value = node->value;
        head_ = head_->next;
        if (head_) {
            head_->prev = nullptr;
        } else {
            tail_ = nullptr;
        }
        delete node;
        --size_;
        return true;
    }

    void clear() {
        std::unique_lock<std::mutex> lock(mutex_);
        Node* node = head_;
        while (node) {
            Node* next = node->next;
            delete node;
            node = next;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }
    bool empty() const { return size_ == 0; }
    int size() const { return size_; }

private:
    struct Node {
        T value;
        Node* prev;
        Node* next;
        Node(const T& v) : value(v), prev(nullptr), next(nullptr) {}
    };

private:
    Node* head_;
    Node* tail_;
    int size_;
    mutable std::mutex mutex_;
};

template<typename T>
class AVQueue
{
public:
    AVQueue() {}
    void push(T packet) {
        queue_.push(packet);
    }
    bool pop(T& packet) {
        return queue_.pop(packet);
    }
    bool empty() const { return queue_.empty(); }
    int size() const { return queue_.size(); }
    void clear() {
        queue_.clear();
    }

protected:
    AVList<T> queue_;
};
#endif // AV_QUEUE_H
