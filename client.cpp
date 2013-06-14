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

void client::find_next_move(const AwaitMoveMessageType& awaitMoveMsg, MoveMessageType& moveMsg) 
{
	/* algorithm:
	check all possible moves

	if treasure is reachable in this round:
		choose move that most limits opponents movements
	else 
		play one more round (one shift for each player), try to cover as many boards as possible (choose positions/orientation of shift card randomly)
		choose move in this round that leads to hightest number of possibilities to reach the treasure next round
	end
	*/

	//number of positions all opponents can do, weighted by the treasures they still need to find (limiting better players is more important)
	int minFreedom = INT_MAX;

	int numPlayers = awaitMoveMsg.treasuresToGo().size();
	vector<pBoardType> possibleBoards;
	expand_board(awaitMoveMsg.board(), possibleBoards);

	vector<pBoardType> possibleBoardsAndPositions;

	for (int i = 0; i < possibleBoards.size(); ++i)
	{
		vector<pBoardType> possiblePositions;

		if (expand_pin_positions(*possibleBoards[i], awaitMoveMsg.treasure(), possiblePositions)) //treasure is reachable on this board
		{			
			//determine sum of possible movements for opponents
			int freedom = count_freedom_opponents(*possibleBoards[i], awaitMoveMsg.treasuresToGo());

			if (freedom < minFreedom) 
			{
				minFreedom = freedom;

				positionType shiftPos, pinPos;
				cardType shiftCard;
			
				shiftPos = opposite(*possiblePositions[i]->forbidden());
				find_player(*possiblePositions[i], pinPos);
				shiftCard = possiblePositions[i]->row()[shiftPos.row()].col()[shiftPos.col()];

				moveMsg.shiftPosition() = shiftPos;
				moveMsg.newPinPos() = pinPos;
				moveMsg.shiftCard() = shiftCard;
			}
		}
		else //treasure is not reachable in this board, insert boards with possible pin positions into vector for further examination
		{
			//only copy if solution hasn't already been found
			if (minFreedom == INT_MAX)
			{
				possibleBoardsAndPositions.insert(possibleBoardsAndPositions.end(), possiblePositions.begin(), possiblePositions.end());
			}
		}
	}

	if (minFreedom < INT_MAX) 
	{
		return; //solution has been found, move is already in moveMsg
	}

	//determine next round
	int maxCount = 0;

	for (int i = 0; i < possibleBoardsAndPositions.size(); i++)
	{
		//move by first opponent (or self if only one player)
		vector<pBoardType> expand;
		expand_board(*possibleBoardsAndPositions[i], expand);

		//move by following opponents
		for (int j = 1; j < numPlayers; ++j)
		{
			vector<pBoardType> expand_next;
			srand(time(NULL));
			for (int k = 0; k < expand.size(); k += 4)
			{
				int offset = rand()%4;
				expand_board(*expand[k + offset], expand_next);
			}
			expand.swap(expand_next);
			expand_next.clear();
		}

		//check number of boards that can reach the treasure
		int count = 0;
		for (int j = 0; j < expand.size(); ++j)
		{
			if (can_reach(*expand[i], awaitMoveMsg.treasure()))
			{
				++count;
			}
		}

		//if new best result: set move message
		if (count > maxCount) 
		{
			positionType shiftPos, pinPos;
			cardType shiftCard;
			
			shiftPos = opposite(*possibleBoardsAndPositions[i]->forbidden());
			find_player(*possibleBoardsAndPositions[i], pinPos);
			shiftCard = possibleBoardsAndPositions[i]->row()[shiftPos.row()].col()[shiftPos.col()];

			moveMsg.shiftPosition() = shiftPos;
			moveMsg.newPinPos() = pinPos;
			moveMsg.shiftCard() = shiftCard;
		}
	}
}

void client::expand_board(const boardType& parent, vector<pBoardType>& children)
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
			int nPossibleOrientations;
			if ((shiftCard.openings().top() && shiftCard.openings().bottom() && !shiftCard.openings().left() && !shiftCard.openings().right()) || 
				(!shiftCard.openings().top() && !shiftCard.openings().bottom() && shiftCard.openings().left() && shiftCard.openings().right()))
			{
				nPossibleOrientations = 2;
			} 
			else
			{
				nPossibleOrientations = 4;
			}

			for (int rotation = 0; rotation < nPossibleOrientations; ++rotation)
			{
				rotate(shiftCard);
				pBoardType newBoard(new boardType(parent));
				shift(*newBoard, shiftPos, shiftCard);
				children.push_back(newBoard);
			}
		}
	}
}

bool client::expand_pin_positions(const boardType& board, const treasureType& treasure, vector<pBoardType>& children)
{
	positionType playerPos;
	find_player(board, playerPos);

	queue<positionType, deque<positionType>> queue;
	queue.push(playerPos);
	set<positionType, positionComp> used;

	while (!queue.empty()) 
	{
		positionType curPos = queue.front();
		queue.pop();

		if (used.find(curPos) == used.end()) 
		{
			//copy board
			pBoardType b(new boardType(board));

			//remove player pin from player position
			cardType* oldPosCard = &b->row()[playerPos.row()].col()[playerPos.col()];
			for (pin::playerID_iterator pin = oldPosCard->pin().playerID().begin(); pin < oldPosCard->pin().playerID().end(); ++pin)
			{
				if (*pin == id_)
				{
					oldPosCard->pin().playerID().erase(pin);
					break;
				}
			}

			//add player pin to current position
			cardType* curPosCard = &b->row()[curPos.row()].col()[curPos.col()];
			curPosCard->pin().playerID().push_back(id_);

			//if treasure is found, delete all other expanded boards, add current board and return
			if (curPosCard->treasure().present() && *curPosCard->treasure() == treasure)
			{
				children.clear();
				children.push_back(b);
				return true;
			}

			//add board to children
			children.push_back(b);

			//mark position as used to not expand it again
			used.insert(curPos);

			//check/enqueue neighbords
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

	return false;
}

void client::shift(boardType& board, const positionType& shiftPos, const cardType& shiftCard) 
{
	board.forbidden() = opposite(shiftPos);

	//card at position that is forbidden in next move is new shift card
	board.shiftCard() = board.row()[board.forbidden()->row()].col()[board.forbidden()->col()];	

	//direction of shift (left to right or top to bottom is 1, right to left or bottom to top is -1)
	int direction = -1 * boost::math::sign(shiftPos.col() + shiftPos.row() - 6); 

	//shift cards on the board (descending i so no temporary objects are needed)
	for (int i = 6; i >= 1; --i) 
	{
		positionType newPos, oldPos;
					
		newPos.col() = shiftPos.col() + direction * i * (shiftPos.row() % 2);
		newPos.row() = shiftPos.row() + direction * i * (shiftPos.col() % 2);
		oldPos.col() = shiftPos.col() + direction * (i - 1) * (shiftPos.row() % 2);
		oldPos.row() = shiftPos.row() + direction * (i - 1) * (shiftPos.col() % 2);

		board.row()[newPos.row()].col()[newPos.col()] = board.row()[oldPos.row()].col()[oldPos.col()];
	}

	//set given shift card at shift position
	board.row()[shiftPos.row()].col()[shiftPos.col()] = shiftCard;

	//switch all pins from new shift card to shift position
	board.row()[shiftPos.row()].col()[shiftPos.col()].pin().playerID().insert(
		board.row()[shiftPos.row()].col()[shiftPos.col()].pin().playerID().begin(),
		board.shiftCard().pin().playerID().begin(),
		board.shiftCard().pin().playerID().end());
	board.shiftCard().pin().playerID().clear();
}

int client::count_freedom_opponents(const boardType& board, const AwaitMoveMessageType::treasuresToGo_sequence& treasuresToGo)
{
	if (treasuresToGo.size() == 0)
	{
		return INT_MAX;
	}

	int sumTreasures = 1; //sum one greater than actual sum so result is not always zero with one opponent
	for (AwaitMoveMessageType::treasuresToGo_const_iterator player = treasuresToGo.begin(); player != treasuresToGo.end(); ++player)
	{
		if (player->player() != id_)
		{
			sumTreasures += player->treasures();
		}
	}

	int sumFreedom = 0;

	for (AwaitMoveMessageType::treasuresToGo_const_iterator player = treasuresToGo.begin(); player != treasuresToGo.end(); ++player)
	{
		int id = player->player();
		int numTreasures = player->treasures();
		int numPositions = 0;

		if (id == id_)
		{
			continue;
		}

		//count possible positions of the player using breadth first search
		positionType playerPos;
		find_player(board, id, playerPos);

		queue<positionType, deque<positionType>> queue;
		queue.push(playerPos);
		set<positionType, positionComp> used;

		while (!queue.empty()) 
		{
			positionType curPos = queue.front();
			queue.pop();

			if (used.find(curPos) == used.end()) 
			{
				++numPositions;

				//mark position as used to not expand it again
				used.insert(curPos);

				//check/enqueue neighbords
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

		sumFreedom += (sumTreasures - numTreasures) * numPositions;
	}

	return sumFreedom;
}

void client::find_player(const boardType& b, positionType& p) 
{
	find_player(b, id_, p);
}

void client::find_player(const boardType& b, int id, positionType& p)
{
	p = positionType(0, 0);
	for (boardType::row_const_iterator row_it = b.row().begin(); row_it < b.row().end(); row_it++)
	{				
		p.col() = 0;
		for (boardType::row_type::col_const_iterator col_it = row_it->col().begin(); col_it < row_it->col().end(); col_it++)
		{			
			if (std::find(col_it->pin().playerID().begin(), col_it->pin().playerID().end(), id) != col_it->pin().playerID().end())
			{
				return;
			}
			p.col() += 1;
		}
		p.row() += 1;
	}
}

bool client::can_reach(const boardType& board, const treasureType& t)
{
	positionType playerPos;
	find_player(board, playerPos);

	queue<positionType, deque<positionType>> queue;
	set<positionType, positionComp> used;

	queue.push(playerPos);

		while (!queue.empty()) 
	{
		positionType curPos = queue.front();
		queue.pop();

		if (used.find(curPos) == used.end()) 
		{
			//mark position as used to not expand it again
			used.insert(curPos);

			//if treasure is found, delete all other expanded boards, add current board and return
			if (board.row()[curPos.row()].col()[curPos.col()].treasure().present() && *board.row()[curPos.row()].col()[curPos.col()].treasure() == t)
			{
				return true;
			}
			
			//check/enqueue neighbords
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

	return false;
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