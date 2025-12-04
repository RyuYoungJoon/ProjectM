#include "pch.h"
#include "Client.h"

Client::Client() : _state(STATUS::ST_FREE), _prev_size(0)
{
	x = 0;
	y = 0;
}

Client::~Client()
{
	closesocket(_socket);
}
