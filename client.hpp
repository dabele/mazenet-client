/*
* class client
* responsible for:
* communication with server
* AI of the game
*/

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <fstream>
#include <memory>

#include "boost/asio.hpp"

#include "mazeCom_.hpp"
#include "board.hpp"

using namespace std;

using boost::asio::io_service;
using boost::asio::ip::tcp;

typedef std::auto_ptr<MazeCom> MazeCom_Ptr;

class client 
{	
public:
	//log for messages, only works after login
	static std::ofstream log;
private:
	io_service io_service_;
	tcp::socket socket_;
	MazeCom::id_type id_;

public:
	int id() const;

	//ctor, connects to server
	client(const string& ip, const string& port);

	//blocking send
	void send(const MazeCom& msg);
	//blocking receive, returns msg if type fits, throws msg if type doesn't fit
	MazeCom_Ptr recv(MazeComType type);

	void login(string& name);
	void play();

private:
	void find_next_move(const AwaitMoveMessageType&, MoveMessageType&);
	//weighted possible movement of all opponents, used for determining quality of moves
	int count_freedom_opponents(const Board&, const vector<int>&);
};


#endif