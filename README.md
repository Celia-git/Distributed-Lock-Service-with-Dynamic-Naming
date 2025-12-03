# Distributed Lock Service (Berkeley Sockets)

This project implements a simple distributed lock service using TCP and Berkeley sockets in C++. A central lock server manages named locks, and multiple clients coordinate access to shared resources via these names.

## Build

Requirements:
- g++ with C++17 support
- POSIX sockets (Linux/macOS/BSD)

To build everything:

make


This produces two binaries:

- `lock_server`
- `lock_client`

To clean build artifacts:

make clean


## Running the Server

Start the lock server (default example port: 5555):

./lock_server 5555

You should see output similar to:

Lock Server started on tcp://*:5555


Leave this running while you start clients.

## Client Usage

General client syntax:

./lock_client <resource_name> READ [server_host server_port]
./lock_client <resource_name> WRITE <value> <sleep_seconds> [server_host server_port]


Defaults:
- `server_host` defaults to `127.0.0.1`
- `server_port` defaults to `5555`

### WRITE example

Terminal 1 (server):

./lock_server 5555


Terminal 2 (writer):

./lock_client file_A WRITE "Hello Distributed World" 5


Behavior:
- Connects to the server.
- Requests a lock on `file_A`.
- Sleeps 5 seconds while holding the lock.
- Writes `Hello Distributed World` to `file_A.txt`.
- Releases the lock.

### READ example

Terminal 3 (reader):

./lock_client file_A READ


Behavior:
- Connects to the server.
- Requests a lock on `file_A`.
- Blocks until the writer releases the lock (if necessary).
- Reads the value from `file_A.txt`.
- Releases the lock.

### Using a different server host/port

If the server runs on another host or port:

./lock_client file_A WRITE "Some value" 2 myserver.example.com 6000
./lock_client file_A READ myserver.example.com 6000


## Convenience `run` Target

You can use the `run` target to start the server and see example commands:

make run


This:
- Builds the binaries (if needed).
- Starts `lock_server` on port `5555`.
- Prints example `lock_client` commands for READ and WRITE interactions.

