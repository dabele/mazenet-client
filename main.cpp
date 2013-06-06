#include <string>
#include <iostream>

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

	/*client c(host, port);
	c.login(string("c"));

	client d(host, port);
	d.login(string("d"));*/

	MazeCom m(MazeComType::AWAITMOVE, 1);
	positionType forbidden(1, 2);
	cardType(

	c.play();

}