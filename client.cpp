#include "client.hpp"

#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <queue>

#include "boost/math/special_functions.hpp"

using namespace std;

namespace asio = boost::asio;

using asio::ip::tcp;



std::ofstream client::log("log.txt");

client::client(const string& host, const string& port)
	: 	io_service_(), socket_(io_service_)
{
	tcp::resolver resolver(io_service_);
	tcp::resolver::query query(host, port);
	tcp::resolver::iterator it = resolver.resolve(query);
	boost::asio::connect(socket_, it);
	io_service_.run();

	id_ = 1; //set to one for debugging, real id is assigned on login
}

void client::send(const MazeCom& msg)
{
	stringstream serializationStream;
	MazeCom_(serializationStream, msg);
	string serializedMsg(serializationStream.str());

	log << "sent:\n" << serializedMsg;

	int msgLength = serializedMsg.length();
	asio::write(socket_, asio::buffer(&msgLength, 4));
	asio::write(socket_, boost::asio::buffer(serializedMsg, msgLength));
}

MazeCom_Ptr client::recv(MazeComType type)
{
	char* buf = new char[4];
	asio::read(socket_, asio::buffer(buf, 4));
	int length = 0;
	length = length | (buf[3] & 0xff);
	length = length << 8;
	length = length | (buf[2] & 0xff);
	length = length << 8;
	length = length | (buf[1] & 0xff);
	length = length << 8;
	length = length | (buf[0] & 0xff);

	delete[] buf;
	buf = new char[length];

	asio::read(socket_, asio::buffer(buf, length));

	stringstream ss;
	for (int i = 0; i < length; ++i)
	{
		ss << buf[i];
	}

	delete[] buf;
	
	string dbg(ss.str());
	log << "recved:\n" << dbg << std::endl;

	MazeCom_Ptr msg(MazeCom_(ss, xml_schema::flags::dont_validate));

	if (msg->mcType() == type)
	{
		return msg;
	} 
	else
	{
		throw msg;
	}
}

void client::login(string& name)
{
	//send login
	MazeCom loginMsg(MazeComType::LOGIN, 0);
	loginMsg.LoginMessage(LoginMessageType(name));
	send(loginMsg);

	//wait for reply
	MazeCom_Ptr r(client::recv(MazeComType::LOGINREPLY));
	id_ = r->id();
}

void client::play()
{
	MazeCom_Ptr moveRequest(recv(MazeComType::AWAITMOVE));
	
	//calculate move
	//TODO algorithm
	//just make any move to check communication
	AwaitMoveMessageType content = *moveRequest->AwaitMoveMessage();
	cardType card = content.board().shiftCard();
	positionType pinPos;
	find_player(content.board(), pinPos);
	positionType movePos(0,1);

	//mock move (don't change pin position, don't turn card, move the first column)
	MoveMessageType moveMsg(movePos, pinPos, card);
	
	//send move
	MazeCom msg(MazeComType::MOVE, id_);
	msg.MoveMessage(moveMsg);
	send(msg);

	//await reply, end
	MazeCom_Ptr accept(recv(MazeComType::ACCEPT));
	std::cout << *accept;
}

void client::find_player(const boardType& b, positionType& p)
{
	p = positionType(0, 0);
	for (boardType::row_const_iterator row_it = b.row().begin(); row_it < b.row().end(); row_it++)
	{				
		p.col() = 0;
		for (boardType::row_type::col_const_iterator col_it = row_it->col().begin(); col_it < row_it->col().end(); col_it++)
		{			
			if (std::find(col_it->pin().playerID().begin(), col_it->pin().playerID().end(), id_) != col_it->pin().playerID().end())
			{
				return;
			}
			p.col() += 1;
		}
		p.row() += 1;
	}
}

void client::find_next_move(const AwaitMoveMessageType& awaitMoveMsg, MoveMessageType& moveMsg) 
{
	
}

void client::expand_board(const boardType& parent, vector<shared_ptr<boardType>>& children)
{
	positionType shiftPos(0,0);
	for ( ; shiftPos.row() < 7; ++shiftPos.row()) 
	{
		int start, step;

		if (shiftPos.row() % 6 == 0) //first and last row: try every second column
		{
			start = 1;
			step = 2;
		} 
		else if (shiftPos.row() % 2 == 1) //every uneven row: try first and last column
		{
			start = 0;
			step = 6;
		}
		else 
		{
			start = 7; //other rows: don't try anything
		}

		for (shiftPos.col() = start; shiftPos.col() < 7; shiftPos.col() += step) 
		{
			if (parent.forbidden().present() && *parent.forbidden() == shiftPos)
			{
				continue;
			}

			cardType shiftCard(parent.shiftCard());

			for (int rotation = 0; rotation < 4; ++rotation)
			{
				rotate(shiftCard);
				shared_ptr<boardType> newBoard(new boardType(parent));
				newBoard->forbidden() = opposite(shiftPos);
				newBoard->shiftCard() = newBoard->row()[newBoard->forbidden()->row()].col()[newBoard->forbidden()->col()];
				newBoard->row()[shiftPos.row()].col()[shiftPos.col()] = shiftCard;

				int direction = -1 * boost::math::sign(shiftPos.col() + shiftPos.row() - 6);
				for (int i = 1; i < 7; ++i) 
				{
					positionType newPos, oldPos;
					
					newPos.col() = shiftPos.col() + direction * i * (shiftPos.row() % 2);
					newPos.row() = shiftPos.row() + direction * i * (shiftPos.col() % 2);
					oldPos.col() = shiftPos.col() + direction * (i - 1) * (shiftPos.row() % 2);
					oldPos.row() = shiftPos.row() + direction * (i - 1) * (shiftPos.col() % 2);

					newBoard->row()[newPos.row()].col()[newPos.col()] = parent.row()[oldPos.row()].col()[oldPos.col()];
				}

				children.push_back(newBoard);
			}
		}
	}
}

void client::expand_pin_positions(const boardType& board, const treasureType& treasure, set<positionType, positionComp>& children)
{
	positionType playerPos;
	find_player(board, playerPos);

	queue<positionType, deque<positionType>> queue;
	queue.push(playerPos);

	while (!queue.empty()) 
	{
		positionType curPos = queue.front();
		queue.pop();

		if (children.find(curPos) == children.end()) 
		{
			children.insert(curPos);
			openings o = board.row()[curPos.row()].col()[curPos.col()].openings();

			if (o.top() && curPos.row() > 0)
			{
				positionType neighbor(curPos.row() - 1, curPos.col());
				if (board.row()[neighbor.row()].col()[neighbor.col()].openings().bottom())
				{
					queue.push(neighbor);
				}
			}

			if (o.bottom() && curPos.row() < 6)
			{
				positionType neighbor(curPos.row() + 1, curPos.col());
				if (board.row()[neighbor.row()].col()[neighbor.col()].openings().top())
				{
					queue.push(neighbor);
				}
			}

			if (o.right() && curPos.col() < 6)
			{
				positionType neighbor(curPos.row(), curPos.col() + 1);
				if (board.row()[neighbor.row()].col()[neighbor.col()].openings().left())
				{
					queue.push(neighbor);
				}
			}

			if (o.left() && curPos.col() > 0)
			{
				positionType neighbor(curPos.row(), curPos.col() - 1);
				if (board.row()[neighbor.row()].col()[neighbor.col()].openings().right())
				{
					queue.push(neighbor);
				}
			}
		}
	}
}

positionType client::opposite(const positionType& pos)
{
	assert((pos.row() % 6 == 0 && pos.col() % 2 == 1) || (pos.col() % 6 == 0 && pos.row() % 2 == 1)); // position is valid shift

	positionType oppPos(pos);
	if (pos.col() % 6 == 0)
	{
		oppPos.col() = abs(oppPos.col() - 6);
	}
	else 
	{
		oppPos.row() = abs(oppPos.row() - 6);
	}

	return oppPos;
}

void client::rotate(cardType& card) 
{
	bool tmp = card.openings().top();
	card.openings().top() = card.openings().left();
	card.openings().left() = card.openings().bottom();
	card.openings().bottom() = card.openings().right();
	card.openings().right() = tmp;
}

void print(ostream& os, boardType& b)
{
	os << "-----------------------------\n";
	for (boardType::row_const_iterator row_it = b.row().begin(); row_it < b.row().end(); row_it++)
	{		
		os << "|";
		for (boardType::row_type::col_const_iterator col_it = row_it->col().begin(); col_it < row_it->col().end(); col_it++)
		{	
			if (col_it->openings().top())
			{
				os << "# #|";
			}
			else 
			{
				os << "###|";
			}
		}
		if (row_it == b.row().begin())
		{
			os << "     ";
			if (b.shiftCard().openings().top())
			{
				os << "# #";
			}
			else 
			{
				os << "###";
			}
		}
		os << "\n|";
		for (boardType::row_type::col_const_iterator col_it = row_it->col().begin(); col_it < row_it->col().end(); col_it++)
		{	
			if (col_it->openings().left())
			{
				os << " ";
			} 
			else
			{
				os << "#";
			}
			if (col_it->treasure().present())
			{
				os << "T";
			} 
			else
			{
				os << " ";
			}
			if (col_it->openings().right())
			{
				os << " |";
			}
			else 
			{
				os << "#|";
			}
		}
		if (row_it == b.row().begin())
		{
			os << "     ";
			if (b.shiftCard().openings().left())
			{
				os << " ";
			} 
			else
			{
				os << "#";
			}
			if (b.shiftCard().treasure().present())
			{
				os << "T";
			} 
			else
			{
				os << " ";
			}
			if (b.shiftCard().openings().right())
			{
				os << " ";
			}
			else 
			{
				os << "#";
			}
		}
		os << "\n|";
		for (boardType::row_type::col_const_iterator col_it = row_it->col().begin(); col_it < row_it->col().end(); col_it++)
		{	
			if (col_it->openings().bottom())
			{
				os << "# #|";
			}
			else 
			{
				os << "###|";
			}
		}
		if (row_it == b.row().begin())
		{
			os << "     ";
			if (b.shiftCard().openings().bottom())
			{
				os << "# #";
			}
			else 
			{
				os << "###";
			}
		}
		os << "\n-----------------------------\n";
	}
}