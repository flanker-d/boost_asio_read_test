#include <common/communications.h>
#include <iostream>
#include <chrono>

namespace common
{
  namespace tcp
  {
    using milli = std::chrono::milliseconds;

    class client_session
     : public iclient_session
    {
      public:
        client_session(boost::asio::ip::tcp::socket& a_sock, boost::asio::io_service& a_io_service, iserver::ref& a_server, params_t& a_params);
        ~client_session() override;
        void send_message(const std::string& a_data) override;
        void start() override;
        void shutdown() override;
      
      private:
        void increase_and_check_counter();

        void do_receive_custom_eol();
        void do_receive_custom_eol_std_find();
        void do_receive_read_until_eol();
        void do_receive_async_read();

      private:
        boost::asio::io_service::strand m_strand;
        boost::asio::ip::tcp::socket m_sock;
        iserver::weak_ref m_server;
        int m_client_id;
        pbuf_t m_buffer;
        std::string m_buffer_str;
        params_t& m_params;
        boost::asio::streambuf m_streambuf;
        std::function<void()> m_do_receive_func;
        decltype(std::chrono::high_resolution_clock::now()) m_start_time;
        int m_counter = 0;

    };

    client_session::client_session(boost::asio::ip::tcp::socket& a_sock, boost::asio::io_service& a_io_service, iserver::ref& a_server, params_t& a_params)
     : m_strand(a_io_service)
     , m_sock(std::move(a_sock))
     , m_server(a_server)
     , m_client_id(m_sock.native_handle())
     , m_buffer(std::make_unique<buf_t>())
     , m_params(a_params)
    {
      switch (m_params.do_read_type)
      {
        case read_func_type_e::custom_eol:
          m_do_receive_func = std::bind(&client_session::do_receive_custom_eol, this);
          break;
        case read_func_type_e::custom_eol_std_find:
          m_do_receive_func = std::bind(&client_session::do_receive_custom_eol_std_find, this);
          break;
        case read_func_type_e::read_until_eol:
          m_do_receive_func = std::bind(&client_session::do_receive_read_until_eol, this);
          break;
        case read_func_type_e::async_read:
          m_do_receive_func = std::bind(&client_session::do_receive_async_read, this);
          break;
        default:
          m_do_receive_func = std::bind(&client_session::do_receive_custom_eol, this);
          break;
      }
    }

    client_session::~client_session()
    {
      //std::cout << "client session dtor called" << std::endl;
    }

    void client_session::send_message(const std::string& a_data)
    {
      auto async_write_handler = [this](boost::system::error_code /*ec*/, std::size_t /*length*/)
      {
        //std::cout << to_send_str;
      };
      if(m_params.use_strand)
        boost::asio::async_write(m_sock, boost::asio::buffer(a_data.c_str(), a_data.length()), m_strand.wrap(async_write_handler));
      else
        boost::asio::async_write(m_sock, boost::asio::buffer(a_data.c_str(), a_data.length()), async_write_handler);

    }

    void client_session::start()
    {
      m_start_time = std::chrono::high_resolution_clock::now();
      m_do_receive_func();
    }

    void client_session::shutdown()
    {
      m_sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
      m_sock.close();
    }

    void client_session::increase_and_check_counter()
    {
      m_counter++;

      if(m_counter == m_params.read_counter)
      {
        auto finish_time = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<milli>(finish_time - m_start_time).count() << std::endl;
      }
    }

    void client_session::do_receive_custom_eol()
    {
      m_buffer->fill(0);
      
        auto async_read_completion_handler = [this](const boost::system::error_code& a_ec, std::size_t a_len)->std::size_t
        {
          if(a_ec)
            return 0;
          if(a_len > 0)
          {
            bool cond = (m_buffer->data()[a_len - 1] == '\n');
            return cond ? 0 : 1;
          }
          return 1;
        };
      
        auto async_read_handler = [this](const boost::system::error_code& a_ec, std::size_t a_len)
        {
          if(a_len == 0)
          {
            if(auto serv = m_server.lock())
            {
              serv->remove_client(m_client_id);
            }
          }

          if (!a_ec)
          {
            if(auto serv = m_server.lock())
            {
              serv->on_message(m_client_id, m_buffer->data(), a_len);
              increase_and_check_counter();
            }

            do_receive_custom_eol();
          }
          else
          {
            if(auto serv = m_server.lock())
            {
              serv->remove_client(m_client_id);
            }
          }
        };

        if(m_params.use_strand)
          boost::asio::async_read(m_sock, boost::asio::buffer(m_buffer->data(), BUF_LENGTH), async_read_completion_handler, m_strand.wrap(async_read_handler));
        else
          boost::asio::async_read(m_sock, boost::asio::buffer(m_buffer->data(), BUF_LENGTH), async_read_completion_handler, async_read_handler);
    }

    void client_session::do_receive_custom_eol_std_find()
    {
      m_buffer->fill(0);

      auto async_read_completion_handler = [this](const boost::system::error_code& a_ec, std::size_t a_len)->std::size_t
      {
        if ( a_ec)
          return 0;
        bool found = std::find(m_buffer->data(), m_buffer->data() + a_len, '\n') < m_buffer->data() + a_len;
        // we read one-by-one until we get to enter, no buffering
        return found ? 0 : 1;
      };

      auto async_read_handler = [this](const boost::system::error_code& a_ec, std::size_t a_len)
      {
        if(a_len == 0)
        {
          if(auto serv = m_server.lock())
          {
            serv->remove_client(m_client_id);
          }
        }

        if (!a_ec)
        {
          if(auto serv = m_server.lock())
          {
            serv->on_message(m_client_id, m_buffer->data(), a_len);
            increase_and_check_counter();
          }

          do_receive_custom_eol_std_find();
        }
        else
        {
          if(auto serv = m_server.lock())
          {
            serv->remove_client(m_client_id);
          }
        }
      };

      if(m_params.use_strand)
        boost::asio::async_read(m_sock, boost::asio::buffer(m_buffer->data(), BUF_LENGTH), async_read_completion_handler, m_strand.wrap(async_read_handler));
      else
        boost::asio::async_read(m_sock, boost::asio::buffer(m_buffer->data(), BUF_LENGTH), async_read_completion_handler, async_read_handler);
    }

    void client_session::do_receive_read_until_eol()
    {
      auto async_read_handler = [this](const boost::system::error_code& a_ec, std::size_t a_len)
      {
        if(a_len == 0)
        {
          if(auto serv = m_server.lock())
          {
            serv->remove_client(m_client_id);
          }
        }

        if (!a_ec)
        {
          if(auto serv = m_server.lock())
          {
            std::istream is(&m_streambuf);
            std::string line;
            std::getline(is, line);
            serv->on_message(m_client_id, line.c_str(), line.size());
            increase_and_check_counter();
          }

          do_receive_read_until_eol();
        }
        else
        {
          if(auto serv = m_server.lock())
          {
            serv->remove_client(m_client_id);
          }
        }
      };
      if(m_params.use_strand)
        boost::asio::async_read_until(m_sock, m_streambuf, '\n', m_strand.wrap(async_read_handler));
      else
        boost::asio::async_read_until(m_sock, m_streambuf, '\n', async_read_handler);

    }

    void client_session::do_receive_async_read()
    {
      m_buffer->fill(0);
      auto async_read_handler = [this](const boost::system::error_code& a_ec, std::size_t a_len)
      {
        if(a_len == 0)
        {
          if(auto serv = m_server.lock())
          {
            serv->remove_client(m_client_id);
          }
        }

        if (!a_ec)
        {
          if(auto serv = m_server.lock())
          {
            m_buffer_str += {m_buffer->data(), a_len};

            while(true)
            {
              auto term_pos = m_buffer_str.find('\n');
              if(term_pos != std::string::npos)
              {
                std::string cmd(m_buffer_str.begin(), m_buffer_str.begin() + term_pos);
                serv->on_message(m_client_id, cmd.c_str(), cmd.size());
                m_buffer_str.erase(m_buffer_str.begin(), m_buffer_str.begin() + term_pos + 1);
                increase_and_check_counter();
              }
              else
              {
                m_buffer->fill(0);
                return;
              }
            }
            m_buffer->fill(0);
          }

          do_receive_async_read();
        }
        else
        {
          if(auto serv = m_server.lock())
          {
            serv->remove_client(m_client_id);
          }
        }
      };

      if(m_params.use_strand)
        boost::asio::async_read(m_sock, boost::asio::buffer(m_buffer->data(), BUF_LENGTH), m_strand.wrap(async_read_handler));
      else
        boost::asio::async_read(m_sock, boost::asio::buffer(m_buffer->data(), BUF_LENGTH), async_read_handler);
    }
  } //namespace tcp
} //namespace common

namespace common
{
  namespace tcp
  {
    iclient_session::ref create_client_session(boost::asio::ip::tcp::socket& a_sock, boost::asio::io_service& a_io_service, iserver::ref a_server, params_t& a_params)
    {
      return std::make_shared<client_session>(a_sock, a_io_service, a_server, a_params);
    }
  } //namespace tcp
} //namespace common