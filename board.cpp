/*
* more efficient way to store boards than generated types by code synthesis
*/

#include "boost/math/special_functions.hpp"

#include <queue>
#include <set>

#include "board.hpp"

using namespace std;

/////////////////////
//////  Coord  //////
/////////////////////

Coord Coord::opposite() const
{
	Coord c(*this);
	if (c.col % 6 == 0)
	{
		c.col = abs(c.col - 6);
	}
	else
	{
		c.row = abs(c.row - 6);
	}
	return c;
}

bool Coord::operator==(const Coord& arg) const
{
	return row == arg.row && col == arg.col;
}

bool Coord::operator<(const Coord& arg) const
{
	if (row < arg.row)
	{
		return true;
	} 
	else
	{
		if (row == arg.row && col < arg.col) 
		{
			return true;
		}
	}

	return false;
}

/////////////////////
//////   Card  //////
/////////////////////

Card::Card (const cardType& c) : _t(conv(c.treasure()))
{
	_op[P1] = std::find(c.pin().playerID().begin(), c.pin().playerID().end(), 1) != c.pin().playerID().end();
	_op[P2] = std::find(c.pin().playerID().begin(), c.pin().playerID().end(), 2) != c.pin().playerID().end();
	_op[P3] = std::find(c.pin().playerID().begin(), c.pin().playerID().end(), 3) != c.pin().playerID().end();
	_op[P4] = std::find(c.pin().playerID().begin(), c.pin().playerID().end(), 4) != c.pin().playerID().end();
	_op[T] = c.openings().top();
	_op[B] = c.openings().bottom();
	_op[L] = c.openings().left();
	_op[R] = c.openings().right();
}

void Card::rotate()
{
	bool tmp = _op[T];
	_op[T] = _op[L];
	_op[L] = _op[B];
	_op[B] = _op[R];
	_op[R] = tmp;
}

vector<Card> Card::default_rotations(const Coord& shiftPos) const
{
	vector<Card> rotatedCards;

	int type; //0 = T, 1 = I, 2 = L
	if ((this->_op[B] && this->_op[T] && (this->_op[R] || this->_op[L])) ||
		(this->_op[R] && this->_op[L] && (this->_op[T] || this->_op[B])))
	{
		type = 0;
	}
	else if ((this->_op[B] && this->_op[T]) || (this->_op[R] && this->_op[L]))
	{
		type = 1;
	}
	else
	{
		type = 2;
	}
			
	if (shiftPos.row == 0) //top to bottom
	{
		switch (type)
		{
		case 0:
			{
			Card card(*this);
			card._op[T] = false;
			card._op[L] = true;
			card._op[R] = true;
			card._op[B] = true;
			rotatedCards.push_back(card);
			break;
			}
		case 1:
			{
			Card card(*this);
			card._op[T] = false;
			card._op[L] = true;
			card._op[R] = true;
			card._op[B] = false;
			rotatedCards.push_back(card);
			break;
			}
		case 2:
			{
			Card card1(*this);
			card1._op[T] = false;
			card1._op[L] = true;
			card1._op[R] = false;
			card1._op[B] = true;
			Card card2(*this);
			card2._op[T] = false;
			card2._op[L] = false;
			card2._op[R] = true;
			card2._op[B] = true;
			rotatedCards.push_back(card1);
			rotatedCards.push_back(card2);
			break;
			}
		}
	}
	else if (shiftPos.row == 6) //bottom to top
	{
		switch (type)
		{
		case 0:
			{
			Card card(*this);
			card._op[T] = true;
			card._op[L] = true;
			card._op[R] = true;
			card._op[B] = false;
			rotatedCards.push_back(card);
			break;
			}
		case 1:
			{
			Card card(*this);
			card._op[T] = false;
			card._op[L] = true;
			card._op[R] = true;
			card._op[B] = false;
			rotatedCards.push_back(card);
			break;
			}
		case 2:
			{
			Card card1(*this);
			card1._op[T] = true;
			card1._op[L] = true;
			card1._op[R] = false;
			card1._op[B] = false;
			Card card2(*this);
			card2._op[T] = true;
			card2._op[L] = false;
			card2._op[R] = true;
			card2._op[B] = false;
			rotatedCards.push_back(card1);
			rotatedCards.push_back(card2);
			break;
			}
		}
	}
	else if (shiftPos.col == 0) //left to right
	{
		switch (type)
		{
		case 0:
			{
			Card card(*this);
			card._op[T] = true;
			card._op[L] = false;
			card._op[R] = true;
			card._op[B] = true;
			rotatedCards.push_back(card);
			break;
			}
		case 1:
			{
			Card card(*this);
			card._op[T] = true;
			card._op[L] = false;
			card._op[R] = false;
			card._op[B] = true;
			rotatedCards.push_back(card);
			break;
			}
		case 2:
			{
			Card card1(*this);
			card1._op[T] = false;
			card1._op[L] = false;
			card1._op[R] = true;
			card1._op[B] = true;
			Card card2(*this);
			card2._op[T] = true;
			card2._op[L] = false;
			card2._op[R] = true;
			card2._op[B] = false;
			rotatedCards.push_back(card1);
			rotatedCards.push_back(card2);
			break;
			}
		}
	}
	else if (shiftPos.col == 6) //left to right
	{
		switch (type)
		{
		case 0:
			{
			Card card(*this);
			card._op[T] = true;
			card._op[L] = true;
			card._op[R] = false;
			card._op[B] = true;
			rotatedCards.push_back(card);
			break;
			}
		case 1:
			{
			Card card(*this);
			card._op[T] = true;
			card._op[L] = false;
			card._op[R] = false;
			card._op[B] = true;
			rotatedCards.push_back(card);
			break;
			}
		case 2:
			{
			Card card1(*this);
			card1._op[T] = false;
			card1._op[L] = true;
			card1._op[R] = false;
			card1._op[B] = true;
			Card card2(*this);
			card2._op[T] = true;
			card2._op[L] = true;
			card2._op[R] = false;
			card2._op[B] = false;
			rotatedCards.push_back(card1);
			rotatedCards.push_back(card2);
			break;
			}
		}
	}

	return rotatedCards;
}

Treasure Card::conv(const cardType::treasure_optional& t)
{
	if (t.present())
	{
		return conv(*t);
	}
	else
	{
		return t0;
	}
}

Treasure Card::conv(const treasureType::value t)
{
	return static_cast<Treasure>(t);
}

void Card::conv(Treasure t, cardType& card)
{
	if (t != t0)
	{
		card.treasure() = treasureType(static_cast<treasureType::value>(t));
	}
}

Card::operator cardType()
{
	openings o(_op[T], _op[B], _op[L], _op[R]);
		
	cardType::pin_type pin;

	if (_op[P1]) pin.playerID().push_back(1);
	if (_op[P2]) pin.playerID().push_back(2);
	if (_op[P3]) pin.playerID().push_back(3);
	if (_op[P4]) pin.playerID().push_back(4);

	cardType card(o, pin);
	conv(_t, card);
	return card;
}

/////////////////////
//////  Board  //////
/////////////////////

Board::Board() : _card(SIZE, vector<Card>(SIZE, Card()))
{
	_forbidden = Coord(0,0);
	_shift._t = t0;
	_shift._op = std::bitset<8>(rand() % 256);

	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < SIZE; ++j)
		{
			_card[i][j]._t = static_cast<Treasure>(rand()%29);
			_card[i][j]._op = std::bitset<8>(rand() % 256);
		}
	}
}

Board::Board(const boardType& b) : _shift(b.shiftCard()), _card(SIZE, vector<Card>(SIZE, Card()))
{
	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < SIZE; ++j)
		{
			_card[i][j] = Card(b.row()[i].col()[j]);
		}
	}

	if (b.forbidden().present())
	{
		_forbidden = Coord(b.forbidden()->row(), b.forbidden()->col());
	} 
	else
	{
		_forbidden = Coord(0,0);
	}
}

Board::Board(const Board& b) : _shift(b._shift), _forbidden(b._forbidden), _card(b._card)
{
}

Board::~Board()
{
}

Board& Board::operator=(const Board& b)
{
	_card = b._card;
	_forbidden = b._forbidden;
	_shift = b._shift;

	return *this;
}

void Board::shift(const Coord& shiftPos, const Card& shiftCard)
{
	_forbidden = shiftPos.opposite();

	//card at position that is forbidden in next move is new shift card
	_shift = card_at(_forbidden);

	//direction of shift (left to right or top to bottom is 1, right to left or bottom to top is -1)
	int direction = -1 * boost::math::sign(shiftPos.col + shiftPos.row - 6);

	//shift cards on the board (descending i so no temporary objects are needed)
	for (int i = 6; i >= 1; --i) 
	{
		Coord newPos, oldPos;
					
		newPos.col = shiftPos.col + direction * i * (shiftPos.row % 2);
		newPos.row = shiftPos.row + direction * i * (shiftPos.col % 2);
		oldPos.col = shiftPos.col + direction * (i - 1) * (shiftPos.row % 2);
		oldPos.row = shiftPos.row + direction * (i - 1) * (shiftPos.col % 2);

		card_at(newPos) = card_at(oldPos);
	}

	card_at(shiftPos) = shiftCard;

	//switch all pins from new shift card to shift position
	if (_shift._op[P1] || _shift._op[P2] || _shift._op[P3] || _shift._op[P4])
	{
		card_at(shiftPos)._op[P1] = _shift._op[P1];
		card_at(shiftPos)._op[P2] = _shift._op[P2];
		card_at(shiftPos)._op[P3] = _shift._op[P3];
		card_at(shiftPos)._op[P4] = _shift._op[P4];
		_shift._op[P1] = false;
		_shift._op[P2] = false;
		_shift._op[P3] = false;
		_shift._op[P4] = false;
	}
}

void Board::expand_shifts(vector<ptr>& children) const
{
	Coord shiftPos(0,0);
	for ( ; shiftPos.row < 7; ++shiftPos.row) 
	{
		int start, step;

		if (shiftPos.row % 6 == 0) //first and last row: try every second column
		{
			start = 1;
			step = 2;
		} 
		else if (shiftPos.row % 2 == 1) //every uneven row: try first and last column
		{
			start = 0;
			step = 6;
		}
		else 
		{
			start = 7; //other rows: don't try anything
		}

		for (shiftPos.col = start; shiftPos.col < 7; shiftPos.col += step) 
		{
			if (_forbidden == shiftPos)
			{
				continue;
			}

			Card shiftCard(_shift);
			int nPossibleOrientations;
			if ((shiftCard._op[T] && shiftCard._op[B] && !shiftCard._op[L] && !shiftCard._op[R]) || 
				(!shiftCard._op[T] && !shiftCard._op[B] && shiftCard._op[L] && shiftCard._op[R]))
			{
				nPossibleOrientations = 2;
			}
			else
			{
				nPossibleOrientations = 4;
			}

			for (int rotation = 0; rotation < nPossibleOrientations; ++rotation)
			{
				shiftCard.rotate();
				ptr newBoard(new Board(*this));
				newBoard->shift(shiftPos, shiftCard);
				children.push_back(newBoard);
			}
		}
	}
}

void Board::expand_beneficial_default_shifts(int id, vector<ptr>& children) const 
{
	set<Coord> positionsThisBoard;
	set<Treasure> treasuresThisBoard;

	vector<Coord> p;
	expand_positions(id, p);

	for (size_t i = 0; i < p.size(); ++i)
	{
		positionsThisBoard.insert(p[i]);
		treasuresThisBoard.insert(card_at(p[i])._t);
	}
	treasuresThisBoard.insert(t0);

	for (int i = 0; i < 4; ++i) //add start symbols for other players, because they are not beneficial to this player
	{
		if (i + 1 != id)
		{
			treasuresThisBoard.insert(static_cast<Treasure>(i));
		}
	}

	vector<ptr> allShifts;
	expand_default_shifts(allShifts);

	for (size_t i = 0; i < allShifts.size(); i++)
	{
		vector<Coord> positionsCurBoard;
		allShifts[i]->expand_positions(id, positionsCurBoard);
		bool beneficial = false;

		for (size_t j = 0; j < positionsCurBoard.size(); j++)
		{
			if (positionsThisBoard.find(positionsCurBoard[j]) == positionsThisBoard.end() || treasuresThisBoard.find(card_at(positionsCurBoard[j])._t) == treasuresThisBoard.end())
			{
				beneficial = true;
				break;
			}
		}

		if (beneficial)
		{
			children.push_back(allShifts[i]);
		}
	}

	//add some random moves to get a more significant number of boards
	while (children.size() < 5)
	{
		Board::ptr r = allShifts[rand() % allShifts.size()];
		if (std::find(children.begin(), children.end(), r)  == children.end())
		{
			children.push_back(r);
		}
	}
}

void Board::expand_beneficial_shifts(int id, vector<ptr>& children) const 
{
	set<Coord> positionsThisBoard;
	set<Treasure> treasuresThisBoard;

	vector<Coord> p;
	expand_positions(id, p);

	for (size_t i = 0; i < p.size(); ++i)
	{
		positionsThisBoard.insert(p[i]);
		treasuresThisBoard.insert(card_at(p[i])._t);
	}
	treasuresThisBoard.insert(t0);

	for (int i = 0; i < 4; ++i) //add start symbols for other players, because they are not beneficial to this player
	{
		if (i + 1 != id)
		{
			treasuresThisBoard.insert(static_cast<Treasure>(i));
		}
	}

	vector<ptr> allShifts;
	expand_shifts(allShifts);

	for (size_t i = 0; i < allShifts.size(); i++)
	{
		vector<Coord> positionsCurBoard;
		allShifts[i]->expand_positions(id, positionsCurBoard);
		bool beneficial = false;

		for (size_t j = 0; j < positionsCurBoard.size(); j++)
		{
			if (positionsThisBoard.find(positionsCurBoard[j]) == positionsThisBoard.end() || treasuresThisBoard.find(card_at(positionsCurBoard[j])._t) == treasuresThisBoard.end())
			{
				beneficial = true;
				break;
			}
		}

		if (beneficial)
		{
			children.push_back(allShifts[i]);
		}
	}
	
	//add some random moves to get a significant number of boards
	while (children.size() < 10)
	{
		Board::ptr r = allShifts[rand() % allShifts.size()];
		if (std::find(children.begin(), children.end(), r)  == children.end())
		{
			children.push_back(r);
		}
	}
}

void Board::expand_default_shifts(vector<ptr>& children) const
{
	Coord shiftPos(0,0);
	for ( ; shiftPos.row < SIZE; ++shiftPos.row) 
	{
		int start, step;

		if (shiftPos.row % 6 == 0) //first and last row: try every second column
		{
			start = 1;
			step = 2;
		} 
		else if (shiftPos.row % 2 == 1) //every uneven row: try first and last column
		{
			start = 0;
			step = SIZE - 1;
		}
		else 
		{
			start = SIZE; //other rows: don't try anything
		}

		for (shiftPos.col = start; shiftPos.col < SIZE; shiftPos.col += step) 
		{
			if (_forbidden == shiftPos)
			{
				continue;
			}

			vector<Card> defaultRotations = _shift.default_rotations(shiftPos);

			for (size_t rotation = 0; rotation < defaultRotations.size(); ++rotation)
			{
				ptr newBoard(new Board(*this));
				newBoard->shift(shiftPos, defaultRotations[rotation]);
				children.push_back(newBoard);
			}
		}
	}
}

void Board::expand_positions(int id, vector<Coord>& positions) const
{
	Coord playerPos = find_player(id);

	queue<Coord, deque<Coord>> queue;
	queue.push(playerPos);
	set<Coord> used;

	while (!queue.empty()) 
	{
		Coord curPos = queue.front();
		queue.pop();

		if (used.find(curPos) == used.end()) 
		{
			//mark position as used to not expand it again
			used.insert(curPos);
			positions.push_back(curPos);

			//check/enqueue neighbords
			if (card_at(curPos)._op[T] && curPos.row > 0)
			{
				Coord neighbor(curPos.row - 1, curPos.col);
				if (card_at(neighbor)._op[B])
				{
					queue.push(neighbor);
				}
			}

			if (card_at(curPos)._op[B] && curPos.row < SIZE - 1)
			{
				Coord neighbor(curPos.row + 1, curPos.col);
				if (card_at(neighbor)._op[T])
				{
					queue.push(neighbor);
				}
			}

			if (card_at(curPos)._op[R] && curPos.col < SIZE - 1)
			{
				Coord neighbor(curPos.row, curPos.col + 1);
				if (card_at(neighbor)._op[L])
				{
					queue.push(neighbor);
				}
			}

			if (card_at(curPos)._op[L] && curPos.col > 0)
			{
				Coord neighbor(curPos.row, curPos.col - 1);
				if (card_at(neighbor)._op[R])
				{
					queue.push(neighbor);
				}
			}
		}
	}
}

Coord Board::find_player(int id) const
{
	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < SIZE; ++j)
		{
			if (_card[i][j]._op[id - 1])
			{
				return Coord(i, j);
			}
		}
	}

	return Coord(-1,-1);
}

Coord Board::find_treasure(Treasure t) const
{
	for (int i = 0; i < SIZE; ++i)
	{
		for (int j = 0; j < SIZE; ++j)
		{
			if (_card[i][j]._t == t)
			{
				return Coord(i, j);
			}
		}
	}

	return Coord(-1,-1);
}

Card& Board::card_at(const Coord& c)
{
	return _card[c.row][c.col];
}

const Card& Board::card_at(const Coord& c) const
{
	return _card[c.row][c.col];
}

int Board::can_reach(int id, Treasure t) const
{
	vector<Coord> positions;
	expand_positions(id, positions);
		
	Coord treasurePos = find_treasure(t);
	int minDistance = abs(treasurePos.row - positions[0].row) + abs(treasurePos.col - positions[0].col);

	for (size_t i = 1; i < positions.size(); i++)
	{
		if (card_at(positions[i])._t == t)
		{
			return 0;
		}
		else
		{
			int distance = abs(treasurePos.row - positions[i].row) + abs(treasurePos.col - positions[i].col);
			if (distance < minDistance)
			{
				minDistance = distance;
			}
		}
	}

	return minDistance;
}

ostream& operator<<(ostream& os, const Board& b)
{
	for (int i = 0; i < 7; i++)
	{				
		//oberes drittel
		os << "|";
		for (int j = 0; j < 7; ++j)
		{	
			if (b._card[i][j]._op[T])
			{
				os << ">>>   <<<|";
			}
			else 
			{
				os << ">>>>-<<<<|";
			}
		}

		if (i == 0)
		{
			if (b._shift._op[T])
			{
				os << "###   ##";
			}
			else 
			{
				os << "########";
			}
		}
		os << "\n";
		os << "|";
		for (int j = 0; j < 7; ++j)
		{	
			if (b._card[i][j]._op[T])
			{
				os << "###   ###|";
			}
			else 
			{
				os << "#########|";
			}
		}

		if (i == 0)
		{
			if (b._shift._op[T])
			{
				os << "###   ##";
			}
			else 
			{
				os << "########";
			}
		}
		os << "\n";

		//mittleres drittel
		//erste zeile
		os << "|";
		for (int j = 0; j < 7; ++j)
		{				
			if (b._card[i][j]._op[L])
			{
				os << "   ";
			} 
			else
			{
				os << "###";
			}

			if (b._card[i][j]._op[P1])
			{
				os << "1";
			} 
			else
			{
				os << " ";
			}

			if (b._card[i][j]._t != t0)
				os << (char)('a' + b._card[i][j]._t);
			else
				os << " ";

			if (b._card[i][j]._op[P2])
			{
				os << "2";
			} 
			else
			{
				os << " ";
			}

			if (b._card[i][j]._op[R])
			{
				os << "   |";
			}
			else 
			{
				os << "###|";
			}
		}
		if (i == 0)
		{
			if (b._shift._op[L])
			{
				os << "   ";
			} 
			else
			{
				os << "###";
			}

			if (b._shift._t != t0)
				os << " " << (char)('a' + b._shift._t) << " ";
			else
				os << "   ";

			if (b._shift._op[R])
			{
				os << "  ";
			}
			else 
			{
				os << "##";
			}
		}
		os << "\n|";

		//zweite zeile
		for (int j = 0; j < 7; ++j)
		{	
			if (b._card[i][j]._op[L])
			{
				os << "   ";
			} 
			else
			{
				os << "###";
			}

			if (b._card[i][j]._op[P3])
			{
				os << "3";
			} 
			else
			{
				os << " ";
			}
			os << " ";
			if (b._card[i][j]._op[P4])
			{
				os << "4";
			}
			else 
			{
				os << " ";
			}


			if (b._card[i][j]._op[R])
			{
				os << "   |";
			}
			else 
			{
				os << "###|";
			}
		}
		if (i == 0)
		{
			if (b._shift._op[L])
			{
				os << "   ";
			} 
			else
			{
				os << "###";
			}
			os << "   ";
			if (b._shift._op[R])
			{
				os << "   ";
			}
			else 
			{
				os << "##";
			}
		}
		os << "\n";

		//unteres drittel
		for (int repeat = 0; repeat < 2; ++repeat)
		{
			os << "|";
			for (int j = 0; j < 7; ++j)
			{	
				if (b._card[i][j]._op[B])
				{
					os << "###   ###|";
				}
				else 
				{
					os << "#########|";
				}
			}
			if (i == 0)
			{
				if (b._shift._op[B])
				{
					os << "###   ##";
				}
				else 
				{
					os << "########";
				}
			}
			os << "\n";
		}
		
		//os << "-----------------------------------------------------------------------\n";
	}

	return os;
}