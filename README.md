# Distributed Lock Service with Dynamic Naming

## Overview
This project implements a simple Distributed Lock Service in C++ using ZeroMQ and mutexes.  
The system consists of a centralized Lock Server and multiple Clients that request and release locks on dynamically named resources (for example, `file_A`, `db_connection`).  
It demonstrates mutual exclusion across distributed clients through message-based coordination.

---

## How to Build

### Requirements
- C++17 or higher
- ZeroMQ (`libzmq3-dev`)
- g++ compiler

### Install Dependencies (Ubuntu Example)
```
sudo apt update
sudo apt install libzmq3-dev g++
```

### Build Commands
```
g++ -std=c++17 lock_server.cpp -o lock_server -lzmq -pthread
g++ -std=c++17 lock_client.cpp -o lock_client -lzmq -pthread
```

This will produce two executables:
- `lock_server` (the central coordinator)
- `lock_client` (the program clients use to request locks)

---

## How to Run the System

### Step 1: Start the Lock Server
Run the lock server in one terminal window:
```
./lock_server
```
Expected output:
```
Lock Server started on tcp://*:5555
```

### Step 2: Start One or More Clients
Open additional terminals and start clients that connect to the server at `tcp://localhost:5555`.

**Command Format:**
```
./lock_client <resource_name> <operation> [value] [sleep_seconds]
```

**Where:**
- `<resource_name>`: The name of the shared resource (example: file_A)
- `<operation>`: Either READ or WRITE
- `[value]`: Data to write (only needed for WRITE)
- `[sleep_seconds]`: Optional; how long to hold the lock (default is 2 seconds)

---

## Example Usage

### Example 1: Basic Write Operation
**Terminal 1 (Server):**
```
./lock_server
```
Output:
```
Lock Server started on tcp://*:5555
```

**Terminal 2 (Client 1 - Writer):**
```
./lock_client file_A WRITE "Hello Distributed World" 5
```
Output:
```
REQUESTING lock for resource: file_A
LOCKED file_A
Sleeping for 5 seconds...
WROTE 'Hello Distributed World' to file_A
UNLOCKED file_A
```

---

### Example 2: Read and Write with Multiple Clients

**Terminal 1 (Server):**
```
./lock_server
```

**Terminal 2 (Client 1 - Writer):**
```
./lock_client file_A WRITE "Shared Resource Test" 5
```

**Terminal 3 (Client 2 - Reader, started while Client 1 holds the lock):**
```
./lock_client file_A READ
```

**Illustrated Output Sequence:**

**Client 1:**
```
REQUESTING lock for resource: file_A
LOCKED file_A
Sleeping for 5 seconds...
WROTE 'Shared Resource Test' to file_A
UNLOCKED file_A
```

**Client 2:**
```
REQUESTING lock for resource: file_A
(waiting until Client 1 releases lock)
LOCKED file_A
READ from file_A: Shared Resource Test
UNLOCKED file_A
```

This behavior demonstrates proper synchronization — the second client waits until the first releases the lock before accessing the same resource.

---

## Summary

- The Lock Server manages all named resource locks centrally.
- Clients dynamically request and release locks for any resource name.
- Only one client can hold a lock on a given resource at a time.
- Communication uses the ZeroMQ REQ/REP pattern for reliability.
- Mutexes ensure thread-safe access to the Lock Server’s internal lock state.

---

## File List
| File | Description |
|------|--------------|
| `lock_server.cpp` | Implements the central lock server that maintains lock state. |
| `lock_client.cpp` | Implements the client that requests and releases resource locks. |
| `README.md` | Instructions for building and running the project. |

---

## Example Cleanup
After testing, you can remove generated resource files (like `file_A.txt`) with:
```
rm *.txt
```

---

***

Would you like the README to include a short troubleshooting section for common ZeroMQ runtime errors (for example, “Address already in use” or “Connection refused”)?
