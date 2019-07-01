#pragma once

#include <mutex>
#include <type_traits>

namespace mixxx {

template<typename T>
class MutexWrapper {
  public:
    class Lock {
      public:
        Lock(Lock&&) noexcept = default;

        Lock& operator=(Lock&&) noexcept = default;

        // This function is lvalue ref-qualified because it shouldn't be called on temporaries
        T& operator*() & noexcept {
            return *m_pWrapped;
        }

        // This function is lvalue ref-qualified because it shouldn't be called on temporaries
        T* operator->() & noexcept {
            return m_pWrapped;
        }

      private:
        Lock(T& wrapped, std::mutex& mtx) : m_pWrapped{&wrapped}, m_lock{mtx} {
        }

        T* m_pWrapped;
        std::unique_lock<std::mutex> m_lock;

        friend class MutexWrapper;
    };

    template<typename = typename std::enable_if<std::is_default_constructible<T>::value, int>::type>
    MutexWrapper() noexcept(std::is_nothrow_default_constructible<T>::value){};

    explicit MutexWrapper(const T& t) : m_wrapped(t) {
    }

    explicit MutexWrapper(T&& t) : m_wrapped(std::move(t)) {
    }

    Lock lock() {
        return Lock{m_wrapped, m_mtx};
    }

  private:
    T m_wrapped;
    std::mutex m_mtx;
};

} // namespace mixxx
