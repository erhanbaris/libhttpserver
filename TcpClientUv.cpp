#include <stdlib.h>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>

#include <uv.h>
#include <http_parser.h>

#include <Tools.h>
#include <TcpClientUv.h>
#include <Config.h>

using namespace std;

namespace {
	static char* _strndup(char *str, int chars)
	{
		char *buffer;
		int n;

		buffer = (char *)malloc(chars + 1);
		if (buffer)
		{
			for (n = 0; ((n < chars) && (str[n] != 0)); ++n) buffer[n] = str[n];
			buffer[n] = 0;
		}

		return buffer;
	}
}

struct TcpClientUvPimpl
{
	std::string address;
	size_t port;
	bool isConnected;
	std::string sendMessage;
	bool largeBufferStarted;
	std::stringstream tmpBuffer;

    TcpClientUv* tcpClient;

	// uv
	uv_async_t* asyncOperation;
	uv_async_t* closeAsync;
	uv_tcp_t* client;
	uv_connect_t* connect;

	// Callbacks
    TcpClientUv::MessageCallback messageCallback;
    TcpClientUv::DisconnectCallback disconnectCallback;
    TcpClientUv::ConnectCallback connectCallback;

	TcpClientUvPimpl(TcpClientUv* pTcpClient) : tcpClient(pTcpClient)
	{
		asyncOperation = NULL;
		client = NULL;
		connect = NULL;
		isConnected = false;

		messageCallback = [](std::string const &, TcpClientUv *) { std::cout << "TcpClientUv 'messageCallback' Not Setted" << std::endl; };
		disconnectCallback = [](TcpClientUv *) { std::cout << "TcpClientUv 'disconnectCallback' Not Setted" << std::endl; };
		connectCallback = [](TcpClientUv *) { std::cout << "TcpClientUv 'connectCallback' Not Setted" << std::endl; };
	}

	~TcpClientUvPimpl()
	{
		if (asyncOperation != NULL)
			delete asyncOperation;

		if (client != NULL)
			delete client;

		if (connect != NULL)
			delete connect;
	}

    /* libuv callbacks */
	static void closeCb(uv_handle_t* handle)
	{
		TcpClientUvPimpl * pimpl = (TcpClientUvPimpl*)handle->data;
		pimpl->disconnectCallback(pimpl->tcpClient);
	}

	static void allocCb(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
		*buf = uv_buf_init((char*)malloc(size), size);
	}

	static void readCb(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
	{
		TcpClientUvPimpl * pimpl = (TcpClientUvPimpl*)tcp->data;

		if (nread >= 0) {
			char* tmpData = _strndup(buf->base, nread);
			pimpl->tmpBuffer << tmpData;

			if ((size_t)nread == buf->len && 65536 == nread)
				pimpl->largeBufferStarted = true;

			else if (pimpl->largeBufferStarted && (size_t)nread != buf->len)
				pimpl->largeBufferStarted = false;

			if (!pimpl->largeBufferStarted)
			{
				auto message = pimpl->tmpBuffer.str();
				pimpl->messageCallback(message, pimpl->tcpClient);
				pimpl->tmpBuffer.str(std::string());
			}

			delete tmpData;
		}
		else {
			uv_close((uv_handle_t*)tcp, closeCb);
		}

		free(buf->base);
	}

	static void afterSend(uv_write_t* handle, int status) {
		TcpClientUvPimpl * pimpl = (TcpClientUvPimpl*)handle->data;
        pimpl->sendMessage.clear();
	}

	static void afterSendAndClose(uv_write_t* handle, int status) {
		TcpClientUvPimpl * pimpl = (TcpClientUvPimpl*)handle->data;
        pimpl->sendMessage.clear();
		uv_close((uv_handle_t*)pimpl->client, closeCb);
	}

	static void async(uv_async_t *handle) {
		TcpClientUvPimpl * pimpl = (TcpClientUvPimpl*)handle->data;

        uv_buf_t resbuf;
        resbuf.base = const_cast<char *>(pimpl->sendMessage.c_str());
        resbuf.len = pimpl->sendMessage.size();

        uv_write_t *write_req = new uv_write_t;
        write_req->data = pimpl;

        uv_write(write_req, (uv_stream_t *)pimpl->client, &resbuf, 1, afterSendAndClose);
	}

	static void onConnect(uv_connect_t* connection, int status)
	{
		if (status < 0) {
			return;
		}

		TcpClientUvPimpl * pimpl = (TcpClientUvPimpl*)connection->data;

		pimpl->asyncOperation = new uv_async_t;
		pimpl->asyncOperation->data = pimpl;
		pimpl->isConnected = true;

		uv_stream_t* stream = connection->handle;
		stream->data = pimpl;
		uv_async_init(loop, ((uv_async_t*)pimpl->asyncOperation), TcpClientUvPimpl::async);
		uv_read_start(stream, allocCb, readCb);

		pimpl->connectCallback(pimpl->tcpClient);
	}
    /* libuv callbacks */
};

void TcpClientUv::Disconnect()
{
	if (IsConnected())
	{
		uv_async_send(pimpl->asyncOperation);
	}
}

TcpClientUv::TcpClientUv()
{
	pimpl = new TcpClientUvPimpl(this);
}

TcpClientUv::TcpClientUv(void* serverHandle)
{
	pimpl = new TcpClientUvPimpl(this);
	pimpl->asyncOperation = new uv_async_t;
	pimpl->asyncOperation->data = pimpl;
	pimpl->isConnected = true;

	pimpl->client = new uv_tcp_t;
	pimpl->client->data = pimpl;

	uv_tcp_init(loop, pimpl->client);
    uv_accept((uv_stream_t*)serverHandle, (uv_stream_t*)pimpl->client);

	struct sockaddr_in name;
	int namelen = sizeof(name);
	if (uv_tcp_getpeername(pimpl->client, (struct sockaddr*) &name, &namelen))
		INFO << "uv_tcp_getpeername";

	char addr[16];
	uv_inet_ntop(AF_INET, &name.sin_addr, addr, sizeof(addr));

	pimpl->address = std::string(addr);
	pimpl->port = name.sin_port;


	uv_async_init(loop, ((uv_async_t*)pimpl->asyncOperation), TcpClientUvPimpl::async);
	uv_read_start((uv_stream_t*)pimpl->client, TcpClientUvPimpl::allocCb, TcpClientUvPimpl::readCb);
}

TcpClientUv::~TcpClientUv()
{
	delete pimpl;
}

std::string TcpClientUv::GetRemoteAddress()
{
	return pimpl->address;
}

size_t TcpClientUv::GetRemotePort()
{
	return pimpl->port;
}

bool TcpClientUv::IsConnected()
{
	return pimpl->isConnected;
}

void TcpClientUv::Send(std::string const & message)
{
	pimpl->sendMessage = message;
	uv_async_send(pimpl->asyncOperation);
}

void TcpClientUv::SetOnConnect(ConnectCallback cb)
{
	pimpl->connectCallback = cb;
}

void TcpClientUv::SetOnMessage(MessageCallback cb)
{
	pimpl->messageCallback = cb;
}

void TcpClientUv::SetOnDisconnect(DisconnectCallback cb)
{
	pimpl->disconnectCallback = cb;
}
