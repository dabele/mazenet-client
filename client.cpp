/*
* class client
* responsible for:
* communication with server
* AI of the game
*/

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/asio.hpp"
#include "boost/math/special_functions.hpp"

#include "client.hpp"

#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <queue>
#include <omp.h>

using namespace std;

namespace asio = boost::asio;
using asio::ip::tcp;

ofstream client::log;

client::client(const string& host, const string& port)
	: 	io_service_(), socket_(io_service_)
{
	tcp::resolver resolver(io_service_);
	tcp::resolver::query query(host, port);
	tcp::resolver::iterator it = resolver.resolve(query);
	boost::asio::connect(socket_, it);
	io_service_.run();

	id_ = 2; //set to one for debugging, real id is assigned on login
}

void client::send(const MazeCom& msg)
{
	stringstream serializationStream;
	MazeCom_(serializationStream, msg);
	string serializedMsg(serializationStream.str());

	if (log.is_open())
		log << "sent (" << id_ <<"):\n" << serializedMsg << "\n";

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

	if (log.is_open())
	{
		string dbg(ss.str());	
		log << "recved (" << id_ <<"):\n" << dbg << std::endl;
	}

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

	stringstream logname("log");
	logname << id_ << ".txt";
	client::log.open(logname.str());
}

void client::play()
{
	while (true)
	{
		MazeCom_Ptr moveRequest(recv(MazeComType::AWAITMOVE));
	
		//calculate move
		AwaitMoveMessageType content = *moveRequest->AwaitMoveMessage();
		MoveMessageType moveMsg(positionType(0,0), positionType(0,0), content.board().shiftCard());
		find_next_move(content, moveMsg);

		//boost::asio::deadline_timer t(io_service_, boost::posix_time::seconds(10));
		//t.wait();

		//send move
		MazeCom msg(MazeComType::MOVE, id_);
		msg.MoveMessage(moveMsg);
		send(msg);

		//await reply, end
		MazeCom_Ptr accept(recv(MazeComType::ACCEPT));
	}
}

int client::id() const
{
	return id_;
}

void client::find_next_move(const AwaitMoveMessageType& awaitMoveMsg, MoveMessageType& moveMsg) 
{
	/* algorithm:
	check all possible moves

	if treasure is reachable in this round:
		choose move that most limits opponents movements
	else 
		play one more round (one shift for each player), try to cover as many boards as possible (choose positions/orientation of shift card with smaller distance from target first)
		choose move in this round that leads to hightest number of possibilities to reach the treasure next round
	end
	*/

	struct solution
	{
		Coord pinPos, shiftPos;
		Card shiftCard;
	} solution;

	//get content out of msg
	Board board(awaitMoveMsg.board());
	Treasure treasure = Card::conv(awaitMoveMsg.treasure());

	int numPlayers = awaitMoveMsg.treasuresToGo().size();
	vector<int> treasuresToGo(5, -1); //vector of number of treasures for each player, players that are not playing get -1, index in vector is same as id of player (element 0 not used)
	for (int i = 1; i <= 4; ++i)
	{
		for (size_t j = 0; j < awaitMoveMsg.treasuresToGo().size(); ++j)
		{
			if (awaitMoveMsg.treasuresToGo()[j].player() == i)
			{
				treasuresToGo[i] = awaitMoveMsg.treasuresToGo()[j].treasures();
			}
		}
	}

	Coord playerPos = board.find_player(id_);

	int minFreedom = INT_MAX; //minimum possible movement of opponents for each move you make

	vector<Board::ptr> possibleBoards;
	board.expand_shifts(possibleBoards);

	for (size_t i = 0; i < possibleBoards.size(); ++i)
	{		
		vector<Coord> possiblePositions;
		possibleBoards[i]->expand_positions(id_, possiblePositions);

		//check all reachable positions for target
		bool canReachTreasure = false;
		Coord treasurePos;
		for (size_t j = 0; j < possiblePositions.size(); j++)
		{
			if (possibleBoards[i]->card_at(possiblePositions[j])._t == treasure)
			{
				canReachTreasure = true;
				treasurePos = possiblePositions[j];
			}
		}

		if (canReachTreasure)
		{
			int freedom = count_freedom_opponents(*possibleBoards[i], treasuresToGo);

			if (freedom < minFreedom) 
			{
				//save solution if better than previous
				minFreedom = freedom;

				solution.shiftPos = possibleBoards[i]->_forbidden.opposite();
				solution.shiftCard = possibleBoards[i]->card_at(solution.shiftPos);

				solution.shiftCard._op[P1] = false; //the card might contain pins
				solution.shiftCard._op[P2] = false;
				solution.shiftCard._op[P3] = false;
				solution.shiftCard._op[P4] = false;

				solution.pinPos = treasurePos;				
			}
		}
	}

	if (minFreedom < INT_MAX) //solution found that reaches the target
	{
		//build msg
		moveMsg.shiftPosition() = positionType(solution.shiftPos.row, solution.shiftPos.col);
		moveMsg.newPinPos() = positionType(solution.pinPos.row, solution.pinPos.col);
		moveMsg.shiftCard() = (cardType)solution.shiftCard;

		return;
	}

	// solution found that reaches the target : calculate next round
	int maxCount = 0; //maximum of possible boards after the next round where target is reachable
	int minAvgDistance = INT_MAX; //minimum average distance from target after the next round, checked after maxCount

	//select boards for further examination (can't check every board, so number of boards is reduced to probably useful moves)
	possibleBoards.clear();
	board.expand_beneficial_default_shifts(id_, possibleBoards);

	//create board for every possible position
	vector<Board::ptr> possibleBoardsAndPositions;
	for (size_t i = 0; i < possibleBoards.size(); ++i)
	{
		vector<Coord> positions;
		possibleBoards[i]->expand_positions(id_, positions);

		for (size_t j = 0; j < positions.size(); ++j)
		{
			Board::ptr copy(new Board(*possibleBoards[i]));

			copy->card_at(playerPos)._op[id_ - 1] = false; 
			copy->card_at(positions[j])._op[id_ - 1] = true;

			possibleBoardsAndPositions.push_back(copy);
		}
	}

	//sort boards for ascending distance from target
	std::sort(
		possibleBoardsAndPositions.begin(), 
		possibleBoardsAndPositions.end(), 
		[this, treasure] (Board::ptr& a, Board::ptr& b) 
		{
			Coord aT, aP, bT, bP;
			aP = a->find_player(id_);
			bP = b->find_player(id_);
			aT = a->find_treasure(treasure);
			bT = b->find_treasure(treasure);

			if (abs(aT.row - aP.row) + abs(aT.col - aP.col) < abs(bT.row - bP.row) + abs(bT.col - bP.col)) return true;
			return false;
		});

	double begin = omp_get_wtime();

	//for every board
	//	do probable moves for every opponent
	//	do move for player
	//	check distance from target/count number of possible boards where target is reachable
	#pragma omp parallel num_threads(4)
	{
		for (int i = omp_get_thread_num(); i < possibleBoardsAndPositions.size(); i+= omp_get_num_threads())
		{
			if (omp_get_wtime() - begin > 15) continue;

			vector<Board::ptr> expand;
			expand.push_back(possibleBoardsAndPositions[i]);

			//moves by opponents
			for (int j = 1; j <= 4; ++j)
			{
				if (j == id_ || treasuresToGo[j] < 0) //dont do move for opponents that are not playing and the player himself
				{
					continue;
				}

				vector<Board::ptr> expand_next;
				for (size_t k = 0; k < expand.size(); ++k)
				{
					expand[k]->expand_beneficial_default_shifts(j, expand_next);
				}
				expand.swap(expand_next);
				expand_next.clear();
			}

			//move by player
			vector<Board::ptr> expand_own_move;
			for (size_t j = 0; j < expand.size(); ++j)
			{
				expand[j]->expand_default_shifts(expand_own_move);
			}

			//check number of boards that can reach the treasure
			int count = 0;
			int sumDistance = 0, avgDistance;
			for (size_t j = 0; j < expand_own_move.size(); ++j)
			{
				int distanceFromTreasure = expand_own_move[j]->can_reach(id_, treasure);
				if (distanceFromTreasure == 0)
				{
					++count;
				}

				sumDistance += distanceFromTreasure;
			}
			avgDistance = sumDistance / expand_own_move.size();

			#pragma omp critical
			{
				//if new best result: set move message
				//if (avgDistance < minAvgDistance)
				if ((count > maxCount) || (count == maxCount && avgDistance < minAvgDistance))
				{
					maxCount  = count;
					minAvgDistance = avgDistance;

					//extract solution from board
					solution.shiftPos = possibleBoardsAndPositions[i]->_forbidden.opposite();
					solution.shiftCard = possibleBoardsAndPositions[i]->card_at(solution.shiftPos);
					solution.shiftCard._op[P1] = false; //the card might contain pins because it is extracted from the board after the shift, 
					solution.shiftCard._op[P2] = false; //but cards with pins as shift card are not accepted
					solution.shiftCard._op[P3] = false;
					solution.shiftCard._op[P4] = false;
					solution.pinPos = possibleBoardsAndPositions[i]->find_player(id_);			
				}
			}
		}
	}

	//fill move message
	moveMsg.shiftPosition() = positionType(solution.shiftPos.row, solution.shiftPos.col);
	moveMsg.newPinPos() = positionType(solution.pinPos.row, solution.pinPos.col);
	moveMsg.shiftCard() = (cardType)solution.shiftCard;
}

int client::count_freedom_opponents(const Board& board, const vector<int>& treasuresToGo)
{
	//sum of all treasures of opponents
	int sumTreasures = 1; //sum one greater than actual sum so result is not always zero with one opponent
	for (size_t id = 1; id <= 4; id++)
	{
		if (id != id_ && treasuresToGo[id] >= 0)
		{
			sumTreasures += treasuresToGo[id];
		}
	}

	int sumFreedom = 0;

	//for every player:
	//get number of possible positions
	//weigh by treasures to go
	for (size_t id = 1; id <= 4; id++)
	{
		if (id == id_ || treasuresToGo[id] < 0) continue;

		vector<Coord> positions;
		board.expand_positions(id, positions);

		int numTreasures = treasuresToGo[id];
		int numPositions = positions.size();

		sumFreedom += (sumTreasures - numTreasures) * numPositions; //opponents with lower treasures to go are weighed higher
	}

	return sumFreedom;
}

void print(ostream& os, boardType& b)
{
	for (boardType::row_const_iterator row_it = b.row().begin(); row_it < b.row().end(); row_it++)
	{				
		//oberes drittel
		os << "|";
		for (boardType::row_type::col_const_iterator col_it = row_it->col().begin(); col_it < row_it->col().end(); col_it++)
		{	
			if (col_it->openings().top())
			{
				os << ">>>   <<<|";
			}
			else 
			{
				os << ">>>>-<<<<|";
			}
		}

		if (row_it == b.row().begin())
		{
			if (b.shiftCard().openings().top())
			{
				os << "###   ###";
			}
			else 
			{
				os << "#########";
			}
		}
		os << "\n";
		os << "|";
		for (boardType::row_type::col_const_iterator col_it = row_it->col().begin(); col_it < row_it->col().end(); col_it++)
		{	
			if (col_it->openings().top())
			{
				os << "###   ###|";
			}
			else 
			{
				os << "#########|";
			}
		}

		if (row_it == b.row().begin())
		{
			if (b.shiftCard().openings().top())
			{
				os << "###   ###";
			}
			else 
			{
				os << "#########";
			}
		}
		os << "\n";
		/*for (int i = 0; i < 2; ++i)
		{
			os << "|";
			for (boardType::row_type::col_const_iterator col_it = row_it->col().begin(); col_it < row_it->col().end(); col_it++)
			{	
				if (col_it->openings().top())
				{
					os << "###   ###|";
				}
				else 
				{
					os << "#########|";
				}
			}

			if (row_it == b.row().begin())
			{
				if (b.shiftCard().openings().top())
				{
					os << "###   ###";
				}
				else 
				{
					os << "#########";
				}
			}
			os << "\n";
		}		*/

		//mittleres drittel
		//erste zeile
		os << "|";
		for (boardType::row_type::col_const_iterator col_it = row_it->col().begin(); col_it < row_it->col().end(); col_it++)
		{				
			if (col_it->openings().left())
			{
				os << "   ";
			} 
			else
			{
				os << "###";
			}

			if (find(col_it->pin().playerID().begin(), col_it->pin().playerID().end(), 1) != col_it->pin().playerID().end())
			{
				os << "1";
			} 
			else
			{
				os << " ";
			}

			if (col_it->treasure().present())
			{
				os << "T";
			}
			else
			{
				os << " ";
			}

			if (find(col_it->pin().playerID().begin(), col_it->pin().playerID().end(), 2) != col_it->pin().playerID().end())
			{
				os << "2";
			} 
			else
			{
				os << " ";
			}

			if (col_it->openings().right())
			{
				os << "   |";
			}
			else 
			{
				os << "###|";
			}
		}
		if (row_it == b.row().begin())
		{
			if (b.shiftCard().openings().left())
			{
				os << "   ";
			} 
			else
			{
				os << "###";
			}

			if (b.shiftCard().treasure().present())
			{
				os << " T ";
			}
			else
			{
				os << "   ";
			}

			if (b.shiftCard().openings().right())
			{
				os << "   ";
			}
			else 
			{
				os << "###";
			}
		}
		os << "\n|";

		//zweite zeile
		for (boardType::row_type::col_const_iterator col_it = row_it->col().begin(); col_it < row_it->col().end(); col_it++)
		{	
			if (col_it->openings().left())
			{
				os << "   ";
			} 
			else
			{
				os << "###";
			}

			if (find(col_it->pin().playerID().begin(), col_it->pin().playerID().end(), 3) != col_it->pin().playerID().end())
			{
				os << "3";
			} 
			else
			{
				os << " ";
			}
			os << " ";
			if (find(col_it->pin().playerID().begin(), col_it->pin().playerID().end(), 4) != col_it->pin().playerID().end())
			{
				os << "4";
			}
			else 
			{
				os << " ";
			}


			if (col_it->openings().right())
			{
				os << "   |";
			}
			else 
			{
				os << "###|";
			}
		}
		if (row_it == b.row().begin())
		{
			if (b.shiftCard().openings().left())
			{
				os << "   ";
			} 
			else
			{
				os << "###";
			}
			os << "   ";
			if (b.shiftCard().openings().right())
			{
				os << "   ";
			}
			else 
			{
				os << "###";
			}
		}
		os << "\n";

		//unteres drittel
		for (int i = 0; i < 2; ++i)
		{
			os << "|";
			for (boardType::row_type::col_const_iterator col_it = row_it->col().begin(); col_it < row_it->col().end(); col_it++)
			{	
				if (col_it->openings().bottom())
				{
					os << "###   ###|";
				}
				else 
				{
					os << "#########|";
				}
			}
			if (row_it == b.row().begin())
			{
				if (b.shiftCard().openings().bottom())
				{
					os << "###   ###";
				}
				else 
				{
					os << "#########";
				}
			}
			os << "\n";
		}
		
		//os << "-----------------------------------------------------------------------\n";
	}
}