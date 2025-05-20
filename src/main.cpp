#include <iostream>
#include <string>
#include <stdexcept>

/* Needs:
    - input file name
    - enable timing info output
    - enable forwarding
*/
int main (int argc, char* argv[]) {

    std::string fileName;

    // Default settings for no args
    fileName = "traces/random.txt";
    bool timeInfo = false;
    bool forward = false;

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-i") {
            // TODO: Have way to check if filepath to trace exists in directory
            fileName = argv[i+1];   // Saves filepath into fileName
            i++;                    // Skips item with filepath
        } else if (std::string(argv[i]) == "-t") {
            timeInfo = true;
        } else if (std::string(argv[i]) == "-f") {
            forward = true;
        } else {
            throw std::invalid_argument("Argument \"" + std::string(argv[i]) + "\" to program is invalid, try again.");
        }
    }


    // TODO: Maybe add a debug mode to print this?
    std::cout << "Current Settings: " << std::endl;
    std::cout << "\t Filepath:\t\t" << fileName << std::endl;
    std::cout << "\t Print Timing Info:\t" << (timeInfo ? "ENABLED" : "DISABLED") << std::endl;
    std::cout << "\t Forwarding:\t\t" << (forward ? "ENABLED" : "DISABLED") << std::endl;

    // TODO: Create Stats/MemParser and Func Simulator class instances

    return 0;
}