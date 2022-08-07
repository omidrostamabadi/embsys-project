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
#include <atomic>
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <ctime>
#include <helper.h>

using namespace cv;
using namespace std;

/**
* Detects the number of faces
* @param frame The frame which faces are detected in
* @param face_cascade The classifier to detect faces
* @return Number of faces in the frame
*/
int detect_faces(cv::Mat frame, cv::CascadeClassifier &face_cascade)
{
  Mat frame_gray;
  cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
  equalizeHist(frame_gray, frame_gray);
  std::vector<Rect> faces;
  face_cascade.detectMultiScale(frame_gray, faces);	
  for (size_t i = 0; i < faces.size(); i++) {
    cv::putText(frame, 
            to_string(i+1),
            cv::Point(faces[i].x+(faces[i].width/2),faces[i].y - 10), 
            cv::FONT_HERSHEY_COMPLEX_SMALL, 
            1.0, 
            cv::Scalar(255,255,255), 
            1, 
            cv:: LINE_AA);
    
    rectangle(frame, Point(faces[i].x, faces[i].y), Point(faces[i].x + faces[i].width, faces[i].y + faces[i].height), Scalar(0, 0, 255), 3, LINE_8);
    Mat faceROI = frame_gray(faces[i]);
	}
  return (faces.size());
}

void face_detector() {
  std::string classifier_path = "/home/punisher/Documents/courses/Embsys/Class/OpenCV-20220613/installation/OpenCV-master/share/opencv4/haarcascades/haarcascade_frontalcatface.xml";
  CascadeClassifier face_cascade;
  if(camera.isOpened())
    std::cout << "Cam is opened!\n";
  else
    std::cout << "Cam not opened\n";
  init_camera();
  // cv::VideoCapture camera(0);
  // if (!camera.isOpened()) {
  //     std::cerr << "ERROR: Could not open camera" << std::endl;
  //     return EXIT_FAILURE;
  // }

  if(!face_cascade.load(classifier_path.c_str())) {
    cout << "--(!)Error loading face cascade\n";
    return;
  };

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

  cv::Mat frame;
  int num_faces = 0, prev_num_faces;
  char mysql_query_msg[MAX_MYSQL_QUERY];
  while(!finish) {
    camera >> frame;
    prev_num_faces = num_faces;
    num_faces = detect_faces(frame, face_cascade);
    if(num_faces != prev_num_faces) {
      LOG("Number of faces changed", std::cout, "FACE DETECTOR")
      sprintf(mysql_query_msg, "INSERT INTO face_table(ts, num_faces) VALUES (NOW(), %d)", num_faces);
      CHECK_VOID(mysql_query(mysql_connection, mysql_query_msg), "Cannot insert into database", std::cerr)
    }
  }
  LOG("Exiting", std::cout, "FACE DETECTOR")
}