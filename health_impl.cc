#include "health_impl.h"

#include <mutex>

#include <glog/logging.h>
#include <grpc/grpc.h>
#include <grpc++/server_context.h>

namespace fimdhealth {
grpc::Status HealthImpl::Check(grpc::ServerContext *context, const grpc::health::v1::HealthCheckRequest *request,
    grpc::health::v1::HealthCheckResponse *response) {

    std::lock_guard<std::mutex> lock(mu_);
    // if the service is empty we assume that the client wants to check the server's status
    if (request->service().empty()) {
        response->set_status(grpc::health::v1::HealthCheckResponse::SERVING);
        return grpc::Status::OK;
    }

    auto iter = statuses_.find(request->service());
    if (iter == statuses_.end()) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "");
    }
    response->set_status(iter->second);

    return grpc::Status::OK;
}

void HealthImpl::SetStatus(const grpc::string &service, grpc::health::v1::HealthCheckResponse::ServingStatus status) {
    std::lock_guard<std::mutex> lock(mu_);
    statuses_[service] = status;
}

void HealthImpl::SetAll(grpc::health::v1::HealthCheckResponse::ServingStatus status) {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto iter = statuses_.begin(); iter != statuses_.end(); ++iter) {
        iter->second = status;
    }
}

void HealthImpl::ClearStatus(const std::string &service) {
    std::lock_guard<std::mutex> lock(mu_);
    statuses_.erase(service);
}

void HealthImpl::ClearAll() {
    std::lock_guard<std::mutex> lock(mu_);
    statuses_.clear();
}
} // namespace fimdhealth