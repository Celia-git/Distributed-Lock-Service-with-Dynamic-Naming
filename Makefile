CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pthread
TARGETS  := lock_server lock_client

all: $(TARGETS)

lock_server: lock_server.cpp
	$(CXX) $(CXXFLAGS) -o lock_server lock_server.cpp

lock_client: lock_client.cpp
	$(CXX) $(CXXFLAGS) -o lock_client lock_client.cpp

run: all
	@echo "Starting lock_server on port 5555..."
	@./lock_server 5555 & \
	SERVER_PID=$$!; \
	echo "Server PID: $$SERVER_PID"; \
	echo ""; \
	echo "Example writer client:"; \
	echo "  ./lock_client file_A WRITE \"Hello Distributed World\" 5"; \
	echo "Example reader client:"; \
	echo "  ./lock_client file_A READ"; \
	wait $$SERVER_PID

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean run
