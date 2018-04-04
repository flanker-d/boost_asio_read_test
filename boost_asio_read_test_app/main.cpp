#include <common/communications.h>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>

using namespace common::tcp;

class server : public common::interface<server>
{
  public:
    server(const int a_port, boost::asio::io_service& a_io_service, params_t& a_params)
      : m_server(common::tcp::create_server(a_port, a_io_service, a_params))
      , m_io_service(a_io_service)
      , m_strand(m_io_service)
      , m_params(a_params)
    {
      m_server->set_on_connected([](const int client){
        //std::cout << "connected" << std::endl;
      });

      m_server->set_on_disconnected([](const int client){
        //std::cout << "disconnected" << std::endl;
      });

      m_server->set_on_message([this](const int client, const char *data, std::size_t len){
        //std::cout << data << std::endl;
        if(!m_params.use_strand)
        {
          std::string str{data, len};
          m_strand.wrap([this, str]{
            m_queue.push(str);
          });
        }
        else
        {
          std::lock_guard<std::mutex> lk(m_mutex);
          std::string str{data, len};
          m_queue.push(str);
        }
      });

      m_server->run();
    }
    static server::ref create_server(const int a_port, boost::asio::io_service& a_io_service, params_t& a_params)
    {
      return std::make_shared<server>(a_port, a_io_service, a_params);
    }

  private:
    iserver::ref m_server;
    boost::asio::io_service& m_io_service;
    boost::asio::io_service::strand m_strand;
    params_t& m_params;
    std::queue<std::string> m_queue;
    std::mutex m_mutex;
};

int main(int argc, char **argv)
{
  boost::asio::io_service io_service;

  params_t params;
  params.do_read_type = do_read_type_e::read_until_eol;
  params.use_strand = false;
  params.read_counter = 999999;

  auto serv = server::create_server(9005, io_service, params);
  //io_service.run();
  //return 0;

  std::vector<std::thread> thread_group;
  //int threads_count = std::thread::hardware_concurrency();
  int threads_count = 8;
  for(int i = 0; i < threads_count; i++)
  {
    thread_group.push_back(std::thread([&io_service](){
      io_service.run();
    }));
  }

  for(auto& thread : thread_group)
    thread.join();
  return 0;
}

