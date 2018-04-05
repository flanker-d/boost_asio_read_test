#pragma once

#include <common/interface.h>
#include <boost/asio.hpp>
#include <boost/asio/socket_base.hpp>
#include <array>
#include <functional>

namespace common
{
  namespace tcp
  {
    const int BUF_LENGTH = 32768;
    using buf_t = std::array<char, BUF_LENGTH>;
    using pbuf_t = std::unique_ptr<buf_t>;

    enum class read_func_type_e
    {
        custom_eol,
        custom_eol_std_find,
        read_until_eol,
        async_read
    };

    struct params_t
    {
      read_func_type_e do_read_type;
      bool use_strand;
      int read_counter;
    };

    class iserver
      : public interface<iserver>
    {
      public:
        virtual void run() = 0;
        virtual void remove_client(const int a_client_id) = 0;
        virtual void set_on_connected(std::function<void(const int)> a_on_connected) = 0;
        virtual void set_on_disconnected(std::function<void(const int)> a_on_disconnected) = 0;
        virtual void set_on_message(std::function<void(const int, const char *, std::size_t)> a_on_message) = 0;
        virtual void on_connected(const int a_client_id) = 0;
        virtual void on_disconnected(const int a_client_id) = 0;
        virtual void on_message(const int a_client_id, const char *a_data, std::size_t a_len) = 0;
        virtual void send_message(const int a_client_id, const std::string& a_message) = 0;
        virtual std::size_t clients_count() = 0;

      protected:
        virtual void do_accept() = 0;
    };

    iserver::ref create_server(const int a_port, boost::asio::io_service& a_io_service, params_t& a_params);

    class iclient_session
      : public interface<iclient_session>
    {
      public:
        virtual void send_message(const std::string& a_data) = 0;
        virtual void start() = 0;
        virtual void shutdown() = 0;
    };

    iclient_session::ref create_client_session(boost::asio::ip::tcp::socket& a_sock, boost::asio::io_service& a_io_service, iserver::ref a_server, params_t& a_params);

    class iclient
      : public interface<iclient>
    {
      public:
        virtual void run() = 0;
        virtual void send_message(const std::string& a_data) = 0;
        virtual void set_on_connected(std::function<void()> a_on_connected) = 0;
        virtual void set_on_disconnected(std::function<void()> a_on_disconnected) = 0;
        virtual void set_on_message(std::function<void(const std::string&)> a_on_message) = 0;

      protected:
        virtual void do_connect() = 0;
        virtual void do_receive() = 0;
    };

    iclient::ref create_client(const std::string& a_ip, const int a_port, boost::asio::io_service::strand& a_strand);
  } //namespace tcp
} //namespace common