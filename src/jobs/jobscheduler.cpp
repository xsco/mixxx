#include "jobs/jobscheduler.h"

#include <algorithm>

namespace mixxx {

int64_t JobScheduler::Job::id() const noexcept {
    return m_id;
}

std::string& JobScheduler::Job::name() const noexcept {
    return m_pState->name;
}

double& JobScheduler::Job::progress() const noexcept {
    return m_pState->progress;
}

bool& JobScheduler::Job::terminationRequested() const noexcept {
    return m_pState->terminationRequested;
}

JobScheduler::Job::Job(int64_t id) : m_id{id}, m_pState{std::make_shared<State>()} {
}

void JobScheduler::Connection::setName(std::string name) const {
    runSync([& state = m_state, name = std::move(name)]() { state.name = std::move(name); });
}

void JobScheduler::Connection::setProgress(double progress) const {
    if (progress < 0 || progress > 1) {
        // TODO (haslersn): Throw something or so
    }
    runSync([& state = m_state, progress]() { state.progress = progress; });
}

std::string JobScheduler::Connection::name() const {
    return runSync([&state = m_state]() { return state.name; }).get();
}

double JobScheduler::Connection::progress() const {
    return runSync([&state = m_state]() { return state.progress; }).get();
}

bool JobScheduler::Connection::terminationRequested() const {
    return runSync([&state = m_state]() { return state.terminationRequested; }).get();
}

JobScheduler::Connection::Connection(JobScheduler& scheduler, State& state)
        : m_scheduler{scheduler}, m_state{state} {
}

JobScheduler::JobScheduler() = default;

JobScheduler::~JobScheduler() {
    for (auto& job : m_jobs) {
        job.terminationRequested() = true;
    }
    // Wait until all jobs are actually terminated
    while (!m_jobs.empty()) {
        // TODO (haslersn): Try to solve this without polling.
        update();
        removeFinished();
    }
}

const std::vector<JobScheduler::Job>& JobScheduler::jobs() {
    return m_jobs;
}

void JobScheduler::update() {
    while (true) {
        std::future<void> deferred_task;
        {
            auto lock = m_deferredTasks.lock();
            if (lock->empty()) {
                break;
            }
            deferred_task = std::move(lock->front());
            lock->pop_front();
        }
        deferred_task.wait();
    }
}

void JobScheduler::removeFinished() {
    auto new_end = std::remove_if(m_jobs.begin(), m_jobs.end(), [](const Job& job) {
        auto& asyncTask = job.m_pState->asyncTask;
        if (!asyncTask.valid()) {
            return true;
        }
        auto status = asyncTask.wait_for(std::chrono::steady_clock::duration::zero());
        return status == std::future_status::ready;
    });
    m_jobs.erase(new_end, m_jobs.end());
}

} // namespace mixxx
