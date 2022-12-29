
#ifndef FUNNEL_RECEIVES_TELEGRAMS_HPP
#define FUNNEL_RECEIVES_TELEGRAMS_HPP

#include "./broadcast_server.hpp"

#include "protobuf/telegram.pb.h"
#include "protobuf/telegram.grpc.pb.h"

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

class ReceivesTelegramsImpl final : public tlms::ReceivesTelegrams::Service {
private:
    BroadcastServer websocket_server_;
    std::thread message_processor_;
    std::thread active_listener_;
public:
    explicit ReceivesTelegramsImpl(unsigned short websocket_port) noexcept;
    ~ReceivesTelegramsImpl() noexcept override;

    auto receive_r09(grpc::ServerContext *context, const tlms::R09GrpcTelegram *telegram,
                     tlms::ReturnCode *return_code) noexcept -> grpc::Status override;
};

#endif //FUNNEL_RECEIVES_TELEGRAMS_HPP
