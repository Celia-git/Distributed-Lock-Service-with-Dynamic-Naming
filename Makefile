CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LDFLAGS = -lzmq -pthread

TARGETS = lock_server lock_client

all: $(TARGETS)

lock_server: lock_server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

lock_client: lock_client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

run_server:
	./lock_server

run_client:
	./lock_client


run: run_server

clean:
	rm -f $(TARGETS)

