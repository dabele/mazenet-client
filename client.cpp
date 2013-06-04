#include "client.hpp"
#include "mazeCom.hpp"

#include <sstream>

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

string client::recv()
{
	char* l = new char[4];
	asio::read(socket_, asio::buffer(l, 4));
	int length = 0;
	length = length | l[3];
	length = length << 8;
	length = length | l[2];
	length = length << 8;
	length = length | l[1];
	length = length << 8;
	length = length | l[0];

	std::vector<char> b;
	asio::read(socket_, asio::buffer(b, length));

	string s;
	for (int i = 0; i < b.size(); ++i)
	{
		s += b[i];
	}
	return s;

}