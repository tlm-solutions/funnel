
#ifndef FUNNEL_BROADCAST_SERVER_CPP
#define FUNNEL_BROADCAST_SERVER_CPP

#include "./broadcast_server.hpp"

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>

#include <google/protobuf/util/type_resolver_util.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/message.h>

#include <utility>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

BroadcastServer::BroadcastServer() noexcept {
    // setting the state of the broadcast_server to active
    kill_.store(false);

    // Initialize Asio Transport
    server_.init_asio();

    // Register handler callbacks
    server_.set_open_handler([this](auto &&PH1) { on_open(std::forward<decltype(PH1)>(PH1)); });
    server_.set_close_handler([this](auto &&PH1) { on_close(std::forward<decltype(PH1)>(PH1)); });
    server_.set_message_handler([this](auto &&PH1, auto &&PH2) {
        on_message(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
    server_.clear_access_channels(websocketpp::log::alevel::frame_header | websocketpp::log::alevel::frame_payload);

}

BroadcastServer::~BroadcastServer() noexcept {
    server_.reset();
}

void BroadcastServer::run(uint16_t port) noexcept {
    server_.listen(port);
    server_.start_accept();

    try {
        server_.run();
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}

void BroadcastServer::on_open(connection_hdl hdl) noexcept {
    {
        lock_guard<mutex> guard(action_lock_);
        actions_.push(action(SUBSCRIBE, std::move(hdl)));
    }
    action_condition_.notify_one();
}

void BroadcastServer::on_close(connection_hdl hdl) noexcept {
    {
        lock_guard<mutex> guard(action_lock_);
        actions_.push(action(UNSUBSCRIBE, std::move(hdl)));
    }
    action_condition_.notify_one();
}

void BroadcastServer::on_message(connection_hdl hdl, server::message_ptr msg) noexcept {
    // queue message up for sending by processing thread
    {
        lock_guard<mutex> guard(action_lock_);
        actions_.push(action(MESSAGE, std::move(hdl), std::move(msg)));
    }
    action_condition_.notify_one();
}

void BroadcastServer::process_messages() noexcept {
    // gets prometheus counter
    auto &opened_connections = exporter_.get_opened_connections();
    auto &closed_connections = exporter_.get_closed_connections();

    // initializes the two labels
    opened_connections.Add({{"count", "accumulative"}});
    closed_connections.Add({{"count", "accumulative"}});

    while (not kill_) {
        std::unique_lock<mutex> lock(action_lock_);

        while (actions_.empty()) {
            action_condition_.wait(lock);
        }

        action a = actions_.front();
        actions_.pop();

        lock.unlock();

        if (a.type == SUBSCRIBE) {
            std::lock_guard<mutex> guard(connection_lock_);

            // add new connection to connection pool
            connections_.push_back(a.hdl);
            filters_.emplace_back();
            opened_connections.Add({{"count", "accumulative"}}).Increment();

        } else if (a.type == UNSUBSCRIBE) {
            std::lock_guard<mutex> guard(connection_lock_);

            const auto pos = std::find_if(connections_.begin(), connections_.end(), [&a](const connection_hdl &ptr1) {
                return ptr1.lock().get() == ((const std::weak_ptr<void> &) a.hdl).lock().get();
            });

            auto index = pos - std::begin(connections_);

            // after successfully searching the connection we remove it from the connection poool
            connections_.erase(std::begin(connections_) + index);
            filters_.erase(std::begin(filters_) + index);

            closed_connections.Add({{"count", "accumulative"}}).Increment();

        } else if (a.type == MESSAGE) {
            std::string message = a.msg->get_payload();

            // parses string to filter struct
            JS::ParseContext context(message);
            Filter filter;
            JS::Error return_value = context.parseTo(filter);

            // no error while decoding payload from user
            if (return_value == JS::Error::NoError) {
                // we sadly need a lock here to make sure that nobody deletes the connection while writing
                std::lock_guard<mutex> guard(connection_lock_);

                // searches for connection
                const auto pos = std::find_if(connections_.begin(), connections_.end(),
                                              [&a](const connection_hdl &ptr1) {
                                                  return ptr1.lock().get() ==
                                                         ((const std::weak_ptr<void> &) a.hdl).lock().get();
                                              });

                auto index = pos - std::begin(connections_);

                // overwrites old filter
                filters_[index] = filter;
            }
        } else {
            // undefined.
        }
    }
}

void BroadcastServer::queue_waypoint(const tlms::GrpcWaypoint *waypoint) noexcept {
    // serialize the protobuf struct into json string
    std::string plain_serialized;
    plain_serialized.reserve(400);

    google::protobuf::util::JsonPrintOptions options;
    options.always_print_enums_as_ints = true;
    options.add_whitespace = true;
    options.preserve_proto_field_names = true;
    options.always_print_enums_as_ints = true;

    google::protobuf::util::MessageToJsonString(*waypoint, &plain_serialized, options);
    

    // lock connection list and yeet the waypoint to all peers
    {
        std::lock_guard<std::mutex> guard(connection_lock_);
        //connection_list::iterator it;
        auto filter_iterator = filters_.begin();
        for (auto &connection: connections_) {
            if (filter_iterator->match(waypoint->line(), waypoint->region())) {
                server_.send(connection, plain_serialized, websocketpp::frame::opcode::TEXT);
            }
            filter_iterator++;
        }
    }
}

void BroadcastServer::kill() noexcept {
    kill_.store(true);
}


#endif //FUNNEL_BROADCAST_SERVER_CPP
