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

/* MQTT server address:port */
const mqtt::string MQTT_SERVER_URI = "localhost:1883";

/* Client ID that mqtt_server uses to subscribe/publish */
const mqtt::string MQTT_CLIENT_ID = "SERVER";

/* MQTT username for secure connection */
const mqtt::string_ref user_name("omid");

/* MQTT password for secure connection */
const mqtt::buffer_ref<char> password("12345678");

/* Topics that is used in MQTT connections. Topics come in pair with odd indices 
  used for REQ category (that come from other clients to mqtt_server and ask for info) and
  even indices used for RES category (that correspond to the REQ topic just before them. Server uses
  this topics to send response to requests coming from other clients). */
const std::vector<mqtt::string> topics = {"req/temp", "res/temp",
                                         "req/numfaces", "res/numfaces",
                                         "req/load", "res/load",
                                         "req/audio", "res/audio",
                                         "req/ble", "res/ble"};

/* The file used to read CPU temperature */
const std::string CPU_TEMP_FILE = "/sys/class/thermal/thermal_zone5/temp";

/* Maximum MQTT payload */
const size_t MAX_MQTT_PAYLOAD = 500;

/* Maximum MYSQL query string length that can be generated */
const size_t MAX_MYSQL_QUERY = 500;

const std::string db_user_name = "omid";
const std::string db_password = "123456";
const std::string db_host_name = "localhost";
const std::string db_database_name = "emb";
const unsigned int db_port = 3306;

/* System camera to get pictures from. This is used by FACE DETECTOR to process the stream and 
  detect faces and by HTTP server to capture and return the picture upon get/capture-photo request */
extern cv::VideoCapture camera;
/* Make access and init on camera object thread safe */
extern std::mutex cam_mtx;

extern std::mutex db_mtx;

extern bool finish;

enum MQTT_SERV_CODES {
  SERVER_REQ_TMP = 0,
  SERVER_RES_TMP,
  SERVER_REQ_NUM_FACES,
  SERVER_RES_NUM_FACES,
  SERVER_REQ_LOAD,
  SERVER_RES_LOAD,
  SERVER_REQ_AUDIO,
  SERVER_RES_AUDIO,
  SERVER_REQ_BLE,
  SERVER_RES_BLE
};

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

/* Face derector system thread function */
void face_detector();

/* HTTP system thread function */
void http_server(int argc, char* argv[]);

/* MQTT system thread function */
void mqtt_server();

void ble_manager();

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

/**
 * Initialize camera system
 * @return 0 on success, 1 otherwise
*/
static int init_camera() {
  int ret_code = EXIT_SUCCESS;
  try {
  cam_mtx.lock();
  if(camera.isOpened()) {
    ret_code = EXIT_SUCCESS;
    goto finish;
  }
  camera.open(0);
  if(!camera.isOpened()) {
    ret_code = EXIT_FAILURE;
    goto finish;
  }
  LOG("Camera opened successfully", std::cout, "HELPER FILE")
  }
  catch(const std::exception &exc) {
    ret_code = EXIT_FAILURE;
    goto finish;
  }

  finish:
  cam_mtx.unlock();
  return ret_code;
}

/**
 * Gets the current camera frame
 * @return Current camera frame
*/
static cv::Mat get_current_frame() {
  cv::Mat tmp_frame;
  camera >> tmp_frame;
  return tmp_frame;
}

/**
 * Creates new MQTT client and connects it to server_uri
 * @param client The client connection
 * @param server_uri Server address
 * @param client_id ID that client uses to subscribe
 * @param con_opts Connection options
 * @return Returns 0 on success, 1 otherwise
*/
static int create_new_mqtt_client(mqtt::client **client, mqtt::string server_uri, mqtt::string client_id,
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
static int server_subscribe_topics(mqtt::client *client) {
  for(auto topic = topics.begin(); topic != topics.end(); topic++) {
    client->subscribe(*topic);
  }
  return EXIT_SUCCESS;
}

/**
 * Get the last number of faces stored in db
 * @param mysql_connection Active MYSQL connection
 * @return Number of faces in the last database record
*/
static void get_num_faces_from_db(MYSQL *mysql_connection, char *payload) {
  char mysql_query[MAX_MYSQL_QUERY];
  sprintf(mysql_query, "SELECT * FROM face_table ORDER BY id DESC LIMIT 1");
  CHECK_VOID(mysql_real_query(mysql_connection, mysql_query, strlen(mysql_query)),
   "Cannot perform MYSQL query for number of faces", std::cerr)
  MYSQL_RES *result = mysql_store_result(mysql_connection);
  MYSQL_ROW row;
  row = mysql_fetch_row(result);
  sprintf(payload, "%s at %s", row[2], row[1]);
}

#endif /* ifdef HELPER_H */