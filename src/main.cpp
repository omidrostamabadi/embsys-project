#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <fstream>
#include <atomic>
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <ctime>
#include <face_detect.h>
#include <unistd.h>
#include <mqtt/callback.h>
#include <mqtt/client.h>
#include <mqtt/connect_options.h>
#include <mqtt/types.h>
#include <mqtt/topic.h>
#include <mqtt/iclient_persistence.h>
#include "MQTTClient.h"
#include <mqtt_client.h>

double get_cpu_temp() {
  /* - REPLACE - thermal_zone5 represents CPU temperature on this machine
    Replace with proper thermal_zone on other machines */
  std::string temp_path = "/sys/class/thermal/thermal_zone5/temp";
  std::ifstream temp_file;
  temp_file.open(temp_path, std::ios::in);
  int temp;
  temp_file >> temp;
  return (double) temp / 1000;
}

int main() {
  const mqtt::string face_topic = "door0/numfaces";
  const mqtt::string temp_topic = "door0/temperature";

  get_cpu_temp();

  CascadeClassifier face_cascade;
  cv::VideoCapture camera(0);
  if (!camera.isOpened()) {
      std::cerr << "ERROR: Could not open camera" << std::endl;
      return 1;
  }

  /* - REPLACE - Give correct path to haarcascade_frontalcatface.xml on your machine */
  if( !face_cascade.load( "/home/punisher/Documents/courses/Embsys/Class/OpenCV-20220613/installation/OpenCV-master/share/opencv4/haarcascades/haarcascade_frontalcatface.xml" ) )
  {
      cout << "--(!)Error loading face cascade\n";
      return -1;
  };

  /* Init MQTT publisher */
  mqtt::client *client;
  mqtt::connect_options connOpts;
  mqtt::message_ptr pubmsg;
  try {
  client = new mqtt::client(ADDRESS, CLIENTID_PUB);
  connOpts.set_keep_alive_interval(20);
  connOpts.set_clean_session(true);
  connOpts.set_user_name(user_name);
  connOpts.set_password(password);

  client->connect(connOpts);

  }

  catch (const mqtt::exception& exc) {
    std::cerr << "Error: " << exc.what() << " ["
        << exc.get_reason_code() << "]" << endl;
    return 1;
  }
  
  namedWindow("Face");
	Mat frame;
  int num_faces = 0;
  int prev_num_faces = 0;

  double temperature = get_cpu_temp();
  while(true) {
    usleep(200 * 1000);
    camera >> frame;
    imwrite("test.png", frame);
    prev_num_faces = num_faces;
    num_faces = detectAndDisplay(frame, face_cascade);
    cout << "Number of faces: " << prev_num_faces << " -> " 
      << num_faces << endl;
    if(num_faces == prev_num_faces) {
      cout << "Number of faces not changed\n";
      continue;
    }
    try {
      cout << "Number of faces changed, publishing ...\n";
      char temp_msg_string[10];
      char face_msg_string[10];
      sprintf(temp_msg_string, "%.1f", temperature);
      sprintf(face_msg_string, "%d", num_faces);
      client->publish(temp_topic, temp_msg_string,
       strlen(temp_msg_string));
      client->publish(face_topic, face_msg_string,
       strlen(face_msg_string));
    }
    catch(const mqtt::exception& exc) {
      std::cerr << "Error: " << exc.what() << " ["
        << exc.get_reason_code() << "]" << endl;
      return 1;
    }
    temperature = get_cpu_temp();
  }
	
  return 0;
}