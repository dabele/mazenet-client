#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <fstream>
#include <memory>
#include <vector>
#include <set>

#include "boost/asio.hpp"
#include "boost/bind.hpp"

#include "mazeCom_.hpp"

using namespace std;

using boost::asio::io_service;
using boost::asio::ip::tcp;

typedef std::auto_ptr<MazeCom> MazeCom_Ptr;

class client 
{	
public:
	struct positionComp 
	{
		bool operator()(const positionType& p1, const positionType& p2)
		{
			if (p1.row() < p2.row()) 
			{
				return true;
			}
			else if (p1.row() == p2.row()) 
			{
				if (p1.col() < p2.col())
				{
					return true;
				}
			}
			return false;
		}
	};

	static std::ofstream log;	

	typedef shared_ptr<boardType> pBoardType;
private:
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

//private:
	void find_player(const boardType&, positionType&);
	void find_next_move(const AwaitMoveMessageType&, MoveMessageType&);
	void expand_board(const boardType&, vector<pBoardType>&); //all possible shift results
	bool expand_pin_positions(const boardType&, const treasureType&, vector<pBoardType>&); //all possible pin positions without shifting

	//help functions
	void rotate(cardType&);
	positionType opposite(const positionType&);
};


#endif