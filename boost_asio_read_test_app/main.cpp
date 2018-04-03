#include <common/communications.h>
#include <iostream>
#include <thread>

class server : public common::interface<server>
{
  public:
    server(const int a_port, boost::asio::io_service::strand& a_strand, common::tcp::do_read_type_e a_type, bool a_use_strand)
      : m_server(common::tcp::create_server(a_port, a_strand, a_type, a_use_strand))
    {
      m_server->set_on_connected([](const int client){
        //std::cout << "connected" << std::endl;
      });

      m_server->set_on_disconnected([](const int client){
        //std::cout << "disconnected" << std::endl;
      });

      m_server->set_on_message([](const int client, const char *data, std::size_t len){
        //std::cout << data << std::endl;
      });

      m_server->run();
    }
    static server::ref create_server(const int a_port, boost::asio::io_service::strand& a_strand, common::tcp::do_read_type_e a_type, bool a_use_strand)
    {
      return std::make_shared<server>(a_port, a_strand, a_type, a_use_strand);
    }

  private:
    common::tcp::iserver::ref m_server;
};

int main(int argc, char **argv)
{
  boost::asio::io_service io_service;
  boost::asio::io_service::strand strand(io_service);

  auto serv = server::create_server(9005, strand, common::tcp::do_read_type_e::read_until_eol, false);
//  io_service.run();
//  return 0;

  std::vector<std::thread> thread_group;
  //int threads_count = std::thread::hardware_concurrency();
  int threads_count = 3;
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

