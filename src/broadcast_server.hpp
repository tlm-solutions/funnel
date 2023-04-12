#ifndef FUNNEL_BROADCAST_SERVER_HPP
#define FUNNEL_BROADCAST_SERVER_HPP

#include "protobuf/telegram.pb.h"
#include "filter.hpp"
#include "prometheus.hpp"
#include "api_request.hpp"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>
#include <google/protobuf/util/json_util.h>

#include <iostream>
#include <atomic>
#include <utility>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::bind;

using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::lock_guard;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;

enum action_type {
    SUBSCRIBE,
    UNSUBSCRIBE,
    MESSAGE
};

struct action {
    action(action_type t, connection_hdl h) : type(t), hdl(std::move(h)) {}
    action(action_type t, connection_hdl h, server::message_ptr m)
      : type(t), hdl(std::move(h)), msg(std::move(m)) {}

    action_type type;
    websocketpp::connection_hdl hdl;
    server::message_ptr msg;
};

class BroadcastServer {
private:
    server server_;
    std::vector<connection_hdl> connections_;
    std::vector<Filter> filters_;

    std::queue<action> actions_;
    std::atomic<bool> kill_{};

    std::mutex action_lock_;
    std::mutex connection_lock_;
    std::condition_variable action_condition_;

    PrometheusExporter exporter_;

public:
    BroadcastServer() noexcept;
    ~BroadcastServer() noexcept;

    void run(uint16_t port) noexcept;
    void on_open(connection_hdl hdl) noexcept;
    void on_close(connection_hdl hdl) noexcept;
    void on_message(connection_hdl hdl, server::message_ptr msg) noexcept;
    void process_messages() noexcept;
    void queue_waypoint(const tlms::GrpcWaypoint* waypoint) noexcept;
    void kill() noexcept;
};

#endif //FUNNEL_BROADCAST_SERVER_HPP

