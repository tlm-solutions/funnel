#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_

#include "protobuf/telegram.pb.h"
#include "protobuf/telegram.grpc.pb.h"

#include <google/protobuf/util/json_util.h>
#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>


#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>

#include <iostream>
#include <set>


typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
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
    action(action_type t, connection_hdl h) : type(t), hdl(h) {}
    action(action_type t, connection_hdl h, server::message_ptr m)
      : type(t), hdl(h), msg(m) {}

    action_type type;
    websocketpp::connection_hdl hdl;
    server::message_ptr msg;
};

class BroadcastServer {
public:
    BroadcastServer () {
        // Initialize Asio Transport
        server_.init_asio();

        // Register handler callbacks
        server_.set_open_handler(bind(&BroadcastServer::on_open,this,::_1));
        server_.set_close_handler(bind(&BroadcastServer::on_close,this,::_1));
        server_.set_message_handler(bind(&BroadcastServer::on_message,this,::_1,::_2));
    }

    void run(uint16_t port) {
        server_.listen(port);
        server_.start_accept();

        try {
            server_.run();
        } catch (const std::exception & e) {
            std::cout << e.what() << std::endl;
        }
    }

    void on_open(connection_hdl hdl) {
        {
            lock_guard<mutex> guard(action_lock_);
            std::cout << "on_open" << std::endl;
            actions_.push(action(SUBSCRIBE,hdl));
        }
        action_condition_.notify_one();
    }

    void on_close(connection_hdl hdl) {
        {
            lock_guard<mutex> guard(action_lock_);
            std::cout << "on_close" << std::endl;
            actions_.push(action(UNSUBSCRIBE,hdl));
        }
        action_condition_.notify_one();
    }

    void on_message(connection_hdl hdl, server::message_ptr msg) {
        // queue message up for sending by processing thread
        {
            lock_guard<mutex> guard(action_lock_);
            std::cout << "on_message" << std::endl;
            actions_.push(action(MESSAGE,hdl,msg));
        }
        action_condition_.notify_one();
    }

    void process_messages() {
        while(1) {
            unique_lock<mutex> lock(action_lock_);

            while(actions_.empty()) {
                action_condition_.wait(lock);
            }

            action a = actions_.front();
            actions_.pop();

            lock.unlock();

            std::cout << "a: " << a.type << "size: " << std::to_string(connections_.size()) << std::endl;
            if (a.type == SUBSCRIBE) {
                std::cout << "SUBSCRIBE" << std::endl;
                std::lock_guard<mutex> guard(connection_lock_);
                connections_.insert(a.hdl);
                std::cout << "size: " << std::to_string(connections_.size()) << std::endl;
            } else if (a.type == UNSUBSCRIBE) {
                std::cout << "UNSUBSCRIBE" << std::endl;
                std::lock_guard<mutex> guard(connection_lock_);
                connections_.erase(a.hdl);
            } else if (a.type == MESSAGE) {
                lock_guard<mutex> guard(connection_lock_);

                connection_list::iterator it;
                for (it = connections_.begin(); it != connections_.end(); ++it) {
                    server_.send(*it,a.msg);
                }
            } else {
                // undefined.
            }

        }
    }

    void queue_telegram(const dvbdump::R09GrpcTelegram* telegram) {
        std::string serialized;
        serialized.reserve(200);
        google::protobuf::util::MessageToJsonString(*telegram, &serialized);
        //std::cout << serialized << std::endl;

        {
            std::lock_guard<mutex> guard(connection_lock_);
            connection_list::iterator it;
            std::cout << "size: " << std::to_string(connections_.size()) << std::endl;
            std::size_t i = 0;
            for (it = connections_.begin(); it != connections_.end(); ++it) {
                std::cout << "sending message to: " << i << std::endl;
                server_.send(*it,serialized, websocketpp::frame::opcode::TEXT);
                i++;
            }
        }
    }


private:
    using connection_list = std::set<connection_hdl,std::owner_less<connection_hdl> >;

    server server_;
    connection_list connections_;
    std::queue<action> actions_;
    std::queue<const dvbdump::R09GrpcTelegram*> telegrams_;

    std::mutex telegram_queue_lock_;
    std::mutex action_lock_;
    std::mutex connection_lock_;
    std::condition_variable action_condition_;
};

class ReceivesTelegramsImpl final : public dvbdump::ReceivesTelegrams::Service {
    private:
        BroadcastServer websocket_server_;
        std::unique_ptr<std::thread> message_processer_ = nullptr;
        std::unique_ptr<std::thread> active_listener_ = nullptr;
    public:
        ReceivesTelegramsImpl(unsigned short websocket_port) {
            BroadcastServer websocket_server_;

            std::cout << "spawning message processor" << std::endl; 
            active_listener_ = std::make_unique<std::thread>(bind(&BroadcastServer::process_messages,&websocket_server_));

            std::cout << "spawning listening loop" << std::endl; 
            message_processer_ = std::make_unique<std::thread>([&]() { websocket_server_.run(websocket_port); });
        }

        ~ReceivesTelegramsImpl() {
            active_listener_->join();
            message_processer_->join();
        }

        grpc::Status receive_r09(grpc::ServerContext* context, const dvbdump::R09GrpcTelegram* telegram, dvbdump::ReturnCode* return_code) override {
            std::cout << "received telegram" << std::endl;
            this->websocket_server_.queue_telegram(telegram);
            return_code->set_status(0);
            return grpc::Status::OK;
        }
};

int main() {
    try {
        std::cout << "GRPC_PORT:" << std::getenv("GRPC_PORT") << std::endl;
        std::cout << "GRPC_WEBSOCKET:" << std::getenv("WEBSOCKET_PORT") << std::endl;
        unsigned short grpc_port = static_cast<unsigned short>(std::stoi(std::getenv("GRPC_PORT")));
        unsigned short websocket_port = static_cast<unsigned short>(std::stoi(std::getenv("WEBSOCKET_PORT")));

        std::string server_address("127.0.0.1:" + std::to_string(grpc_port));
        ReceivesTelegramsImpl service(websocket_port);

        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        std::cout << "Server listening on " << server_address << std::endl;
        server->Wait();

        //t2.join();
        //t1.join();

    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    }
}

