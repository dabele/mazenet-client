/*
* more efficient way to store boards than generated types by codesynthesis xsd
*/

#ifndef BOARD_HPP
#define BOARD_HPP

#include <bitset>
#include <vector>
#include <iostream>

#include "mazeCom_.hpp"

using namespace std;

//used for bitset openings and player pins in cards
#define P1 0
#define P2 1
#define P3 2
#define P4 3
#define T 4
#define B 5
#define L 6
#define R 7

//size of the board, probably not used everywhere it should be
#define SIZE 7

enum Treasure
{
	//position is important so cast between this enum and treasureType works
	s1,s2,s3,s4, //start positions
	t1,t2,t3,t4,t5,t6,t7,t8,t9, t10, t11, t12, t13, t14, t15, t16, t17, t18, t19, t20, t21, t22, t23, t24, //treasures
	t0 //no treasure
};

struct Coord
{
	int row, col;

	Coord() : col(0), row(0) { } 
	Coord(int r, int c) : col(c), row(r) { }
	Coord(const positionType& p) : row(p.row()), col(p.col()) { }

	Coord opposite() const;	
	bool operator==(const Coord& arg) const;
	bool operator<(const Coord& arg) const;
};

struct Card
{
	Treasure _t;
	//bits for player pins and openings
	std::bitset<8> _op; 

	Card() : _t(t0), _op(0) { }
	Card (const cardType& c);
	
	void rotate();

	//one (or two in case of corner) rotation for this card that based on shift position makes the most connections
	vector<Card> default_rotations(const Coord& shiftPos) const;

	//conversion functions for different treasure enums
	operator cardType();
	static Treasure conv(const cardType::treasure_optional& t);
	static Treasure conv(const treasureType::value);
	static void conv(Treasure t, cardType& );
};

struct Board
{
	typedef shared_ptr<Board> ptr;

	Coord _forbidden;
	Card _shift;
	vector<vector<Card>> _card;

	//default ctor, field initialized random
	Board(); 
	//ctor, intializes like boardType
	Board(const boardType& b);
	//copy ctor
	Board(const Board& b);
	//dtor
	~Board();
	//assignment
	Board& operator=(const Board& b);
	//do shift at shiftPos, not using the shiftCard stored in this board but the shiftCard argument
	void shift(const Coord& shiftPos, const Card& shiftCard);
	//adds all board for each possible shift to the vector (each position but the forbidden, every possible rotation of shift card)
	void expand_shifts(vector<ptr>& children) const;
	//adds all board for each possible shift to the vector (each position but the forbidden, default rotation for shift card)
	void expand_default_shifts(vector<ptr>& children) const;
	//adds board for shifts to the vector that are beneficial to the player with id 
	//(tries each position but the forbidden, all rotations of the shift card, 
	//then removes those that don't change anything about the possible positions or reachable treasures)
	void expand_beneficial_shifts(int id, vector<ptr>& children) const;	
	//adds board for shifts to the vector that are beneficial to the player with id 
	//(tries each position but the forbidden, default rotation of shift card, 
	//then removes those that don't change anything about the possible positions or reachable treasures)
	void expand_beneficial_default_shifts(int id, vector<ptr>& children) const;	
	//all possible positions on this board for player with id, using breadth first search
	void expand_positions(int id, vector<Coord>& positions) const;
	//returns coordinates of player with id, returns -1/-1 if player not found
	Coord find_player(int id) const;
	//returns coordinates of treasure, returns -1/-1 if player not found	
	Coord find_treasure(Treasure t) const;
	//card at position, no range check
	Card& card_at(const Coord& c);
	const Card& card_at(const Coord& c) const;
	//returns distance of position of player with id from position of treasure t (0 means treasure is reachable)
	int can_reach(int id, Treasure t) const;
};

ostream& operator<<(ostream& os, const Board& b);

#endif