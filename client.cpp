/*
* class client
* responsible for:
* communication with server
* AI of the game
*/

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
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
using asio::placeholders::error;
using asio::placeholders::bytes_transferred;

ofstream client::log;

client::client(const string& host, const string& port) : _ios(), _socket(_ios) //throws boost::system::system_error
{
	tcp::resolver rslv(_ios);
	tcp::resolver::query qry(host, port);
	asio::connect(_socket, rslv.resolve(qry));	

	_id = 1; // set for testing
}

void client::close()
{
	cout << "Connection closed."<< endl;
	_socket.close();
}

void client::write_body(boost::system::error_code e, size_t, shared_ptr<string> msg)
{
	if (!e)
	{
		asio::write(_socket, asio::buffer(*msg, msg->length()));
	}
	else
	{
		close();
	}
}

void client::login(const string& name)
{
	MazeCom loginMsg(MazeComType::LOGIN, 0);
	loginMsg.LoginMessage(LoginMessageType(name));

	send(loginMsg);
	read_next();

	_ios.run();
}

void client::send(const MazeCom& msg)
{
	stringstream serializationStream;
	MazeCom_(serializationStream, msg);

	shared_ptr<string> serializedMsg(new string(serializationStream.str()));

	int msgLength = serializedMsg->length();
	asio::async_write(_socket, asio::buffer(&msgLength, 4), boost::bind(&client::write_body, this, error, bytes_transferred, serializedMsg));
}

void client::read_next()
{
	shared_ptr<vector<char>> buf(new vector<char>(4, 0));
	asio::async_read(_socket, asio::buffer(*buf, 4), boost::bind(&client::read_body, this,  error, bytes_transferred, buf));
}

void client::read_body(boost::system::error_code, size_t, shared_ptr<vector<char>> buf)
{
	int length = 0;
	length = length | ((*buf)[3] & 0xff);
	length = length << 8;
	length = length | ((*buf)[2] & 0xff);
	length = length << 8;
	length = length | ((*buf)[1] & 0xff);
	length = length << 8;
	length = length | ((*buf)[0] & 0xff);

	buf->clear();
	*buf = vector<char>(length);

	asio::async_read(_socket, asio::buffer(*buf), boost::bind(&client::handle_msg, this, error, bytes_transferred, buf));
}

void client::handle_msg(boost::system::error_code, size_t, shared_ptr<vector<char>> buf)
{
	stringstream deserializationStream;
	for (size_t i = 0; i < buf->size(); ++i)
	{
		deserializationStream << (*buf)[i];
	}

	MazeCom_Ptr msg(MazeCom_(deserializationStream, xml_schema::flags::dont_validate));

	switch (msg->mcType())
	{
	case MazeComType::AWAITMOVE:
		{
			MazeCom rply(MazeComType::MOVE, _id);
			MoveMessageType moveMsg(positionType(0,1), positionType(0,0), msg->AwaitMoveMessage()->board().shiftCard());

			if (_accepted) 
				find_next_move(*msg->AwaitMoveMessage(), moveMsg);
			else 
				default_move(*msg->AwaitMoveMessage(), moveMsg);

			rply.MoveMessage(moveMsg);

			send(rply);
			read_next();
			break;
		}
	case MazeComType::ACCEPT:
		{
			if (!msg->AcceptMessage()->accept())
			{
				_accepted = false;

				cout << "Illegal move detected" << endl
					<< "Reason: " << msg->AcceptMessage()->errorCode() << endl
					<< "Check Log for details" << endl << endl;
			}
			else
			{
				_accepted = true;
			}

			read_next();			
			break;
		}
	case MazeComType::WIN:
		{
			cout << "Game has ended" << endl
				<< "Winner: " << msg->WinMessage()->winner() << endl << endl;
			close();
			break;
		}
	case MazeComType::LOGINREPLY:
		{
			_id = msg->LoginReplyMessage()->newID();
			cout << "Login successful." << endl;
			cout << "ID: " << _id << endl;
			cout << "Playing, don't close this window." << endl << endl;
			read_next();
			break;
		}
	case MazeComType::DISCONNECT:
		{
			cout << "Disconnected by server." << endl
				<< "Reason: " << msg->DisconnectMessage()->erroCode() << endl << endl;
			close();
			break;
		}
	default:
		{
			cout << "Unexpected Message." << endl << endl;
			close();
			break;
		}
	}
}

int client::id() const
{
	return _id;
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

	Coord playerPos = board.find_player(_id);

	int minFreedom = INT_MAX; //minimum possible movement of opponents for each move you make

	vector<Board::ptr> possibleBoards;
	board.expand_shifts(possibleBoards);

	for (size_t i = 0; i < possibleBoards.size(); ++i)
	{		
		vector<Coord> possiblePositions;
		possibleBoards[i]->expand_positions(_id, possiblePositions);

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
	double maxRatio = 0; //maximum ratio of possible boards after the next round where target is reachable
	int minAvgDistance = INT_MAX; //minimum average distance from target after the next round, checked after maxCount

	//create board for every possible position
	vector<Board::ptr> possibleBoardsAndPositions;
	for (size_t i = 0; i < possibleBoards.size(); ++i)
	{
		vector<Coord> positions;
		possibleBoards[i]->expand_positions(_id, positions);

		for (size_t j = 0; j < positions.size(); ++j)
		{
			Board::ptr copy(new Board(*possibleBoards[i]));

			copy->card_at(playerPos)._op[_id - 1] = false; 
			copy->card_at(positions[j])._op[_id - 1] = true;

			possibleBoardsAndPositions.push_back(copy);
		}
	}

	//sort boards for ascending distance from target
	std::sort(
		possibleBoardsAndPositions.begin(), 
		possibleBoardsAndPositions.end(), 
		[this, treasure] (Board::ptr& a, Board::ptr& b) -> bool 
		{
			return this->distance(*a, _id, treasure) < this->distance(*b, _id, treasure);
		});

	double begin = omp_get_wtime();

	//for every board
	//	do probable moves for every opponent
	//	do move for player
	//find board with maximum ratio of positive solutions
	#pragma omp parallel num_threads(omp_get_num_procs()) //omp construct separated from for loop for ordered iteration, otherwise sorting is destroyed
	{
		for (size_t i = omp_get_thread_num(); i < possibleBoardsAndPositions.size(); i+= omp_get_num_threads())
		{
			if (omp_get_wtime() - begin > 15) continue;

			vector<Board::ptr> expand;
			expand.push_back(possibleBoardsAndPositions[i]);

			//moves by opponents
			for (int j = 1; j <= 4; ++j)
			{
				if (j == _id || treasuresToGo[j] < 0) //dont do move for opponents that are not playing and not for the player himself
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
			
			//count number of boards where the treasure is reachable with the players move
			//calculate ratio of boards where treasure is reachable to all possible boards after opponents moves
			double ratio = 0; 
			int count = 0;
			int sumDistance = 0, avgDistance;

			for (size_t j = 0; j < expand.size(); ++j)
			{
				//move by player
				vector<Board::ptr> expand_own_move;
				expand[j]->expand_default_shifts(expand_own_move);
				
				int minDistance = INT_MAX;

				for (size_t j = 0; j < expand_own_move.size(); ++j)
				{
					int distanceFromTreasure = expand_own_move[j]->can_reach(_id, treasure);

					if (distanceFromTreasure < minDistance)
					{
						minDistance = distanceFromTreasure;
					}

					if (minDistance == 0) break;
				}

				sumDistance += minDistance;
				if (minDistance == 0) count++;
			}

			ratio = (double)count / (double)expand.size();
			avgDistance = sumDistance / expand.size();

			#pragma omp critical
			{
				//if new best result: set move message
				//if (avgDistance < minAvgDistance)
				if ((ratio > maxRatio) || (ratio == maxRatio && avgDistance < minAvgDistance))
				{
					maxRatio = ratio;
					minAvgDistance = avgDistance;

					//extract solution from board
					solution.shiftPos = possibleBoardsAndPositions[i]->_forbidden.opposite();
					solution.shiftCard = possibleBoardsAndPositions[i]->card_at(solution.shiftPos);
					solution.shiftCard._op[P1] = false; //the card might contain pins because it is extracted from the board after the shift, 
					solution.shiftCard._op[P2] = false; //but cards with pins as shift card are not accepted
					solution.shiftCard._op[P3] = false;
					solution.shiftCard._op[P4] = false;
					solution.pinPos = possibleBoardsAndPositions[i]->find_player(_id);			
				}
			}
		}
	}

	//fill move message
	moveMsg.shiftPosition() = positionType(solution.shiftPos.row, solution.shiftPos.col);
	moveMsg.newPinPos() = positionType(solution.pinPos.row, solution.pinPos.col);
	moveMsg.shiftCard() = (cardType)solution.shiftCard;
}

void client::default_move(const AwaitMoveMessageType& awaitMoveMsg, MoveMessageType& moveMsg)
{
	struct solution
	{
		Coord pinPos, shiftPos;
		Card shiftCard;
	} solution;
	
	Board b(awaitMoveMsg.board());

	//shift opposite of forbidden, without rotation, don't move pin (except if shifted)
	solution.shiftPos = b._forbidden.opposite();
	solution.shiftCard = b._shift;
	b.shift(solution.shiftPos, solution.shiftCard);
	solution.pinPos = b.find_player(_id);

	moveMsg.shiftPosition() = positionType(solution.shiftPos.row, solution.shiftPos.col);
	moveMsg.newPinPos() = positionType(solution.pinPos.row, solution.pinPos.col);
	moveMsg.shiftCard() = (cardType)solution.shiftCard;
}

int client::distance(const Board& board, int id, Treasure t)
{
	Coord tPos, pPos;
	pPos = board.find_player(id);
	tPos = board.find_treasure(t);

	if (tPos.row >= 0)
		return abs(tPos.row - pPos.row) + abs(tPos.col - pPos.col);
	else 
		return INT_MAX;
}

int client::count_freedom_opponents(const Board& board, const vector<int>& treasuresToGo)
{
	//sum of all treasures of opponents
	int sumTreasures = 1; //sum one greater than actual sum so result is not always zero with one opponent
	for (size_t id = 1; id <= 4; id++)
	{
		if (id != _id && treasuresToGo[id] >= 0)
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
		if (id == _id || treasuresToGo[id] < 0) continue;

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