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
	io_service _ios;
	tcp::socket _socket;
	MazeCom::id_type _id;
	bool _accepted;

public:
	int id() const;

	//ctor, connects to server
	client(const string& ip, const string& port);

	void find_next_move(const AwaitMoveMessageType&, MoveMessageType&);
	void default_move(const AwaitMoveMessageType&, MoveMessageType&);
	//distance of player with id to card with treasure t
	int distance(const Board& board, int id, Treasure t);
	//weighted possible movement of all opponents, used for determining quality of moves
	int count_freedom_opponents(const Board&, const vector<int>&);

	void write_body(boost::system::error_code e, size_t, shared_ptr<string> msg);
	void close();
	void login(const string&);
	void send(const MazeCom&);
	void start_recv();
	void read_next();
	void read_body(boost::system::error_code, size_t, shared_ptr<vector<char>>);
	void handle_msg(boost::system::error_code, size_t, shared_ptr<vector<char>>);
};


#endif