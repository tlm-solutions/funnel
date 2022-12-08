
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


    if (std::getenv("API_DOMAIN") == nullptr) {
        api_url_ = std::getenv("API_DOMAIN");
        std::cout << "API_DOMAIN FOUND:" << api_url_.value() << std::endl;
    } else {
        api_url_ = std::nullopt;
        std::cout << "NO API_DOMAIN FOUND" << std::endl;
    };
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

auto BroadcastServer::fetch_api(unsigned int line, unsigned int run, unsigned int region) const noexcept
-> dvbdump::Edge * {
    if (!api_url_.has_value()) {
        return nullptr;
    }

    RequestInterpolated request{line, run};

    std::string body = JS::serializeStruct(request);

    std::list<std::string> header;
    header.emplace_back("Content-Type: application/json");

    curlpp::Cleanup clean;
    curlpp::Easy r;
    r.setOpt(new curlpp::options::Url(api_url_.value() + "/vehicles/" + std::to_string(region) + "/position"));
    r.setOpt(new curlpp::options::HttpHeader(header));
    r.setOpt(new curlpp::options::PostFields(body));
    r.setOpt(new curlpp::options::PostFieldSize(static_cast<long>(body.length())));

    std::stringstream response;
    r.setOpt(new curlpp::options::WriteStream(&response));

    r.perform();

    auto http_code = curlpp::infos::ResponseCode::get(r);

    if (http_code == 200) {
        std::string json_string = response.str();
        auto protobuf_string = google::protobuf::StringPiece(json_string);
        google::protobuf::util::JsonParseOptions parse_options;
        parse_options.case_insensitive_enum_parsing = false;
        parse_options.ignore_unknown_fields = true;
        auto *interpolation_struct = new dvbdump::Edge();

        auto status = google::protobuf::util::JsonStringToMessage(protobuf_string, interpolation_struct, parse_options);

        if (status.ok()) {
            return interpolation_struct;
        } else {
            std::cout << "couldn't decode:" << json_string << " error:" << status.message() << std::endl;
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

void BroadcastServer::queue_telegram(const dvbdump::R09GrpcTelegram *telegram) noexcept {
    // serialize the protobuf struct into json string
    std::string plain_serialized;
    plain_serialized.reserve(400);

    std::string enriched_serialized;
    enriched_serialized.reserve(600);

    google::protobuf::util::JsonPrintOptions options;
    options.always_print_primitive_fields = true;
    options.preserve_proto_field_names = true;
    options.always_print_enums_as_ints = true;

    google::protobuf::util::MessageToJsonString(*telegram, &plain_serialized, options);
    auto interpolation_data = fetch_api(telegram->line(), telegram->run_number(), telegram->region());

    bool enrichment_possible = interpolation_data != nullptr;
    dvbdump::R09GrpcTelegramEnriched enriched_telegram;
    if (enrichment_possible) {
        enriched_telegram.set_time(telegram->time());
        enriched_telegram.set_station(telegram->station());
        enriched_telegram.set_region(telegram->region());
        enriched_telegram.set_telegram_type(telegram->telegram_type());
        enriched_telegram.set_delay(telegram->delay());
        enriched_telegram.set_reporting_point(telegram->reporting_point());
        enriched_telegram.set_junction(telegram->junction());
        enriched_telegram.set_direction(telegram->direction());
        enriched_telegram.set_request_status(telegram->request_status());
        enriched_telegram.set_priority(telegram->priority());
        enriched_telegram.set_direction_request(telegram->direction_request());
        enriched_telegram.set_line(telegram->line());
        enriched_telegram.set_run_number(telegram->run_number());
        enriched_telegram.set_destination_number(telegram->destination_number());
        enriched_telegram.set_train_length(telegram->train_length());
        enriched_telegram.set_vehicle_number(telegram->vehicle_number());
        enriched_telegram.set_operator_(telegram->operator_());
        enriched_telegram.set_allocated_enriched(interpolation_data);

        google::protobuf::util::MessageToJsonString(enriched_telegram, &enriched_serialized, options);
    }

    // lock connection list and yeet the telegram to all peers
    {
        std::lock_guard<std::mutex> guard(connection_lock_);
        //connection_list::iterator it;
        auto filter_iterator = filters_.begin();
        for (auto &connection: connections_) {
            if (filter_iterator->match(telegram->line(), telegram->reporting_point(), telegram->region())) {
                if (filter_iterator->enrich && enrichment_possible) {
                    server_.send(connection, enriched_serialized, websocketpp::frame::opcode::TEXT);
                } else {
                    server_.send(connection, plain_serialized, websocketpp::frame::opcode::TEXT);
                }
            }
            filter_iterator++;
        }
    }
}

void BroadcastServer::kill() noexcept {
    kill_.store(true);
}


#endif //FUNNEL_BROADCAST_SERVER_CPP
