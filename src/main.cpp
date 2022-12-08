//#define ASIO_STANDALONE
//#define _WEBSOCKETPP_CPP11_TYPE_TRAITS_

#include "receives_telegrams.hpp"

#include <cstdlib>
#include <string>
#include <thread>
#include <iostream>

int main() {
    try {
        std::cout << "GRPC_PORT:" << std::getenv("GRPC_PORT") << std::endl;
        std::cout << "WEBSOCKET_PORT:" << std::getenv("WEBSOCKET_PORT") << std::endl;
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

    } catch (websocketpp::exception const &e) {
        std::cout << e.what() << std::endl;
    }
}

