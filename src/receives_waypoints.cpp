#ifndef FUNNEL_RECEIVES_WAYPOINTS_CPP
#define FUNNEL_RECEIVES_WAYPOINTS_CPP

#include "./receives_waypoints.hpp"

ReceivesWaypointsImpl::ReceivesWaypointsImpl(unsigned short websocket_port) noexcept {
    active_listener_ = std::thread([ObjectPtr = &websocket_server_] { ObjectPtr->process_messages(); });
    message_processor_ = std::thread([&]() { websocket_server_.run(websocket_port); });
}

ReceivesWaypointsImpl::~ReceivesWaypointsImpl() noexcept {
    websocket_server_.kill();
    active_listener_.join();
    message_processor_.join();
}

auto ReceivesWaypointsImpl::receive_waypoint([[maybe_unused]]grpc::ServerContext *context,
                                        const tlms::GrpcWaypoint* waypoint,
                                        tlms::ReturnCode *return_code) noexcept -> grpc::Status {
    this->websocket_server_.queue_waypoint(waypoint);
    return_code->set_status(0);
    return grpc::Status::OK;
}

#endif //FUNNEL_RECEIVES_WAYPOINTS_CPP

