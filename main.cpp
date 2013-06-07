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
	c.login(string("c"));

	client d(host, port);
	d.login(string("d"));

	try
	{
		c.play();
	}
	catch (xml_schema::expected_element& e)
	{
		cout << e.what() << ": " << e.name() << endl;
	}

	/*try
	{
		std::ifstream ifs("D:\\exercises\\Rechnernetze\\MazeNet\\awaitmove.xml");
		cout << *MazeCom_(ifs, xml_schema::flags::dont_validate) << endl;
	}
	catch (xml_schema::expected_element& e)
	{
		cout << e.what() << ": " << e.name() << endl;
	}*/

}