#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

#include "boost/asio.hpp"
#include "boost/bind.hpp"

#include "mazeCom_.hpp"

using std::string;
using boost::asio::io_service;
using boost::asio::ip::tcp;

typedef std::auto_ptr<MazeCom> MazeCom_Ptr;

class client 
{	
	io_service io_service_;
	tcp::socket socket_;
	MazeCom::id_type id_;

	client(const client&);
	client& operator=(const client&);

public:
	//connect
	client(const string& ip, const string& port);

	//blocking send and recv
	void send(const MazeCom& msg);
	MazeCom_Ptr recv(MazeComType type);

	//start game
	void login(string& name);
	void play();

private:
	MazeCom_Ptr await_move_request();
};


#endif