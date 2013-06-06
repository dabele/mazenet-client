#include <string>
#include <iostream>
#include <sstream>

#include "mazeCom.hpp"
#include "boost/asio.hpp"
#include "client.hpp"

using std::string;
using std::stringstream;
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
		port = "5000";
	}

	client c(host, port);

	MazeCom m(MazeComType::LOGIN, 1);
	m.LoginMessage(LoginMessageType("test"));
	//MazeCom_(cout, m);

	c.send(m);

	client::mc_ptr r(c.recv());
	cout << *r << endl;
	MazeCom_(cout, *r);
}