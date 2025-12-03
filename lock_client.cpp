#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

static const int DEFAULT_PORT = 5555;

int connect_to_server(const std::string &host, int port);
bool send_line(int fd, const std::string &line);
bool recv_line(int fd, std::string &line);
bool lock_resource(int fd, const std::string &name);
bool unlock_resource(int fd, const std::string &name);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage:\n";
        std::cerr << "  " << argv[0] << " <resource_name> READ [server_host server_port]\n";
        std::cerr << "  " << argv[0] << " <resource_name> WRITE <value> <sleep_seconds> [server_host server_port]\n";
        return 1;
    }

    std::string resource = argv[1];
    std::string mode = argv[2];

    std::string host = "127.0.0.1";
    int port = DEFAULT_PORT;

    if (mode == "READ") {
        if (argc >= 5) {
            host = argv[3];
            port = std::stoi(argv[4]);
        }
    } else if (mode == "WRITE") {
        if (argc < 5) {
            std::cerr << "WRITE requires value and sleep_seconds\n";
            return 1;
        }
        if (argc >= 7) {
            host = argv[5];
            port = std::stoi(argv[6]);
        }
    } else {
        std::cerr << "Mode must be READ or WRITE\n";
        return 1;
    }

    std::cout << "CONNECTING to lock server at tcp://" << host << ":" << port << std::endl;
    int fd = connect_to_server(host, port);
    if (fd < 0) {
        std::cerr << "Failed to connect to server\n";
        return 1;
    }

    std::cout << "REQUESTING lock for resource: " << resource << std::endl;
    if (!lock_resource(fd, resource)) {
        std::cerr << "Failed to acquire lock\n";
        ::close(fd);
        return 1;
    }
    std::cout << "LOCKED " << resource << std::endl;

    std::string filename = resource + ".txt";

    if (mode == "WRITE") {
        std::string value = argv[3];
        int sleep_seconds = std::stoi(argv[4]);

        std::cout << "Sleeping for " << sleep_seconds << " seconds before WRITE..." << std::endl;
        ::sleep(sleep_seconds);

        std::cout << "WRITING value to " << resource << ": " << value << std::endl;
        std::ofstream ofs(filename);
        ofs << value;
        ofs.close();
    } else {
        std::ifstream ifs(filename);
        std::string content;
        if (ifs) {
            std::getline(ifs, content);
        }
        std::cout << "READING value from " << resource << ": " << content << std::endl;
    }

    std::cout << "RELEASING lock for resource: " << resource << std::endl;
    if (!unlock_resource(fd, resource)) {
        std::cerr << "Failed to release lock\n";
    } else {
        std::cout << "UNLOCKED " << resource << std::endl;
    }

    send_line(fd, "QUIT\n");
    ::close(fd);
    return 0;
}

int connect_to_server(const std::string &host, int port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::perror("socket");
        return -1;
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        std::perror("inet_pton");
        ::close(fd);
        return -1;
    }

    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        std::perror("connect");
        ::close(fd);
        return -1;
    }

    return fd;
}

bool send_line(int fd, const std::string &line) {
    const char *data = line.c_str();
    std::size_t len = line.size();
    std::size_t sent = 0;

    while (sent < len) {
        ssize_t n = ::write(fd, data + sent, len - sent);
        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

bool recv_line(int fd, std::string &line) {
    line.clear();
    char ch;
    while (true) {
        ssize_t n = ::read(fd, &ch, 1);
        if (n <= 0) return false;
        if (ch == '\n') break;
        line.push_back(ch);
    }
    return true;
}

bool lock_resource(int fd, const std::string &name) {
    std::string cmd = "LOCK " + name + "\n";
    if (!send_line(fd, cmd)) return false;
    std::string resp;
    if (!recv_line(fd, resp)) return false;
    return resp == "OK";
}

bool unlock_resource(int fd, const std::string &name) {
    std::string cmd = "UNLOCK " + name + "\n";
    if (!send_line(fd, cmd)) return false;
    std::string resp;
    if (!recv_line(fd, resp)) return false;
    return resp == "OK";
}
