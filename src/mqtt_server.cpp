#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <ctime>
#include <stdlib.h>
#include <fstream>
#include <unistd.h>
#include <iterator>
#include <mqtt/callback.h>
#include <mqtt/client.h>
#include <mqtt/connect_options.h>
#include <mqtt/types.h>
#include <mqtt/topic.h>
#include <mqtt/iclient_persistence.h>
#include <mqtt/properties.h>
#include "MQTTClient.h"
#include <mysql/mysql.h>
#include <helper.h>

static std::string cpu_load_file_name = "load.txt";
std::fstream cpu_load_file;
static double get_cpu_load();
static double get_audio_energy_from_db(MYSQL *mysql_connection);
static double get_ble_from_db(MYSQL *mysql_connection);
static double get_cpu_temp();

void mqtt_server() {
  int status = system("touch load.txt");
  CHECK_VOID(status, "Cannot touch load.txt file", std::cerr)
  // cpu_load_file.open(cpu_load_file_name, std::ios::out | std::ios::in);
  // CHECK_VOID(!cpu_load_file.is_open(), "Cannot open cpu load file", std::cerr)

  /* Init MQTT */
  mqtt::client *client;
  mqtt::connect_options connOpts;
  connOpts.set_clean_session(false);
  connOpts.set_user_name(user_name);
  connOpts.set_password(password);
  CHECK_VOID(create_new_mqtt_client(&client, MQTT_SERVER_URI, MQTT_CLIENT_ID, &connOpts),
   "Could not create MQTT client", std::cerr)

  /* Subscribe to some topics */
  server_subscribe_topics(client);

  /* Connect to database */
  MYSQL *mysql_connection = mysql_init(NULL);
  // std::string user = "omid";
  // std::string password = "123456";
  // std::string host_name = "localhost";
  // std::string database_name = "emb";
  // uint32_t port = 3306;
  // CHECK_VOID(connect_to_db(mysql_connection, user, password, host_name, database_name),
  //  "Cannot connect to database", std::cerr)
  MYSQL *mysql_connection_ret = NULL;
  mysql_connection_ret = mysql_real_connect(mysql_connection, db_host_name.c_str(), db_user_name.c_str(),
   db_password.c_str(), db_database_name.c_str(), db_port, NULL, 0);
  if(mysql_connection_ret == NULL) {
    std::cerr << __FILE__ << ":" << __LINE__ << ": Connection to db failed\n";
    return;
  }

  while(!finish) {
    usleep(500 * 1000);
    auto msg = client->consume_message();
    if (msg) {
      /* Handle temperature request */
      if(msg->get_topic() == topics[SERVER_REQ_TMP]) {
        LOG("Got temperature request", std::cout, "MQTT SERVER")
        double cpu_temp = get_cpu_temp();
        char payload[MAX_MQTT_PAYLOAD];
        sprintf(payload, "%.1f", cpu_temp);
        client->publish(topics[SERVER_RES_TMP], payload, strlen(payload));
      }
      /* Handle num faces request */
      else if(msg->get_topic() == topics[SERVER_REQ_NUM_FACES]) {
        LOG("Got temperature request", std::cout, "MQTT SERVER")
        int num_faces = get_num_faces_from_db(mysql_connection);
        char payload[MAX_MQTT_PAYLOAD];
        sprintf(payload, "%d", num_faces);
        client->publish(topics[SERVER_RES_NUM_FACES], payload, strlen(payload));
      }
      /* Handle num faces request */
      else if(msg->get_topic() == topics[SERVER_REQ_LOAD]) {
        LOG("Got load request", std::cout, "MQTT SERVER")
        double cpu_load = get_cpu_load();
        char payload[MAX_MQTT_PAYLOAD];
        sprintf(payload, "%.2f", cpu_load);
        client->publish(topics[SERVER_RES_LOAD], payload, strlen(payload));
      }
      /* Handle num faces request */
      else if(msg->get_topic() == topics[SERVER_REQ_AUDIO]) {
        LOG("Got audio request", std::cout, "MQTT SERVER")
        double energy = get_audio_energy_from_db(mysql_connection);
        char payload[MAX_MQTT_PAYLOAD];
        sprintf(payload, "%.2f", energy);
        client->publish(topics[SERVER_RES_AUDIO], payload, strlen(payload));
      }
      /* Handle num faces request */
      else if(msg->get_topic() == topics[SERVER_REQ_BLE]) {
        LOG("Got audio request", std::cout, "MQTT SERVER")
        double energy = get_ble_from_db(mysql_connection);
        char payload[MAX_MQTT_PAYLOAD];
        sprintf(payload, "%.2f", energy);
        client->publish(topics[SERVER_RES_BLE], payload, strlen(payload));
      }
    }
    else if (!client->is_connected()) {
      std::cout << "Lost connection" << std::endl;
      std::cout << "Re-established connection" << std::endl;
    }
  }
  LOG("Exiting", std::cout, "MQTT SERVER")
  mysql_close(mysql_connection);
  return;
}

/**
 * Get the CPU temperature 
 * @return Current CPU temperature [C]
*/
static double get_cpu_temp() {
  std::ifstream temp_file;
  temp_file.open(CPU_TEMP_FILE, std::ios::in);
  CHECK(!temp_file.is_open(), "Cannot open CPU temperature file", std::cerr)
  int temp;
  temp_file >> temp;
  temp_file.close();
  return (double) temp / 1000;
}

double get_cpu_load() {
  cpu_load_file.open(cpu_load_file_name, std::ios::out | std::ios::in);
  double ins_load;
  std::string cmd = std::string("top -n 1 | awk 'NR==3''{print $8}' > ") + cpu_load_file_name;
  system(cmd.c_str());
  cpu_load_file >> ins_load;
  cpu_load_file.close();
  return (100 - ins_load);
}

/**
 * Get the last audio energy stored in db
 * @param mysql_connection Active MYSQL connection
 * @return audio energy in the last database record
*/
static double get_audio_energy_from_db(MYSQL *mysql_connection) {
  char mysql_query[MAX_MYSQL_QUERY];
  sprintf(mysql_query, "SELECT audio_energy FROM audio_table ORDER BY id DESC LIMIT 1");
  CHECK(mysql_real_query(mysql_connection, mysql_query, strlen(mysql_query)),
   "Cannot perform MYSQL query for audio energy", std::cerr)
  MYSQL_RES *result = mysql_store_result(mysql_connection);
  MYSQL_ROW row;
  row = mysql_fetch_row(result);
  double energy = atof(row[0]);
  return energy;
}

/**
 * Get the last BLE distance stored in db
 * @param mysql_connection Active MYSQL connection
 * @return BLE distance in the last database record
*/
static double get_ble_from_db(MYSQL *mysql_connection) {
  char mysql_query[MAX_MYSQL_QUERY];
  sprintf(mysql_query, "SELECT dist FROM ble_table ORDER BY id DESC LIMIT 1");
  CHECK(mysql_real_query(mysql_connection, mysql_query, strlen(mysql_query)),
   "Cannot perform MYSQL query for BLE", std::cerr)
  MYSQL_RES *result = mysql_store_result(mysql_connection);
  MYSQL_ROW row;
  row = mysql_fetch_row(result);
  double ble = atof(row[0]);
  return ble;
}