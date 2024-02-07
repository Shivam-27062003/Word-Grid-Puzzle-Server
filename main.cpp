// Import necessary libraries
#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <queue>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/thread.hpp>
#include "gridManager.hpp"

// Maximum number of connections/Players allowed in game
const uint64_t MAX_CONNECTIONS = 3; 

// Variable to keep track of the number of players
static unsigned Players = 0; 

// Function to write data to a file from a thread
void* writeToFile(void* args) {
    auto params = reinterpret_cast<std::tuple<int, std::string, char*>*>(args);
    std::string filename = std::get<1>(*params);
    signed lineNumber = std::get<0>(*params);
    char* grid = std::get<2>(*params);
    std::ofstream outFile(filename, std::ios::app);
    if (outFile.is_open()) {
        // Move the file pointer to the specified line
        outFile.seekp(lineNumber * GRID_SIZE, std::ios::beg);
        std::string line = "";
        for (unsigned i = 0; i < GRID_SIZE; i++) {
            char value = grid[lineNumber * GRID_SIZE + i];
            if (value != '\0') {
                // Format the data and write to the file
                line += '(' + std::to_string(lineNumber) + ',' + std::to_string(i) + ')';
                line += ':';
                line += value;
                line += " ";
            }
        }
        outFile << line << '\n';
        outFile.close();
    } else {
        std::cerr << "Error opening file: " << filename << "\n";
    }
    pthread_exit(nullptr);
}

// Class to handle incoming messages from clients
class MessageHandler {
public:
    MessageHandler(GridManager& gridManager, int clientSocket, std::string File)
        : gridManager(gridManager), clientSocket(clientSocket), filename(File) {}

    // Handle incoming messages from the client
    void HandleMessages() {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        while (true) {
            // Receive data from the client
            ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0) {
                break;
            }
            std::string receivedData(buffer);
            ProcessMessages(receivedData);
        }
        Players++;

        // If the maximum number of connections is reached, initiate loggers and reset player count
        if (Players == MAX_CONNECTIONS) {
            initiate_loggers(gridManager.grid, filename);
            gridManager.renew();
            Players = 0;
        }
        close(clientSocket);
    }

private:
    GridManager& gridManager;
    int clientSocket;
    std::string filename;

    // Process received messages
    void ProcessMessages(std::string& receivedData) {
        size_t delimiterPos = receivedData.find("END");
        while (delimiterPos != std::string::npos) {
            std::string currentMessage = receivedData.substr(0, delimiterPos);
            std::istringstream iss(currentMessage);
            std::vector<std::string> tokens;
            std::string token;
            while (std::getline(iss, token, ',')) {
                tokens.push_back(token);
            }
            if (tokens.size() >= 4) {
                if (tokens[0] == "WRITE" && tokens.size() > 5) {
                    gridManager.Write(tokens);
                } else if (tokens[0] == "READ") {
                    std::string word = gridManager.Read(tokens);
                    const char* response = word.c_str();
                    // Send the response back to the client
                    send(clientSocket, response, strlen(response), 0);
                }
            }
            receivedData = receivedData.substr(delimiterPos + 3);
            delimiterPos = receivedData.find("END");
        }
    }

    // Initiate logger threads for writing to the file in parallel
    void initiate_loggers(char* grid, std::string& filename) {
        pthread_t threads[MAX_CONNECTIONS];
        // Launch threads
        std::ofstream outFile(filename, std::ios::out | std::ios::trunc);
        outFile.close();
        for (unsigned long i = 0; i < GRID_SIZE; i++) {
            std::tuple<signed, std::string, char*> params = std::make_tuple(i, filename, grid);
            int result = pthread_create(&threads[i % MAX_CONNECTIONS], nullptr, &writeToFile, &params);
            if (result != 0) {
                std::cerr << "Error creating thread " << i + 1 << std::endl;
                return;
            }
        }
        // Wait for all threads to finish
        for (int j = 0; j < MAX_CONNECTIONS; j++) {
            pthread_join(threads[j], nullptr);
        }
    }
};

// Class to handle the server functionality
class Server {
public:
    Server(int port, std::string Filename)
        : port(port), gridManager(), socket(createAndBindSocket()), filename(Filename) {}

    ~Server() {
        // Close the server socket
        close(socket);
    }

    // Start the server and accept incoming connections
    void Start() {
        while (true) {
            AcceptConnection();
        }
    }

private:
    int port;
    int socket;
    GridManager gridManager;
    std::string filename;

    // Create and bind the server socket
    int createAndBindSocket() {
        int serverSocket = ::socket(AF_INET, SOCK_STREAM, 0);

        if (serverSocket == -1) {
            std::cerr << "Error creating socket." << std::endl;
            exit(EXIT_FAILURE);
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            std::cerr << "Error binding socket." << std::endl;
            close(serverSocket);
            exit(EXIT_FAILURE);
        }

        if (listen(serverSocket, 5) == -1) {
            std::cerr << "Error listening on socket." << std::endl;
            close(serverSocket);
            exit(EXIT_FAILURE);
        }

        std::cout << "Server listening on port " << port << std::endl;

        return serverSocket;
    }

    // Accept incoming client connections
    void AcceptConnection() {
        sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        int clientSocket = accept(socket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == -1) {
            std::cerr << "Error accepting connection." << std::endl;
            close(socket);
            exit(EXIT_FAILURE);
        }

        std::cout << "Connection accepted from " << inet_ntoa(clientAddr.sin_addr) \
		<< ":" << ntohs(clientAddr.sin_port) << std::endl;

        // Create a new thread to handle the client
        pthread_t clientThread;
        MessageHandler messageHandler(gridManager, clientSocket, filename);
        if (pthread_create(&clientThread, nullptr, &Server::HandleClient, &messageHandler) != 0) {
            std::cerr << "Error creating thread." << std::endl;
            close(socket);
            exit(EXIT_FAILURE);
        }

        // Detach the thread (allow it to run independently)
        pthread_detach(clientThread);
    }

    // Static method to handle the client thread
    static void* HandleClient(void* arg) {
        auto messageHandler = reinterpret_cast<MessageHandler*>(arg);
        messageHandler->HandleMessages();
        return nullptr;
    }
};

// Main function
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <port> and <filename>" << std::endl;
        return EXIT_FAILURE;
    }

    // Get the port and filename from command line arguments
    const short port = std::stoi(argv[1]);
    std::string filename = argv[2];

    // Create an instance of the server with the specified port
    Server server(port, filename);

    // Start the server
    server.Start();

    return 0;
}