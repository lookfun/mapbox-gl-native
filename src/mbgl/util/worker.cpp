#include <mbgl/util/worker.hpp>
#include <mbgl/util/work_task.hpp>
#include <mbgl/util/work_request.hpp>
#include <mbgl/platform/platform.hpp>

#include <cassert>
#include <future>

namespace mbgl {

class Worker::Impl {
public:
    Impl(uv_loop_t*) {}

    void doWork(Fn work) {
        work();
    }
};

Worker::Worker(std::size_t count) {
    for (std::size_t i = 0; i < count; i++) {
        threads.emplace_back(std::make_unique<util::Thread<Impl>>("Worker", util::ThreadPriority::Low));
    }
}

Worker::~Worker() = default;

std::unique_ptr<WorkRequest> Worker::send(Fn work, Fn after) {
    current = (current + 1) % threads.size();
    return threads[current]->invokeWithResult(&Worker::Impl::doWork, after, work);
}

} // end namespace mbgl
