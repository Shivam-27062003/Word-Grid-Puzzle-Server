// Include guards to prevent multiple inclusion of the header file
#ifndef GRIDMANAGER
#define GRIDMANAGER

// Include necessary libraries
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

// Define the size of the grid
const uint64_t GRID_SIZE = 30;

// Class to manage the grid operations
class GridManager {
public:
    // Constructor
    GridManager() {
        // Initialize the mapped file parameters
        params.path = "matrix.dat";
        params.new_file_size = sizeof(char) * GRID_SIZE * GRID_SIZE;
        // Open the mapped file
        file.open(params);
        // Map the file content to a char pointer
        grid = reinterpret_cast<char*>(file.data());
    }

    // Destructor
    ~GridManager() {
        // Close the mapped file
        file.close();
    }

    // Reset the grid and open a new mapped file
    void renew() {
        file.close();
        params.path = "matrix.dat";
        params.new_file_size = sizeof(char) * GRID_SIZE * GRID_SIZE;
        // Open the mapped file
        file.open(params);
        // Map the file content to a char pointer
        grid = reinterpret_cast<char*>(file.data());
    }

    // Write operation on the grid
    void Write(std::vector<std::string>& task) {
        unsigned x = std::stoi(task[2]), y = std::stoi(task[3]);
        std::string direction = task[4];
        std::string word = task[5];
        unsigned len = word.size();
        {
            // Use unique_lock to handle mutex automatically
            std::unique_lock<boost::interprocess::interprocess_mutex> lock(mutex);
            if (direction == "+X") {
                for (unsigned i = 0; i < len; i++) {
                    grid[x * GRID_SIZE + y + i] = word[i];
                }
            } else if (direction == "-X") {
                for (unsigned i = 0; i < len; i++) {
                    grid[x * GRID_SIZE + y - i] = word[i];
                }
            } else if (direction == "+Y") {
                for (unsigned i = 0; i < len; i++) {
                    grid[(x + i) * GRID_SIZE + y] = word[i];
                }
            } else if (direction == "-Y") {
                for (unsigned i = 0; i < len; i++) {
                    grid[(x - i) * GRID_SIZE + y] = word[i];
                }
            }
        }
    }

    // Read operation on the grid
    std::string Read(std::vector<std::string>& task) {
        unsigned x = std::stoi(task[2]), y = std::stoi(task[3]);
        std::string direction = task[4];
        std::string word = "";
        {
            // Use unique_lock to handle mutex automatically
            std::unique_lock<boost::interprocess::interprocess_mutex> lock(mutex);
            if (direction == "+X") {
                for (unsigned i = 0;; i++) {
                    if (grid[x * GRID_SIZE + y + i] == '\0') {
                        break;
                    }
                    word += grid[x * GRID_SIZE + y + i];
                }
            } else if (direction == "-X") {
                for (unsigned i = 0;; i++) {
                    if (grid[x * GRID_SIZE + y - i] == '\0') {
                        break;
                    }
                    word += grid[x * GRID_SIZE + y - i];
                }
            } else if (direction == "+Y") {
                for (unsigned i = 0;; i++) {
                    if (grid[(x + i) * GRID_SIZE + y] == '\0') {
                        break;
                    }
                    word += grid[(x + i) * GRID_SIZE + y];
                }
            } else if (direction == "-Y") {
                for (unsigned i = 0;; i++) {
                    if (grid[(x - i) * GRID_SIZE + y] == '\0') {
                        break;
                    }
                    word += grid[(x - i) * GRID_SIZE + y];
                }
            }
            // Other directions handled similarly
        }
        return word;
    }

    // Pointer to the grid data
    char* grid;

private:
    boost::iostreams::mapped_file_params params;
    boost::iostreams::mapped_file_sink file;
    boost::interprocess::interprocess_mutex mutex;
};

#endif
