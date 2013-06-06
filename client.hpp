
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

#include "mazeCom.hpp"

#include "boost/asio.hpp"
#include "boost/bind.hpp"

using std::string;
using boost::asio::io_service;
using boost::asio::ip::tcp;

class client 
{
	
	io_service io_service_;
	tcp::socket socket_;

	client(const client&);
	client& operator=(const client&);

public:
	typedef std::auto_ptr<MazeCom> mc_ptr;

	client(const string& ip, const string& port);
	void send(const MazeCom& msg);
	mc_ptr recv();
};


#endif