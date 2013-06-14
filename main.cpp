#include <string>
#include <iostream>
#include <fstream>

#include "boost/asio.hpp"

#include "mazeCom_.hpp"
#include "client.hpp"

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

	client c(host, port);
	//c.login(string("c"));

	/*client d(host, port);
	d.login(string("d"));

	try
	{
		c.play();
	}
	catch (xml_schema::expected_element& e)
	{
		cout << e.what() << ": " << e.name() << endl;
	}*/

	try
	{
		std::ifstream ifs("board.txt");
		ofstream ofs("boards.txt");
		ofstream ofs2("pins.txt");

		MazeCom_Ptr msg(MazeCom_(ifs, xml_schema::flags::dont_validate));
		
		//move by first opponent (or self if only one player)
		vector<client::pBoardType> expand;
		c.expand_board(msg->AwaitMoveMessage()->board(), expand);

		//move by following opponents
		for (int j = 1; j < 4; ++j)
		{
			vector<client::pBoardType> expand_next;
			srand(time(NULL));
			for (int k = 0; k < expand.size(); k += 4)
			{
				int offset = rand()%4;
				c.expand_board(*expand[k + offset], expand_next);
			}
			expand.swap(expand_next);
			expand_next.clear();
		}

		int numDouble = 0;
		for (vector<client::pBoardType>::iterator it = expand.begin(); it != expand.end(); ++it)
		{
			cout << it - expand.begin() << " " << numDouble << endl;
			if (std::find(vector<client::pBoardType>::iterator(it) + 1, expand.end(), *it) != expand.end())
			{
				
				numDouble++;
			}
		}


		//MoveMessageType moveMsg(positionType(0,0), positionType(0,0), cardType());
		//c.find_next_move(*msg->AwaitMoveMessage(), moveMsg);
	}
	catch (xml_schema::expected_element& e)
	{
		cout << e.what() << ": " << e.name() << endl;
	}
}
