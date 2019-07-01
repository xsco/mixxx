#include "jobs/qtjobscheduler.h"

#include <QTimer>

#include "util/parented_ptr.h"

namespace mixxx {

QtJobScheduler::QtJobScheduler() : m_pTimer{std::make_unique<QTimer>()} {
    QObject::connect(m_pTimer.get(), &QTimer::timeout, m_pTimer.get(), [this]() { update(); });
    m_pTimer->start();
}

void QtJobScheduler::onUpdatable() {
    // Do nothing. Currently we just call `update()` on every event loop iteration.
}

} // namespace mixxx
