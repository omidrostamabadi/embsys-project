#include <iostream>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <math.h>
#include <mysql/mysql.h>
#include <helper.h>

std::string dev_name = "Redmi 9";
std::string out_file = "tmp.txt";

const int meas_pow = -74;
const int N = 2;

const float min_distance = 1.0;

double get_distance(int rssi) {
  double dist = pow(10, ((float)meas_pow - rssi) / 10 / N);
  return dist;
}

void scan_ble_devices() {
  std::string cmd = "sudo btmgmt find > " + out_file;
  system(cmd.c_str());
}

int find_device_rssi() {
  std::ifstream file;
  file.open(out_file, std::ios::in);

  if(!file.is_open()) {
    std::cout << "File " << out_file << " Could not be opened\n";
    exit(EXIT_FAILURE);
  }

  std::stringstream buf;
  buf << file.rdbuf();
  std::string file_cont = buf.str();
  size_t loc = file_cont.find(dev_name, 0);
  size_t lloc = file_cont.rfind("rssi", loc);
  std::string rssi_str = file_cont.substr(lloc + 5, 10);
  size_t sp_loc = rssi_str.find(' ');
  std::string rssi_substr = rssi_str.substr(0, sp_loc);
  int rssi = atof(rssi_substr.c_str());
  return rssi;
}

static MYSQL *mysql_connection;

void ble_manager() {
  // sleep(7);
  std::cout << "Started ble\n";
  /* Connect to database */
  // MYSQL *mysql_connection2 = mysql_init(NULL);
  // MYSQL *mysql_connection_ret2 = NULL;
  // mysql_connection_ret2 = mysql_real_connect(mysql_connection2, "localhost", "omid", "123456",
  //  "emb", 0, NULL, 0);
  // if(mysql_connection_ret2 != NULL) {
  //   std::cout << "Connected to db!\n";
  // }
  // else {
  //   std::cout << "Failed connecting to db :(\n";
  //   exit(EXIT_FAILURE);
  // }
  // MYSQL *mysql_connection;
  mysql_connection = mysql_init(NULL);
  std::string user = "omid";
  std::string password = "123456";
  std::string host_name = "localhost";
  std::string database_name = "emb";
  CHECK_VOID(connect_to_db(mysql_connection, user, password, host_name, database_name),
   "Cannot connect to database", std::cerr)
  // connect_to_db(mysql_connection, user, password, host_name, database_name);
  // MYSQL *mysql_connection_ret2 = mysql_real_connect(mysql_connection, host_name.c_str(), user.c_str(),
  //  password.c_str(), database_name.c_str(), 3306, NULL, 0);
  std::cout << "ERRNO is " << mysql_errno(mysql_connection) << std::endl;
  std::cout << "Error is " << mysql_error(mysql_connection) << std::endl;
  
  int rssi;
  double distance;
  char mysql_query_msg[MAX_MYSQL_QUERY];
  while(true) {
    scan_ble_devices();
    rssi = find_device_rssi();
    distance = get_distance(rssi);
    if(distance < min_distance) {
      LOG("Distance less than minimum", std::cout, "BLE MANAGER")
      sprintf(mysql_query_msg, "INSERT INTO ble_table(ts, dist) VALUES (NOW(), %.2f)", distance);
      CHECK_VOID(mysql_query(mysql_connection, mysql_query_msg), "Cannot insert into database", std::cerr)
    }
    std::cout << "RSSI is " << rssi << std::endl;
    std::cout << "Distance is " << distance << std::endl;
  }
}