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

const mqtt::string MQTT_SERVER_URI = "localhost:1883";
const mqtt::string MQTT_CLIENT_ID = "SERVER";
const mqtt::string_ref user_name("omid"); // MQTT username, not DB
const mqtt::buffer_ref<char> password("12345678"); // MQTT pass, not DB
const std::vector<mqtt::string> topics = {"req/temp", "res/temp", "req/numfaces", "res/numfaces"};
const std::string CPU_TEMP_FILE = "/sys/class/thermal/thermal_zone5/temp";
const size_t MAX_MQTT_PAYLOAD = 500;
const size_t MAX_MYSQL_QUERY = 500;

enum MQTT_SERV_CODES {
  SERVER_REQ_TMP = 0,
  SERVER_RES_TMP,
  SERVER_REQ_NUM_FACES,
  SERVER_RES_NUM_FACES
};

char *remove_last_char(char *in) {
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

/* Used for temporarily save the result of time(0) */
time_t TMP_TIMER_VAR;

/**
 * Write a log message to a file with timestamp and the program writing the log
 * @param msg The message that will be logged
 * @param file The file to write the log to
 * @param program The name of the program writing the log
*/
#define LOG(msg, file, program) TMP_TIMER_VAR = time(0); file << "(" << program << "): " << msg << " [" << remove_last_char(ctime(&TMP_TIMER_VAR)) << "]" << std::endl;

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

/**
 * Creates new MQTT client and connects it to server_uri
 * @param client The client connection
 * @param server_uri Server address
 * @param client_id ID that client uses to subscribe
 * @param con_opts Connection options
 * @return Returns 0 on success, 1 otherwise
*/
int create_new_mqtt_client(mqtt::client **client, mqtt::string server_uri, mqtt::string client_id,
 mqtt::connect_options *con_opts) {
  CHECK(!(*client = new mqtt::client(server_uri, client_id)), "Could not create MQTT client", std::cerr)
  (*client)->connect(*con_opts).get_server_uri().data();
  return EXIT_SUCCESS;
}

/**
 * Registers the client on all the topics defined in topics vector defined above
 * @param client The client that subscribes to topics
 * @return 0 on success, 1 otherwise
*/
int server_subscribe_topics(mqtt::client *client) {
  for(auto topic = topics.begin(); topic != topics.end(); topic++) {
    client->subscribe(*topic);
  }
  return EXIT_SUCCESS;
}

/**
 * Get the CPU temperature 
 * @return Current CPU temperature [C]
*/
double get_cpu_temp() {
  std::ifstream temp_file;
  temp_file.open(CPU_TEMP_FILE, std::ios::in);
  CHECK(!temp_file.is_open(), "Cannot open CPU temperature file", std::cerr)
  int temp;
  temp_file >> temp;
  temp_file.close();
  return (double) temp / 1000;
}

double get_cpu_temp_from_db(MYSQL *mysql_connection) {
  char mysql_query[MAX_MYSQL_QUERY];
  sprintf(mysql_query, "SELECT temp FROM temp_table ORDER BY id DESC LIMIT 1");
  CHECK(mysql_real_query(mysql_connection, mysql_query, strlen(mysql_query)),
   "Cannot perform MYSQL query for number of faces", std::cerr)
  MYSQL_RES *result = mysql_store_result(mysql_connection);
  MYSQL_ROW row;
  row = mysql_fetch_row(result);
  double temp = atof(row[0]);
  return temp;
}

int get_num_faces_from_db(MYSQL *mysql_connection) {
  char mysql_query[MAX_MYSQL_QUERY];
  sprintf(mysql_query, "SELECT num_faces FROM face_table ORDER BY id DESC LIMIT 1");
  CHECK(mysql_real_query(mysql_connection, mysql_query, strlen(mysql_query)),
   "Cannot perform MYSQL query for number of faces", std::cerr)
  MYSQL_RES *result = mysql_store_result(mysql_connection);
  MYSQL_ROW row;
  row = mysql_fetch_row(result);
  int num_faces = atoi(row[0]);
  return num_faces;
}