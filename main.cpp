/*
* main program for mazenet client
*
* dependencies:
* code synthesis xsd for data binding
* boost asio/system for socket communication
* some other boost header-only libraries
* requires C++11, tested with MSVC 10.0 and 11.0
* OpenMP recommended for speed, OpenMP header required for timeout
*
* mazenet server not included
*/

#include <string>
#include <iostream>
#include <fstream>
#include <omp.h>

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
	catch (boost::system::system_error& e) //thrown if connection problems
	{
		cout << e.what() << endl;
	}
}
