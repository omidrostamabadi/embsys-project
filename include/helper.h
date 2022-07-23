#include <mysql/mysql.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <ctime>

/**
 * Print error message to file with additional info on the name of the file and the line that error takes place
 * @param call A call to a function to check its return value
 * @param msg Error message to print to file
 * @param file A file to send output to
*/
#define CHECK(call, msg, file) if(call) {file << "Error in " << __FILE__ << ":" << __LINE__ << ": " << msg << std::endl; return EXIT_FAILURE;}

/**
 * Write a log message to a file with timestamp and the program writing the log
 * @param msg The message that will be logged
 * @param file The file to write the log to
 * @param program The name of the program writing the log
*/
#define LOG(msg, file, program) file << ctime(time(0)) << "(" << program << "): " << msg << std::endl;

/**
 * Connect to MySQL database with specified parameteres
 * @param mysql_connection Represents the established connection if connection succeeds
 * @param user Username for the database
 * @param password Password for user
 * @param host_name Hostname where MySQL server resides
 * @param database_name The name of the database to connect
 * @return Returns 0 on success, 1 otherwise
*/
int connect_to_db(MYSQL *mysql_connection, std::string user, std::string password, std::string host_name,
 std::string database_name) {
  MYSQL *mysql_connection_ret = NULL;
  mysql_connection_ret = mysql_real_connect(mysql_connection, host_name.c_str(), user.c_str(), password.c_str(),
   database_name.c_str(), 0, NULL, 0);
  if(mysql_connection_ret == NULL) {
    std::cout << "Connection to database failed\n";
    std::cout << "Database input params:\n";
    std::cout << "Username: " << user << " Password: " << password << " Hostname: " <<
     host_name << " Database Name: " << database_name << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}