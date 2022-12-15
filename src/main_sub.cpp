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
#include <mqtt_client.h>
#include <mysql/mysql.h>

class my_callback : public virtual mqtt::callback
{

	void connected(const std::string& cause) override {
		std::cout << "\nConnected: " << cause << std::endl;
		std::cout << std::endl;
	}

	// Callback for when the connection is lost.
	// This will initiate the attempt to manually reconnect.
	void connection_lost(const std::string& cause) override {
		std::cout << "\nConnection lost";
		if (!cause.empty())
			std::cout << ": " << cause;
		std::cout << std::endl;
	}

	// Callback for when a message arrives.
	void message_arrived(mqtt::const_message_ptr msg) override {
		std::cout << msg->get_topic() << ": " << msg->get_payload_str() << std::endl;
	}

	void delivery_complete(mqtt::delivery_token_ptr token) override {}

public:
	my_callback() {}
};


int main() {
  const mqtt::string face_topic = "door0/numfaces";
  const mqtt::string temp_topic = "door0/temperature";

  /* Init MQTT publisher */
  mqtt::client *client;
  mqtt::connect_options connOpts;
  try {
    client = new mqtt::client(ADDRESS, CLIENTID_SUB);
    connOpts.set_clean_session(false);
    connOpts.set_user_name(user_name);
    connOpts.set_password(password);

    my_callback cb = my_callback();
    client->set_callback(cb);

    client->connect(connOpts);
    client->subscribe(face_topic, 0);
    client->subscribe(temp_topic);
  }

  catch (const mqtt::exception& exc) {
    std::cerr << "Error: " << exc.what() << " ["
        << exc.get_reason_code() << "]" << std::endl;
    return 1;
  }


  /* Connect to the database */
  /* - REPLACE - Your database credentials */
  std::string host_name = "localhost";
  std::string user = "omid";
  std::string password = "123456";

  MYSQL *MySQLConRet;
  MYSQL *MySQLConnection = NULL;
  MySQLConnection = mysql_init(NULL);

  MySQLConRet = mysql_real_connect( MySQLConnection,
                                          host_name.c_str(), 
                                          user.c_str(), 
                                          password.c_str(), 
                                          NULL,  // No database specified
                                          0, 
                                          NULL,
                                          0 );

  if (MySQLConRet == NULL)
    std::cout << "Connecting to Database failed!\n";
   
  printf("MySQL Connection Info: %s \n", mysql_get_host_info(MySQLConnection));
  printf("MySQL Client Info: %s \n", mysql_get_client_info());
  printf("MySQL Server Info: %s \n", mysql_get_server_info(MySQLConnection));

  /* - REPLACE - Replace 'emb' with your database */
  if(mysql_query(MySQLConnection,
   "use emb")) {
    printf("Error %u: %s\n", mysql_errno(MySQLConnection), mysql_error(MySQLConnection));
    return EXIT_FAILURE;
   }

  mqtt::string srvr_uri;
  double temperature = 0.0;
  int num_faces = 0;
  int db_counter = 0;
  while(true) {
    srvr_uri = client->get_server_uri();
    usleep(500 * 1000);
    auto msg = client->consume_message();

			if (msg) {
				if (msg->get_topic() == "command" &&
						msg->to_string() == "exit") {
					std::cout << "Exit command received" << std::endl;
					break;
				}

        if(msg->get_topic() == temp_topic) {
          temperature = atof(msg->to_string().c_str());
          std::cout << "Got temperature info " <<
           msg->to_string() << " = " <<
            atof(msg->to_string().c_str()) << std::endl;
          db_counter++;
        }

        if(msg->get_topic() == face_topic) {
          num_faces = atoi(msg->to_string().c_str());
          std::cout << "Got face info " <<
           msg->to_string() << " = " <<
            atof(msg->to_string().c_str()) << std::endl;
          db_counter++;
        }

				std::cout << msg->get_topic() << ": " << msg->to_string() << std::endl;

        char mysql_q_msg[200];
        /* - REPLACE - Replace 'emb_table' with your table */
        sprintf(mysql_q_msg,
         "INSERT INTO emb_table(ts, num_faces, temp) VALUES (NOW(), %d, %0.1f)",
          num_faces, temperature);
        if(db_counter == 2) {
          db_counter = 0;
          std::cout << "Inserting into database ...\n";
          if (mysql_query(MySQLConnection, mysql_q_msg))
            {
              printf("Error %u: %s\n", mysql_errno(MySQLConnection), mysql_error(MySQLConnection));
              return(1);
            }
        }
			}
			else if (!client->is_connected()) {
				std::cout << "Lost connection" << std::endl;
				
				std::cout << "Re-established connection" << std::endl;
			}
  }
	
  mysql_close(MySQLConnection);
  return 0;
}