#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <chrono>

std::unordered_map<std::string, bool> lock_table;
std::mutex lock_mutex;

std::string handle_request(const std::string& request) {
    // Expected format: "LOCK <resource>" or "UNLOCK <resource>"
    std::istringstream iss(request);
    std::string command, resource;
    iss >> command >> resource;

    if (command == "LOCK") {
        while (true) {
            std::unique_lock<std::mutex> guard(lock_mutex);
            if (!lock_table[resource]) {
                lock_table[resource] = true;
                guard.unlock();
                return "LOCKED " + resource;
            }
            guard.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } else if (command == "UNLOCK") {
        std::lock_guard<std::mutex> guard(lock_mutex);
        lock_table[resource] = false;
        return "UNLOCKED " + resource;
    } else {
        return "ERROR: Unknown command";
    }
}

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:5555");

    std::cout << "Lock Server started on tcp://*:5555" << std::endl;

    while (true) {
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);
        std::string message(static_cast<char*>(request.data()), request.size());

        std::string reply = handle_request(message);

        zmq::message_t response(reply.size());
        memcpy(response.data(), reply.data(), reply.size());
        socket.send(response, zmq::send_flags::none);
    }
}
