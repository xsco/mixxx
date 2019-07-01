#include "util/threadjoiner.h"

namespace mixxx {

ThreadJoiner::ThreadJoiner() noexcept = default;

ThreadJoiner::ThreadJoiner(std::thread t) noexcept : m_thread(std::move(t)) {
}

ThreadJoiner::ThreadJoiner(ThreadJoiner&& other) noexcept : m_thread{std::move(other.m_thread)} {
}

ThreadJoiner::~ThreadJoiner() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

ThreadJoiner& ThreadJoiner::operator=(ThreadJoiner&& other) noexcept {
    m_thread = std::move(other.m_thread);
    return *this;
}

std::thread& ThreadJoiner::asThread() {
    return m_thread;
}

const std::thread& ThreadJoiner::asThread() const {
    return m_thread;
}

} // namespace mixxx
