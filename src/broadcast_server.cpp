
#ifndef FUNNEL_BROADCAST_SERVER_CPP
#define FUNNEL_BROADCAST_SERVER_CPP

#include "./broadcast_server.hpp"

BroadcastServer::BroadcastServer () noexcept {
    // setting the state of the broadcast_server to active
    kill_.store(false);

    // Initialize Asio Transport
    server_.init_asio();

    // Register handler callbacks
    server_.set_open_handler(bind(&BroadcastServer::on_open,this,::_1));
    server_.set_close_handler(bind(&BroadcastServer::on_close,this,::_1));
    server_.set_message_handler(bind(&BroadcastServer::on_message,this,::_1,::_2));
}

BroadcastServer::~BroadcastServer() noexcept {
    server_.reset();
}

void BroadcastServer::run(uint16_t port) noexcept {
    server_.listen(port);
    server_.start_accept();

    try {
        server_.run();
    } catch (const std::exception & e) {
        std::cout << e.what() << std::endl;
    }
}

void BroadcastServer::on_open(connection_hdl hdl) noexcept {
    {
        lock_guard<mutex> guard(action_lock_);
        std::cout << "new connection" << std::endl;
        actions_.push(action(SUBSCRIBE,hdl));
    }
    action_condition_.notify_one();
}

void BroadcastServer::on_close(connection_hdl hdl) noexcept {
    {
        lock_guard<mutex> guard(action_lock_);
        std::cout << "closing connection" << std::endl;
        actions_.push(action(UNSUBSCRIBE,hdl));
    }
    action_condition_.notify_one();
}

void BroadcastServer::on_message(connection_hdl hdl, server::message_ptr msg) noexcept {
    // queue message up for sending by processing thread
    {
        lock_guard<mutex> guard(action_lock_);
        std::cout << "message from user" << std::endl;
        actions_.push(action(MESSAGE,hdl,msg));
    }
    action_condition_.notify_one();
}

void BroadcastServer::process_messages() noexcept {
    while(not kill_) {
        std::unique_lock<mutex> lock(action_lock_);

        while(actions_.empty()) {
            action_condition_.wait(lock);
        }

        action a = actions_.front();
        actions_.pop();

        lock.unlock();

        if (a.type == SUBSCRIBE) {
            std::cout << "SUBSCRIBE" << std::endl;
            std::lock_guard<mutex> guard(connection_lock_);
            connections_.insert(a.hdl);
        } else if (a.type == UNSUBSCRIBE) {
            std::cout << "UNSUBSCRIBE" << std::endl;
            std::lock_guard<mutex> guard(connection_lock_);
            connections_.erase(a.hdl);
        } else if (a.type == MESSAGE) {
            // set filter here
        } else {
            // undefined.
        }

    }
}

void BroadcastServer::queue_telegram(const dvbdump::R09GrpcTelegram* telegram) noexcept {

    // serialize the protobuf struct into json string
    std::string serialized;
    serialized.reserve(200);
    google::protobuf::util::MessageToJsonString(*telegram, &serialized);

    // lock connection list and yeet the telegram to all peers
    {
        std::lock_guard<mutex> guard(connection_lock_);
        connection_list::iterator it;
        std::size_t i = 0;
        for (it = connections_.begin(); it != connections_.end(); ++it) {
            server_.send(*it,serialized, websocketpp::frame::opcode::TEXT);
            i++;
        }
    }
}

void BroadcastServer::kill() noexcept {
    kill_.store(true);
}


#endif //FUNNEL_BROADCAST_SERVER_CPP
