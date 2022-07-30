#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include <helper.h>
#include <mutex>

cv::VideoCapture camera;
std::mutex cam_mtx;

int main(int argc, char *argv[]) {
  std::vector<std::thread> threads;
  threads.push_back(std::thread(face_detector));
  threads.push_back(std::thread(http_server, argc, argv));
  threads.push_back(std::thread(mqtt_server));
  std::cout << "Waiting for threads\n";
  threads[0].join();
  threads[1].join();
  threads[2].join();
}