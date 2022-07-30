//
// Copyright (c) 2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP server, small
//
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <helper.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

/* Working directory of the server */
std::string wd = "/home/punisher/Documents/courses/Embsys/project/build/";

/* The directory to save temporary photos at. It is relative to wd */
std::string photo_path = "photo/";

namespace my_program_state
{
    std::size_t
    request_count()
    {
        static std::size_t count = 0;
        return ++count;
    }

    std::time_t
    now()
    {
        return std::time(0);
    }
}

class http_connection : public std::enable_shared_from_this<http_connection>
{
public:
    http_connection(tcp::socket socket)
        : socket_(std::move(socket))
    {
      is_file = false;
    }

    // Initiate the asynchronous operations associated with the connection.
    void
    start()
    {
        read_request();
        CHECK_VOID_deadline();
    }

private:
    // The socket for the currently connected client.
    tcp::socket socket_;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{8192};

    // The request message.
    http::request<http::dynamic_body> request_;

    // The response message.
    http::response<http::dynamic_body> response_;

    http::response<http::file_body> file_response_;

    bool is_file;

    // The timer for putting a deadline on connection processing.
    net::steady_timer deadline_{
        socket_.get_executor(), std::chrono::seconds(60)};

    // Asynchronously receive a complete request message.
    void
    read_request()
    {
        auto self = shared_from_this();

        http::async_read(
            socket_,
            buffer_,
            request_,
            [self](beast::error_code ec,
                std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);
                if(!ec)
                    self->process_request();
            });
    }

    // Determine what needs to be done with the request message.
    void
    process_request()
    {
        response_.version(request_.version());
        response_.keep_alive(false);

        switch(request_.method())
        {
        case http::verb::get:
            response_.result(http::status::ok);
            response_.set(http::field::server, "I am the Beast Server");
            create_response();
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body())
                << "Invalid request-method '"
                << std::string(request_.method_string())
                << "'";
            break;
        }

        write_response();
    }


    // void send_file(beast::string_view target)
    // {
    //     // Request path must be absolute and not contain "..".
    //     if (target.empty() || target[0] != '/' || target.find("..") != std::string::npos)
    //     {
    //         send_bad_response(
    //             http::status::not_found,
    //             "File not found\r\n");
    //         return;
    //     }

    //     std::string full_path = doc_root_;
    //     full_path.append(
    //         target.data(),
    //         target.size());

    //     http::file_body::value_type file;
    //     beast::error_code ec;
    //     file.open(
    //         full_path.c_str(),
    //         beast::file_mode::read,
    //         ec);
    //     if(ec)
    //     {
    //         send_bad_response(
    //             http::status::not_found,
    //             "File not found\r\n");
    //         return;
    //     }

    //     file_response_.emplace(
    //         std::piecewise_construct,
    //         std::make_tuple(),
    //         std::make_tuple(alloc_));

    //     file_response_->result(http::status::ok);
    //     file_response_->keep_alive(false);
    //     file_response_->set(http::field::server, "Beast");
    //     file_response_->set(http::field::content_type, mime_type(std::string(target)));
    //     file_response_->body() = std::move(file);
    //     file_response_->prepare_payload();

    //     file_serializer_.emplace(*file_response_);

    //     http::async_write(
    //         socket_,
    //         *file_serializer_,
    //         [this](beast::error_code ec, std::size_t)
    //         {
    //             socket_.shutdown(tcp::socket::shutdown_send, ec);
    //             file_serializer_.reset();
    //             file_response_.reset();
    //             accept();
    //         });
    // }

    // Construct a response message based on the program state.
    void
    create_response()
    {
      is_file = false;
      static cv::Mat curr_frame;
      std::string file_name = "test.png";
      std::string file_abs_path = wd + photo_path + file_name;
      beast::error_code ec;
        if(request_.target() == "/capture-photo")
        {
          curr_frame = get_current_frame();
          cv::imwrite(file_abs_path, curr_frame);
          LOG("Photo saved", std::cout, "HTTP SERVER")
          response_.set(http::field::content_type, "text/html");
          beast::ostream(response_.body())
              << "<html>\n"
              <<  "<head><title>Capture Photo</title></head>\n"
              <<  "<body>\n"
              <<  "<h1>Photo</h1>\n"
              <<  "<p>There have been "
              <<  my_program_state::request_count()
              <<  " requests so far.</p>\n"
              << "<a href=\""
              << photo_path + file_name
              << "\">see</a>"
              <<  "</body>\n"
              <<  "</html>\n";
        }
        else if(request_.target() == "/time")
        {
            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                <<  "<html>\n"
                <<  "<head><title>Current time</title></head>\n"
                <<  "<body>\n"
                <<  "<h1>Current time</h1>\n"
                <<  "<p>The current time is "
                <<  my_program_state::now()
                <<  " seconds since the epoch.</p>\n"
                <<  "</body>\n"
                <<  "</html>\n";
        }
        else
        {
          LOG("Sending file", std::cout, "HTTP SERVER")
          is_file = true;
          http::file_body::value_type file;
          file.open((photo_path + file_name).c_str(), beast::file_mode::read, ec);
          file_response_.result(http::status::ok);
          file_response_.keep_alive(false);
          file_response_.set(http::field::server, "Beast");
          file_response_.set(http::field::content_type, "image/png");
          file_response_.body() = std::move(file);
          file_response_.prepare_payload();
          // response_.result(http::status::not_found);
          // response_.set(http::field::content_type, "image/*");
          // // beast::ostream(response_.body()) << "File not found\r\n";
          // beast::ostream(response_.body()) << curr_frame << "\r";
        }
    }

    // Asynchronously transmit the response message.
    void
    write_response()
    {
        auto self = shared_from_this();
        if(is_file) {
          file_response_.set(http::field::content_length, file_response_.body().size());

          http::async_write(
              socket_,
              file_response_,
              [self](beast::error_code ec, std::size_t)
              {
                  self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                  self->deadline_.cancel();
              });
        }
        else {
          response_.set(http::field::content_length, response_.body().size());

          http::async_write(
              socket_,
              response_,
              [self](beast::error_code ec, std::size_t)
              {
                  self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                  self->deadline_.cancel();
              });
        }
    }

    // CHECK_VOID whether we have spent enough time on this connection.
    void
    CHECK_VOID_deadline()
    {
        auto self = shared_from_this();

        deadline_.async_wait(
            [self](beast::error_code ec)
            {
                if(!ec)
                {
                    // Close socket to cancel any outstanding operation.
                    self->socket_.close(ec);
                }
            });
    }
};

// "Loop" forever accepting new connections.
void
http_server(tcp::acceptor& acceptor, tcp::socket& socket)
{
  acceptor.async_accept(socket,
      [&](beast::error_code ec)
      {
          if(!ec)
              std::make_shared<http_connection>(std::move(socket))->start();
          http_server(acceptor, socket);
      });
}

void http_server(int argc, char* argv[])
{
    try
    {
      if(mkdir((wd + photo_path).c_str(), S_IRWXU | S_IRWXG | S_IRWXO | S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH | S_IWOTH)) {
        if(errno != EEXIST) {
          std::cerr << "Error in mkdir: " << errno << std::endl;
          return;
        }
        else {
          LOG("Photo directory already exists", std::cout, "HTTP SERVER")
        }
      }
      CHECK_VOID(init_camera(), "Cannot init camera", std::cerr)
        // check command line arguments.
        if(argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " <address> <port>\n";
            std::cerr << "  For IPv4, try:\n";
            std::cerr << "    receiver 0.0.0.0 80\n";
            std::cerr << "  For IPv6, try:\n";
            std::cerr << "    receiver 0::0 80\n";
            return;
        }

        auto const address = net::ip::make_address(argv[1]);
        unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));

        net::io_context ioc{1};

        tcp::acceptor acceptor{ioc, {address, port}};
        tcp::socket socket{ioc};
        http_server(acceptor, socket);

        ioc.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return;
    }
}