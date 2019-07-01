#pragma once

#include <thread>

namespace mixxx {

class ThreadJoiner {
  public:
    ThreadJoiner() noexcept;

    ThreadJoiner(std::thread t) noexcept;

    ThreadJoiner(ThreadJoiner&& other) noexcept;

    ~ThreadJoiner();

    ThreadJoiner& operator=(ThreadJoiner&& other) noexcept;

    std::thread& asThread();

    const std::thread& asThread() const;

  private:
    std::thread m_thread;
};

template<typename... Args>
ThreadJoiner makeThreadJoiner(Args&&... args) {
    return ThreadJoiner(std::thread{std::forward<Args>(args)...});
}

} // namespace mixxx
