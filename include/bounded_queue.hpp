#ifndef BOUNDED_QUEUE_HPP
#define BOUNDED_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class BoundedQueue
{
public:
  explicit BoundedQueue(size_t maxSize)
      : maxSize_(maxSize), closed_(false) {}

  // Push an item. Returns false if the queue is closed.
  bool push(const T &item)
  {
    std::unique_lock<std::mutex> lock(mtx_);
    cond_not_full_.wait(lock, [this]
                        { return queue_.size() < maxSize_ || closed_; });
    if (closed_)
      return false;
    queue_.push(item);
    cond_not_empty_.notify_one();
    return true;
  }

  // Pop an item. Returns false if queue is empty *and* closed.
  bool pop(T &item)
  {
    std::unique_lock<std::mutex> lock(mtx_);
    cond_not_empty_.wait(lock, [this]
                         { return !queue_.empty() || closed_; });
    if (queue_.empty())
      return false;
    item = std::move(queue_.front());
    queue_.pop();
    cond_not_full_.notify_one();
    return true;
  }

  // Signal no more pushes; wakes up any waiting threads.
  void close()
  {
    {
      std::lock_guard<std::mutex> lock(mtx_);
      closed_ = true;
    }
    cond_not_empty_.notify_all();
    cond_not_full_.notify_all();
  }

private:
  std::queue<T> queue_;
  std::mutex mtx_;
  std::condition_variable cond_not_empty_;
  std::condition_variable cond_not_full_;
  size_t maxSize_;
  bool closed_;
};

#endif // BOUNDED_QUEUE_HPP
