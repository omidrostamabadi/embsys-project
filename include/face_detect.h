#ifndef FACE_DETECT_H
#define FACE_DETECT_H

#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include<opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <ctime>

using namespace cv;
using namespace std;

const size_t MYSQL_QUERY_MSG_MAX_LEN = 200; // Maximum length of query
static const std::string PROG_NAME = "face_detector"; // Used for logging purposes

int detectAndDisplay(Mat frame, CascadeClassifier &face_cascade);

#endif
