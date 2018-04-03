#include <common/communications.h>
#include <functional>
#include <iostream>

namespace common
{
  namespace tcp
  {
    class client
     : public iclient
    {
      public:
        client(const std::string& a_ip, const int a_port, boost::asio::io_service::strand& a_strand);
        void run() override;
        void send_message(const std::string& a_data) override;
        void set_on_connected(std::function<void()> a_on_connected) override;
        void set_on_disconnected(std::function<void()> a_on_disconnected) override;
        void set_on_message(std::function<void(const std::string&)> a_on_message) override;

      private:
        void do_connect() override;
        void do_receive() override;

      private:
        boost::asio::io_service::strand& m_strand;
        boost::asio::ip::tcp::endpoint m_ep;
        boost::asio::ip::tcp::socket m_sock;
        bool m_is_connected;
        pbuf_t m_buffer;

        std::function<void()> m_on_connected_func;
        std::function<void()> m_on_disconnected_func;
        std::function<void(const std::string&)> m_on_message_func;
    };

    client::client(const std::string& a_ip, const int a_port, boost::asio::io_service::strand& a_strand)
     : m_strand(a_strand)
     , m_ep(boost::asio::ip::address::from_string(a_ip), a_port)
     , m_sock(a_strand.get_io_service())
     , m_is_connected(false)
     , m_buffer(std::make_unique<buf_t>())
    {
    }

    void client::run()
    {
      do_connect();
    }

    void client::send_message(const std::string& a_data)
    {
      if(m_is_connected)
      {
        auto async_write_handler = [](const boost::system::error_code& /*err*/, size_t /*bytes*/)
        {
        };
      
        m_sock.async_write_some(boost::asio::buffer(a_data.c_str(), a_data.length()), m_strand.wrap(async_write_handler));
      }
    }

    void client::set_on_connected(std::function<void()> a_on_connected)
    {
      m_on_connected_func = a_on_connected;
    }

    void client::set_on_disconnected(std::function<void()> a_on_disconnected)
    {
      m_on_disconnected_func = a_on_disconnected;
    }

    void client::set_on_message(std::function<void(const std::string&)> a_on_message)
    {
      m_on_message_func = a_on_message;
    }

    void client::do_connect()
    {
      m_sock.async_connect(m_ep, m_strand.wrap([this](const boost::system::error_code& a_ec)
      {
        if(!a_ec)
        {
          m_is_connected = true;
          m_sock.set_option(boost::asio::ip::tcp::socket::reuse_address(true));
          do_receive();

          if(m_on_connected_func != nullptr)
            m_on_connected_func();
        }
        else
        {
          m_is_connected = false;

          if(m_on_disconnected_func != nullptr)
            m_on_disconnected_func();
        }
      }));
    }

    void client::do_receive()
    {
      m_buffer->fill(0);

      auto completion_handler = [this](const boost::system::error_code& a_ec, std::size_t a_len) -> std::size_t {
        if(a_ec)
          return 0;

        if(a_len > 0)
        {
          bool cond = (m_buffer->data()[a_len - 1] == '\n');
          return cond ? 0 : 1;
        }
        return 1;
      };

      auto read_handler = [this](const boost::system::error_code& a_ec, std::size_t a_len) {
        if ((!a_ec) && (a_len > 0))
        {
          if(a_len == 0)
          {
            if(m_on_disconnected_func != nullptr)
              m_on_disconnected_func();
          }

          if(m_on_message_func != nullptr)
            m_on_message_func({m_buffer->data(), a_len});

          do_receive();
        }
        else
        {
          m_is_connected = false;
          do_connect();
        }
      };

      boost::asio::async_read(m_sock, boost::asio::buffer(m_buffer->data(), BUF_LENGTH), completion_handler, m_strand.wrap(read_handler));
    }
  } //namespace tcp
} //namespace common

namespace common
{
  namespace tcp
  {
    iclient::ref create_client(const std::string& a_ip, const int a_port, boost::asio::io_service::strand& a_strand)
    {
      return std::make_shared<client>(a_ip, a_port, a_strand);
    }   
  } //namespace tcp
} //namespace common
