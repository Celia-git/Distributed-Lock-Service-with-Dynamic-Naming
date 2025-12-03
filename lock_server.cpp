#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <condition_variable>
#include <set>
#include <string>
#include <thread>
#include <vector>

static const int DEFAULT_PORT = 5555;
static bool g_running = true;

struct LockState {
    bool locked = false;
    int owner_fd = -1;
    std::condition_variable cv;
};

std::map<std::string, LockState> g_locks;
std::mutex g_locks_mtx;

// For cleanup: track which locks each fd owns
std::map<int, std::set<std::string>> g_fd_locks;

void handle_client(int client_fd);
bool handle_command(int client_fd, const std::string &line);
void release_all_locks_for_fd(int client_fd);

void sigint_handler(int) {
    g_running = false;
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    if (argc == 2) {
        port = std::stoi(argv[1]);
    }

    std::signal(SIGINT, sigint_handler);

    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::perror("setsockopt");
        close(server_fd);
        return 1;
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        std::perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 16) < 0) {
        std::perror("listen");
        close(server_fd);
        return 1;
    }

    std::cout << "Lock Server started on tcp://*:" << port << std::endl;

    std::vector<std::thread> threads;

    while (g_running) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(server_fd, (sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR && !g_running) break;
            std::perror("accept");
            continue;
        }

        threads.emplace_back(handle_client, client_fd);
    }

    close(server_fd);
    for (auto &t : threads) {
        if (t.joinable()) t.join();
    }
    return 0;
}

void handle_client(int client_fd) {
    std::cout << "Client connected: fd=" << client_fd << std::endl;
    std::string buffer;
    char buf[1024];

    while (true) {
        ssize_t n = ::read(client_fd, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        buffer.append(buf, n);

        std::size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);

            if (!handle_command(client_fd, line)) {
                goto done;
            }
        }
    }

done:
    release_all_locks_for_fd(client_fd);
    std::cout << "Client disconnected: fd=" << client_fd << std::endl;
    ::close(client_fd);
}

bool handle_command(int client_fd, const std::string &line) {
    std::string cmd, name;
    {
        std::size_t space = line.find(' ');
        if (space == std::string::npos) {
            cmd = line;
        } else {
            cmd = line.substr(0, space);
            name = line.substr(space + 1);
        }
    }

    if (cmd == "LOCK") {
        if (name.empty()) {
            const char *resp = "ERR\n";
            ::write(client_fd, resp, std::strlen(resp));
            return true;
        }

        std::unique_lock<std::mutex> lk(g_locks_mtx);
        LockState &ls = g_locks[name];
        while (ls.locked && ls.owner_fd != client_fd) {
            ls.cv.wait(lk);
        }
        ls.locked = true;
        ls.owner_fd = client_fd;
        g_fd_locks[client_fd].insert(name);
        lk.unlock();

        const char *resp = "OK\n";
        ::write(client_fd, resp, std::strlen(resp));
        return true;
    } else if (cmd == "UNLOCK") {
        if (name.empty()) {
            const char *resp = "ERR\n";
            ::write(client_fd, resp, std::strlen(resp));
            return true;
        }

        std::unique_lock<std::mutex> lk(g_locks_mtx);
        auto it = g_locks.find(name);
        if (it == g_locks.end() || !it->second.locked || it->second.owner_fd != client_fd) {
            const char *resp = "ERR\n";
            ::write(client_fd, resp, std::strlen(resp));
            return true;
        }

        it->second.locked = false;
        it->second.owner_fd = -1;
        g_fd_locks[client_fd].erase(name);
        it->second.cv.notify_one();
        lk.unlock();

        const char *resp = "OK\n";
        ::write(client_fd, resp, std::strlen(resp));
        return true;
    } else if (cmd == "QUIT") {
        const char *resp = "OK\n";
        ::write(client_fd, resp, std::strlen(resp));
        return false;
    } else {
        const char *resp = "ERR\n";
        ::write(client_fd, resp, std::strlen(resp));
        return true;
    }
}

void release_all_locks_for_fd(int client_fd) {
    std::unique_lock<std::mutex> lk(g_locks_mtx);
    auto it = g_fd_locks.find(client_fd);
    if (it == g_fd_locks.end()) return;

    for (const std::string &name : it->second) {
        auto lit = g_locks.find(name);
        if (lit != g_locks.end() && lit->second.locked && lit->second.owner_fd == client_fd) {
            lit->second.locked = false;
            lit->second.owner_fd = -1;
            lit->second.cv.notify_one();
        }
    }
    g_fd_locks.erase(it);
}
