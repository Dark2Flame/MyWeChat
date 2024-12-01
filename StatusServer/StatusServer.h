#pragma once
#include "const.h"
#include <grpcpp/grpcpp.h>
#include "StatusServerServiceImpl.h"
class StatusServer {
public:
    StatusServer(std::string address, std::shared_ptr<StatusServerServiceImpl> service)
        : server_builder_(std::make_unique<grpc::ServerBuilder>()),
        service_(std::move(service)) 
    {
        // Add listening port
        server_address_ = address;
        server_builder_->AddListeningPort(server_address_, grpc::InsecureServerCredentials());

        // Register service
        server_builder_->RegisterService(service_.get());
    }

    void Start() {
        // Start the server
        server_ = server_builder_->BuildAndStart();
        std::cout << "Server listening on " << server_address_ << std::endl;
        server_->Wait();
    }

private:
    std::unique_ptr<grpc::ServerBuilder> server_builder_;
    std::shared_ptr<StatusServerServiceImpl> service_;
    std::string server_address_;
    std::unique_ptr<grpc::Server> server_;
};

//int main(int argc, char** argv) {
//    uint16_t port = 50051;
//    std::shared_ptr<ChatServerServiceImpl> service = std::make_shared<ChatServerServiceImpl>();
//    StatusServer server(port, service);
//    server.Start();
//
//    return 0;
//}

