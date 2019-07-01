#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <future>
#include <vector>

#include "util/mutexwrapper.h"

namespace mixxx {

class JobScheduler {
  private:
    struct State {
        std::string name;
        double progress = 0;
        bool terminationRequested = false;
        std::future<void> asyncTask;
    };

  public:
    // Just a handle that can cheaply be copied
    class Job {
      public:
        int64_t id() const noexcept;

        std::string& name() const noexcept;

        double& progress() const noexcept;

        bool& terminationRequested() const noexcept;

      private:
        Job(int64_t id);

        int64_t m_id;
        std::shared_ptr<State> m_pState;

        friend class JobScheduler;
    };

    class Connection {
      public:
        template<typename Functor>
        auto runSync(Functor&& fn) const -> std::future<decltype(fn())> {
            return m_scheduler.runSync(std::forward<Functor>(fn));
        }

        void setName(std::string name) const;

        void setProgress(double progress) const;

        std::string name() const;

        double progress() const;

        bool terminationRequested() const;

      private:
        Connection(JobScheduler& scheduler, State& state);

        JobScheduler& m_scheduler;
        State& m_state;

        friend class JobScheduler;
    };

    JobScheduler();

    JobScheduler(const JobScheduler&) = delete;
    JobScheduler(JobScheduler&&) = delete;

    virtual ~JobScheduler();

    JobScheduler& operator=(const JobScheduler&) = delete;
    JobScheduler& operator=(JobScheduler&&) = delete;

    template<typename Functor>
    auto run(Functor&& fn)
            -> std::pair<Job, std::future<decltype(fn(std::declval<Connection>()))>> {
        // remove finished tasks first
        removeFinished();

        // insert the new task
        int64_t id = ++m_lastId; // TODO (haslersn): Do we even need an ID?
        m_jobs.push_back(Job{id});
        auto job = m_jobs.back();
        auto& state = *job.m_pState;
        Connection con{*this, state};
        auto task = std::packaged_task<decltype(fn(std::declval<Connection>()))(Connection)>{
                std::forward<Functor>(fn)};
        auto future = task.get_future();
        state.asyncTask = std::async(std::launch::async, std::move(task), con);
        return {std::move(job), std::move(future)};
    }

    const std::vector<Job>& jobs();

  protected:
    void update();

  private:
    template<typename Functor>
    auto runSync(Functor&& fn) -> std::future<decltype(fn())> {
        auto task = std::packaged_task<decltype(fn())()>{std::forward<Functor>(fn)};
        auto future = task.get_future();
        auto void_task = [task = std::move(task)]() mutable { task(); };
        auto deferredTask = std::async(std::launch::deferred, std::move(void_task));
        {
            auto lock = m_deferredTasks.lock();
            lock->push_back(std::move(deferredTask));
        }
        return future;
    }

    void removeFinished();

    virtual void onUpdatable() = 0;

    int64_t m_lastId{0};
    std::vector<Job> m_jobs;
    MutexWrapper<std::deque<std::future<void>>> m_deferredTasks;
};

} // namespace mixxx
