/*
* main program for mazenet client
*
* dependant on: 
* code synthesis xsd for data binding
* boost asio/system for socket communication
* some other boost header only libraries
* C++11
* OpenMP recommended for speed, OpenMP header required for timeout
*
* mazenet server not included
*/

#include <string>
#include <iostream>
#include <fstream>
#include <omp.h>
#include <bitset>

#include "boost/asio.hpp"

#include "mazeCom_.hpp"
#include "client.hpp"
#include "board.hpp"

using std::string;
using std::cout;
using std::endl;

int main(int argc, char** argv) 
{
	string host, port;

	if (argc == 3) 
	{
		host = argv[1];
		port = argv[2];
	} 
	else
	{
		//debug
		host = "localhost";
		port = "5123";
	}	

	try
	{
		client c(host, port);
		srand(time(NULL));

		string name;
		cout << "enter name:" << endl;
		cin >> name;
		c.login(name);
		c.play();
		
		//test the algorithm with a presaved board
		/*std::ifstream ifs("board.txt");
		MazeCom_Ptr msg(MazeCom_(ifs, xml_schema::flags::dont_validate));
		Board b(msg->AwaitMoveMessage()->board());

		cout << b << endl << endl;

		double begin = omp_get_wtime();

		MoveMessageType moveMsg(positionType(0,0), positionType(0,0), cardType());
		c.find_next_move(*msg->AwaitMoveMessage(), moveMsg);

		cout << omp_get_wtime() - begin << endl;		

		b.shift(Coord(moveMsg.shiftPosition()), Card(moveMsg.shiftCard()));

		cout << b << endl;
		cout << "(" << moveMsg.newPinPos().row() << "," << moveMsg.newPinPos().col() << ")" << endl;*/
		
	}
	catch (MazeCom_Ptr& unexpected)
	{
		switch ((MazeComType::value)unexpected->mcType())
		{
		case MazeComType::WIN:
			cout << "winner:" << unexpected->WinMessage()->winner() << endl;
			break;
		}
	} 
	catch (boost::system::system_error e)
	{
		cout << e.what() << endl;
	}
	catch (runtime_error e)
	{
		cout << e.what() << endl;
	}
}
