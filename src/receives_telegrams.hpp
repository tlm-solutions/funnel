
#ifndef FUNNEL_RECEIVES_TELEGRAMS_HPP
#define FUNNEL_RECEIVES_TELEGRAMS_HPP

#include "./broadcast_server.hpp"

#include "protobuf/telegram.pb.h"
#include "protobuf/telegram.grpc.pb.h"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

class ReceivesTelegramsImpl final : public dvbdump::ReceivesTelegrams::Service {
    private:
        BroadcastServer websocket_server_;
        std::thread message_processer_;
        std::thread active_listener_;
    public:
        ReceivesTelegramsImpl(unsigned short websocket_port) noexcept;
        ~ReceivesTelegramsImpl() noexcept;
        auto receive_r09(grpc::ServerContext* context, const dvbdump::R09GrpcTelegram* telegram, dvbdump::ReturnCode* return_code) noexcept -> grpc::Status override;
};

#endif //FUNNEL_RECEIVES_TELEGRAMS_HPP
