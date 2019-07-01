#pragma once

#include "jobs/jobscheduler.h"

class QTimer;

namespace mixxx {

class QtJobScheduler : public JobScheduler {
  public:
    QtJobScheduler();

  private:
    void onUpdatable() override;

    std::unique_ptr<QTimer> m_pTimer;
};

} // namespace mixxx
