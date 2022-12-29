#ifndef FUNNEL_RECEIVES_TELEGRAMS_CPP
#define FUNNEL_RECEIVES_TELEGRAMS_CPP

#include "./receives_telegrams.hpp"

ReceivesTelegramsImpl::~ReceivesTelegramsImpl() noexcept {
    websocket_server_.kill();
    active_listener_.join();
    message_processor_.join();
}

auto ReceivesTelegramsImpl::receive_r09([[maybe_unused]]grpc::ServerContext *context,
                                        const tlms::R09GrpcTelegram *telegram,
                                        tlms::ReturnCode *return_code) noexcept -> grpc::Status {
    this->websocket_server_.queue_telegram(telegram);
    return_code->set_status(0);
    return grpc::Status::OK;
}

ReceivesTelegramsImpl::ReceivesTelegramsImpl(unsigned short websocket_port) noexcept {
    active_listener_ = std::thread([ObjectPtr = &websocket_server_] { ObjectPtr->process_messages(); });
    message_processor_ = std::thread([&]() { websocket_server_.run(websocket_port); });
}


#endif //FUNNEL_RECEIVES_TELEGRAMS_CPP

