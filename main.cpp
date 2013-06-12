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
		msg->AwaitMoveMessage()->board().forbidden() = positionType(0,1);
		print(ofs, msg->AwaitMoveMessage()->board());
		
		vector<client::pBoardType> v;
		c.expand_board(msg->AwaitMoveMessage()->board(), v);

		for (int i = 0; i < v.size(); ++i)
		{
			print(ofs, *v[i]);
			ofs << "\n///////////////////////////////////////////////////\n";
		}

		vector<client::pBoardType> s;
		c.expand_pin_positions(msg->AwaitMoveMessage()->board(), msg->AwaitMoveMessage()->treasure(), s);
		print(ofs2, msg->AwaitMoveMessage()->board());
		for (int i = 0; i < s.size(); ++i)
		{
			positionType pos;
			c.find_player(*s[i], pos);
			ofs2 << "(" << pos.col() << "," << pos.row() << ")" << endl;
		}
	}
	catch (xml_schema::expected_element& e)
	{
		cout << e.what() << ": " << e.name() << endl;
	}
}
