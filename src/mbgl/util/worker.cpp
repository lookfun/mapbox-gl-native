#include <mbgl/util/worker.hpp>
#include <mbgl/util/work_task.hpp>
#include <mbgl/util/work_request.hpp>
#include <mbgl/platform/platform.hpp>
#include <mbgl/map/vector_tile.hpp>
#include <mbgl/map/live_tile.hpp>
#include <mbgl/util/pbf.hpp>
#include <mbgl/renderer/raster_bucket.hpp>

#include <cassert>
#include <future>

namespace mbgl {

class Worker::Impl {
public:
    Impl(uv_loop_t*) {}

    bool parseRasterTile(RasterBucket* bucket, std::string data) {
        return bucket->setImage(data);
    }

    TileParseResult parseVectorTile(TileWorker* worker, std::string data) {
        VectorTile vectorTile(pbf(reinterpret_cast<const unsigned char *>(data.data()), data.size()));
        return worker->parse(vectorTile);
    }

    TileParseResult parseLiveTile(TileWorker* worker, const LiveTile* tile) {
        return worker->parse(*tile);
    }

    void redoPlacement(TileWorker* worker, float angle, bool collisionDebug) {
        worker->redoPlacement(angle, collisionDebug);
    }
};

Worker::Worker(std::size_t count) {
    for (std::size_t i = 0; i < count; i++) {
        threads.emplace_back(std::make_unique<util::Thread<Impl>>("Worker", util::ThreadPriority::Low));
    }
}

Worker::~Worker() = default;

std::unique_ptr<WorkRequest> Worker::parseRasterTile(RasterBucket& bucket, std::string data, std::function<void (bool)> callback) {
    current = (current + 1) % threads.size();
    return threads[current]->invokeWithResult(&Worker::Impl::parseRasterTile, callback, &bucket, data);
}

std::unique_ptr<WorkRequest> Worker::parseVectorTile(TileWorker& worker, std::string data, std::function<void (TileParseResult)> callback) {
    current = (current + 1) % threads.size();
    return threads[current]->invokeWithResult(&Worker::Impl::parseVectorTile, callback, &worker, data);
}

std::unique_ptr<WorkRequest> Worker::parseLiveTile(TileWorker& worker, const LiveTile& tile, std::function<void (TileParseResult)> callback) {
    current = (current + 1) % threads.size();
    return threads[current]->invokeWithResult(&Worker::Impl::parseLiveTile, callback, &worker, &tile);
}

std::unique_ptr<WorkRequest> Worker::redoPlacement(TileWorker& worker, float angle, bool collisionDebug, std::function<void ()> callback) {
    current = (current + 1) % threads.size();
    return threads[current]->invokeWithResult(&Worker::Impl::redoPlacement, callback, &worker, angle, collisionDebug);
}

} // end namespace mbgl
