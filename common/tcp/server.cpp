#include <common/communications.h>
#include <unordered_map>
#include <iostream>

namespace common
{
  namespace tcp
  {
    class server
     : public iserver
     , public std::enable_shared_from_this<iserver>
    {
      public:
        server(const int a_port, boost::asio::io_service& a_io_service, params_t& a_params);
        void run() override;
        void remove_client(const int a_client_id) override;
        void set_on_connected(std::function<void(const int)> a_on_connected) override;
        void set_on_disconnected(std::function<void(const int)> a_on_disconnected) override;
        void set_on_message(std::function<void(const int, const char *, std::size_t)> a_on_message) override;
        void on_connected(const int a_client_id) override;
        void on_disconnected(const int a_client_id) override;
        void on_message(const int a_client_id, const char *a_data, std::size_t a_len) override;
        void send_message(const int a_client_id, const std::string &a_message) override;
        std::size_t clients_count() override;

      private:
        void do_accept() override;

      private:
        boost::asio::io_service& m_io_service;
        boost::asio::ip::tcp::socket m_listener;
        boost::asio::ip::tcp::acceptor m_acceptor;
        std::unordered_map<int, iclient_session::ref> m_clients;
        params_t& m_params;

        std::function<void(const int)> m_on_connected_func;
        std::function<void(const int)> m_on_disconnected_func;
        std::function<void(const int, const char *, std::size_t)> m_on_message_func;
    };

    server::server(const int a_port, boost::asio::io_service& a_io_service, params_t& a_params)
     : m_io_service(a_io_service)
     , m_listener(a_io_service)
     , m_acceptor(a_io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), a_port))
     , m_params(a_params)
    {
    }

    void server::run()
    {
      do_accept();
    }

    void server::remove_client(const int a_client_id)
    {
      const auto& found_it = m_clients.find(a_client_id);
      if(found_it != m_clients.end())
      {
        //std::cout << "client removed" << std::endl;
        found_it->second->shutdown();
        m_clients.erase(found_it);
        on_disconnected(a_client_id);
      }
    }

    void server::set_on_connected(std::function<void(const int)> a_on_connected)
    {
      m_on_connected_func = a_on_connected;
    }

    void server::set_on_disconnected(std::function<void(const int)> a_on_disconnected)
    {
      m_on_disconnected_func = a_on_disconnected;
    }

    void server::set_on_message(std::function<void(const int, const char *, std::size_t)> a_on_message)
    {
      m_on_message_func = a_on_message;
    }

    void server::on_connected(const int a_client_id)
    {
      if(m_on_connected_func != nullptr)
        m_on_connected_func(a_client_id);
    }

    void server::on_disconnected(const int a_client_id)
    {
      if(m_on_disconnected_func != nullptr)
        m_on_disconnected_func(a_client_id);
    }
    
    void server::on_message(const int a_client_id, const char *a_data, std::size_t a_len)
    {
      if(m_on_message_func != nullptr)
        m_on_message_func(a_client_id, a_data, a_len);
    }

    void server::send_message(const int a_client_id, const std::string &a_message)
    {
      const auto& found_it = m_clients.find(a_client_id);
      if(found_it != m_clients.end())
        found_it->second->send_message(a_message);
    }

    std::size_t server::clients_count()
    {
      return m_clients.size();
    }

    void server::do_accept()
    {
      m_acceptor.async_accept(m_listener, [this](boost::system::error_code a_ec)
      {
        if(!a_ec)
        {
          int client_id = m_listener.native_handle();
          auto new_client = common::tcp::create_client_session(m_listener, m_io_service, shared_from_this(), m_params);
          m_clients.insert(std::make_pair(client_id, new_client));
          new_client->start();
          on_connected(client_id);
        }
        do_accept();
      });
    }

  } //namespace tcp
} //namespace common

namespace common
{
  namespace tcp
  {
    iserver::ref create_server(const int a_port, boost::asio::io_service& a_io_service, params_t& a_params)
    {
      return std::make_shared<server>(a_port, a_io_service, a_params);
    }
  } //namespace tcp
} //namespace common