## Word Grid Puzzle Server
## Overview

The Word Grid Puzzle Server is a collaborative word puzzle creation application. It provides a server-client architecture where multiple clients can interact with a shared grid to read and write words. The server manages the grid using Boost's interprocess synchronization tools, enabling concurrent operations.

## Features

- Multi-threaded server handling multiple client connections simultaneously.
- Custom protocol for clients to read and write words on the shared grid.
- Efficient grid manipulation using Boost's interprocess synchronization tools.
- Logging of grid state to a text file after a predefined number of player connections.
- Easy-to-use command-line interface for specifying the server port and filename.

## Prerequisites

- C++ compiler with support for C++11.
- Boost C++ libraries installed.
- pthread library for multi-threading.

## Usage

1. **Compile the Server:**
   ```bash
   make OR
   g++ -o server server.cpp -lboost_system -lboost_thread -pthread -lboost_iostreams
2. **Run the Server:**
   ```bash
   ./server <port> <filename>