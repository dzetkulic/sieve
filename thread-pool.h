#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

class ThreadPool {
  struct ThreadState {
    std::mutex mu;
    std::condition_variable cv;
    std::function<void()> f;
    ThreadState* next;
    bool ready = false, stop = false;
  };

 public:
  ThreadPool(int num_threads) : num_threads_(num_threads) {
    if (num_threads) std::thread([this]{Run(0);}).detach();
  }
    
  ~ThreadPool() {
    for (int i = 0; i < num_threads_; ++i) {
      ThreadState* state = Deque();
      std::lock_guard<std::mutex> lock(state->mu);
      state->ready = state->stop = true;
      state->cv.notify_one();
    }
  }
  
  void Add(std::function<void()> f) {
    ThreadState* state = Deque();
    {
      std::lock_guard<std::mutex> lock(state->mu);
      state->ready = true;
      state->f = std::move(f);
    }
    state->cv.notify_one();
  }

 private:
  void Run(int id) {
    if (2 * id + 1 < num_threads_) std::thread([=]{Run(2 * id + 1);}).detach();
    if (2 * id + 2 < num_threads_) std::thread([=]{Run(2 * id + 2);}).detach();
    ThreadState state;
    while (true) {
      bool notify = false;
      {
        std::lock_guard<std::mutex> lock(mu_);
        notify = !next_state_;
        state.next = std::exchange(next_state_, &state);
      }
      if (notify) cv_.notify_one();
      std::unique_lock<std::mutex> lock(state.mu);
      state.cv.wait(lock, [&](){ return state.ready; });    
      if (state.stop) break;
      std::exchange(state.f, {})();
      state.ready = false;
    }
  }
  
  ThreadState* Deque() {
    std::unique_lock<std::mutex> lock(mu_);
    cv_.wait(lock, [this] { return next_state_ != nullptr; });
    return std::exchange(next_state_, next_state_->next); 
  }
  
  std::mutex mu_;
  ThreadState* next_state_ = nullptr;
  std::condition_variable cv_;
  const int num_threads_;
};
