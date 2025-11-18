#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>

void send_request(zmq::socket_t& socket, const std::string& request) {
    zmq::message_t message(request.size());
    memcpy(message.data(), request.data(), request.size());
    socket.send(message, zmq::send_flags::none);
}

std::string recv_reply(zmq::socket_t& socket) {
    zmq::message_t reply;
    auto result = socket.recv(reply, zmq::recv_flags::none);
    if (!result) {
	std::cerr << "Failed to receive reply from server" << std::endl;
	return "";
    }
    
    return std::string(static_cast<char*>(reply.data()), reply.size());
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: ./lock_client <resource> <operation> [value] [sleep_seconds]" << std::endl;
        return 1;
    }

    std::string resource = argv[1];
    std::string operation = argv[2];
    std::string value = argc >= 4 ? argv[3] : "";
    int sleep_time = argc >= 5 ? std::stoi(argv[4]) : 2;

    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::req);
    socket.connect("tcp://localhost:5555");

    // Request LOCK
    std::cout << "REQUESTING lock for resource: " << resource << std::endl;
    send_request(socket, "LOCK " + resource);
    std::cout << recv_reply(socket) << std::endl;

    // Simulate operations
    if (operation == "WRITE") {
        std::cout << "Sleeping for " << sleep_time << " seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
        std::ofstream file(resource + ".txt");
        file << value;
        file.close();
        std::cout << "WROTE '" << value << "' to " << resource << std::endl;
    } else if (operation == "READ") {
        std::ifstream file(resource + ".txt");
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::cout << "READ from " << resource << ": " << content << std::endl;
    }

    // Release LOCK
    send_request(socket, "UNLOCK " + resource);
    std::cout << recv_reply(socket) << std::endl;

    return 0;
}
