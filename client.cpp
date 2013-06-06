#include "client.hpp"

#include <sstream>
#include <iostream>

namespace asio = boost::asio;
using asio::ip::tcp;
using std::stringstream;


client::client(const string& host, const string& port)
	: 	io_service_(), socket_(io_service_)
{
	tcp::resolver resolver(io_service_);
	tcp::resolver::query query(host, port);
	tcp::resolver::iterator it = resolver.resolve(query);
	boost::asio::connect(socket_, it);
	io_service_.run();
}

void client::send(const MazeCom& msg)
{
	stringstream serializationStream;
	MazeCom_(serializationStream, msg);
	string serializedMsg(serializationStream.str());
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
	std::cout << *moveRequest << std::endl;
	return;


	//send move
	MazeCom moveMsg(MazeComType::MOVE, id_);
}