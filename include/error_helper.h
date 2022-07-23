#include <unistd.h>
#include <string>
#include <iostream>

/**
 * @param call A call to a function to check its return value
 * @param msg Error message to print to file
 * @param file A file to send output to
*/
#define CHECK(call, msg, file) if(call) {file << "Error in " << __FILE__ << ":" << __LINE__ << ": " << msg << std::endl; return EXIT_FAILURE;}