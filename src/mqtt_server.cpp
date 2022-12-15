#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <ctime>
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

int main() {
  /* Init MQTT */
  mqtt::client *client;
  mqtt::connect_options connOpts;
  connOpts.set_clean_session(false);
  connOpts.set_user_name(user_name);
  connOpts.set_password(password);
  CHECK(create_new_mqtt_client(&client, MQTT_SERVER_URI, MQTT_CLIENT_ID, &connOpts),
   "Could not create MQTT client", std::cerr)

  /* Subscribe to some topics */
  server_subscribe_topics(client);

  /* Connect to database */
  MYSQL *mysql_connection = mysql_init(NULL);
  std::string user = "omid";
  std::string password = "123456";
  std::string host_name = "localhost";
  std::string database_name = "emb";
  CHECK(connect_to_db(mysql_connection, user, password, host_name, database_name),
   "Cannot connect to database", std::cerr)

  while(true) {
    usleep(500 * 1000);
    auto msg = client->consume_message();
    if (msg) {
      /* Handle temperature request */
      if(msg->get_topic() == topics[SERVER_REQ_TMP]) {
        LOG("Got temperature request", std::cout, "MQTT SERVER")
        double cpu_temp = get_cpu_temp_from_db(mysql_connection);
        char payload[MAX_MQTT_PAYLOAD];
        sprintf(payload, "%.1f", cpu_temp);
        client->publish(topics[SERVER_RES_TMP], payload, strlen(payload));
      }

      /* Handle num faces request */
      if(msg->get_topic() == topics[SERVER_REQ_NUM_FACES]) {
        LOG("Got temperature request", std::cout, "MQTT SERVER")
        int num_faces = get_num_faces_from_db(mysql_connection);
        char payload[MAX_MQTT_PAYLOAD];
        sprintf(payload, "%d", num_faces);
        client->publish(topics[SERVER_RES_NUM_FACES], payload, strlen(payload));
      }

    }
    else if (!client->is_connected()) {
      std::cout << "Lost connection" << std::endl;
      std::cout << "Re-established connection" << std::endl;
    }
  }
  mysql_close(mysql_connection);
  return 0;
}