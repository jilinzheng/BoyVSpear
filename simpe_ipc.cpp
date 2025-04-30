#include <iostream> // For cout
#include <fstream>  // For ifstream (reading files/FIFOs)
#include <string>   // For string manipulation
#include <unistd.h> // For sleep (optional)
#include <sys/stat.h> // For checking file type (optional but good)
#include <sys/types.h>
#include <fcntl.h> // For low-level open (alternative)
#include <cerrno>   // For errno
#include <cstdio>  // For perror
#include <sstream>

// --- FIFO Configuration - MUST match Python script ---
const char* FIFO_PATH = "/tmp/joystick_fifo";
// ----------------------------------------------------

int main() {
    std::cout << "FIFO Reader started. Waiting for data from " << FIFO_PATH << "..." << std::endl;

    // --- Using C++ fstream (Recommended) ---
    std::ifstream fifo_stream;
    std::string line;

    while (true) {
        // Check if stream is open, if not, try to open it.
        if (!fifo_stream.is_open()) {
            // Optional: Check if the FIFO file exists and is a FIFO before opening
            struct stat stat_buf;
            if (stat(FIFO_PATH, &stat_buf) == 0) {
                if (!S_ISFIFO(stat_buf.st_mode)) {
                    std::cerr << "Error: " << FIFO_PATH << " exists but is not a FIFO." << std::endl;
                    sleep(5); // Wait before retrying
                    continue;
                }
            } else {
                 // File doesn't exist yet, wait for Python script to create it
                 if (errno == ENOENT) {
                    std::cout << "FIFO not found, waiting..." << std::endl;
                 } else {
                    perror("Error checking FIFO status"); // Other stat error
                 }
                 sleep(2);
                 continue;
            }

            // Open the FIFO for reading [4] [6]
            // This will block until the Python script opens it for writing [6]
            std::cout << "Attempting to open FIFO: " << FIFO_PATH << std::endl;
            fifo_stream.open(FIFO_PATH); // Opens in read mode by default

            if (!fifo_stream.is_open()) {
                std::cerr << "Error opening FIFO: " << FIFO_PATH << ". Retrying..." << std::endl;
                // perror("open"); // Can use perror if using low-level open()
                sleep(2); // Wait before retrying
                continue;
            } else {
                std::cout << "FIFO opened successfully." << std::endl;
            }
        }

        // Read line by line from the FIFO stream
        if (std::getline(fifo_stream, line)) {
            // Process the received line (X Y Button)
            std::cout << "Received: " << line << std::endl;

            // Example of parsing (optional):
            // std::stringstream ss(line);
            // int x, y, btn;
            // if (ss >> x >> y >> btn) {
            //    std::cout << "Parsed -> X: " << x << ", Y: " << y << ", Btn: " << btn << std::endl;
            // } else {
            //    std::cerr << "Warning: Could not parse line: " << line << std::endl;
            // }

        } else {
            // getline failed. This could mean the writer closed the pipe (EOF)
            // or some other error occurred.
            if (fifo_stream.eof()) {
                std::cout << "Writer closed the FIFO (EOF reached). Re-opening..." << std::endl;
            } else if (fifo_stream.fail()) {
                std::cerr << "Stream error occurred. Re-opening..." << std::endl;
            } else {
                 std::cerr << "Unknown stream state. Re-opening..." << std::endl;
            }
            fifo_stream.close();    // Close the stream
            fifo_stream.clear();    // Clear error flags
            sleep(1); // Small delay before trying to reopen
        }
    } // End while(true)

    // Should not be reached in this infinite loop example
    if (fifo_stream.is_open()) {
        fifo_stream.close();
    }
    std::cout << "FIFO Reader finished." << std::endl;
    return 0;
}
