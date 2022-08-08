#ifndef HELPER_H
#define HELPER_H

#include <mysql/mysql.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <ctime>
#include <fstream>
#include <vector>
#include <iterator>
#include <mqtt/client.h>
#include <mqtt/connect_options.h>
#include <mqtt/types.h>
#include <mqtt/topic.h>
#include <mqtt/iclient_persistence.h>
#include <mqtt/properties.h>
#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <mutex>


/* Maximum MYSQL query string length that can be generated */
const size_t MAX_MYSQL_QUERY = 500;

const std::string db_user_name = "omid";
const std::string db_password = "123456";
const std::string db_host_name = "localhost";
const std::string db_database_name = "emb";
const unsigned int db_port = 3306;

/**
 * Removes the last character from the string if it's a new line ('\n') character
 * @param in Input string in c str format
 * @return Returns the modified string
*/
static char *remove_last_new_line(char *in) {
  int last_index = strlen(in) - 1;
  if(in[last_index] == '\n')
    in[last_index] = '\0';
  return in;
}

/**
 * Print error message to file with additional info on the name of the file and the line that error takes place
 * @param call A call to a function to check its return value
 * @param msg Error message to print to file
 * @param file A file to send output to
*/
#define CHECK(call, msg, file) if(call) {file << "Error in " << __FILE__ << ":" << __LINE__ << ": " << msg << std::endl; return EXIT_FAILURE;}

/**
 * This does the exact same job as CHECK except that it does not returns anything, 
 *  so it can be used in void functions
*/
#define CHECK_VOID(call, msg, file) if(call) {file << "Error in " << __FILE__ << ":" << __LINE__ << ": " << msg << std::endl; return;}

/* Used for temporarily save the result of time(0) */
static time_t TMP_TIMER_VAR;

/**
 * Write a log message to a file with timestamp and the program writing the log
 * @param msg The message that will be logged
 * @param file The file to write the log to
 * @param program The name of the program writing the log
*/
#define LOG(msg, file, program) TMP_TIMER_VAR = time(0); file << "(" << program << "): " << msg << " [" << remove_last_new_line(ctime(&TMP_TIMER_VAR)) << "]" << std::endl;

#endif /* ifdef HELPER_H */