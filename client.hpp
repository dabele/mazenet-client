#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <fstream>

#include "boost/asio.hpp"
#include "boost/bind.hpp"

#include "mazeCom_.hpp"

using std::string;
using boost::asio::io_service;
using boost::asio::ip::tcp;

typedef std::auto_ptr<MazeCom> MazeCom_Ptr;

class client 
{	
	static std::ofstream log;

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
	void find_player(const boardType&, positionType&);
	void find_next_move(const AwaitMoveMessageType&, MoveMessageType&);
	void rotate(cardType&);
};


#endif