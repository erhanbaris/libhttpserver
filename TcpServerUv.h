#pragma once

#include <TcpClientUv.h>
#include <uv.h>

#include <iostream>
#include <set>
#include <functional>

using namespace std;

struct TcpServerUvPimpl;
class TcpServerUv {
public:
    typedef std::function<void(std::string const &, TcpClientUv *)> MessageReceivedCallback;
    typedef std::function<void(TcpClientUv *)> ClientConnectedCallback;
    typedef std::function<void(TcpClientUv *)> ClientDisconnectedCallback;

	TcpServerUv();
	void Start(size_t port);
	void Stop();
	size_t GetPort();
    TcpClientUv* CreateClient();

	void SetMessageReceived(MessageReceivedCallback);
	void SetClientConnected(ClientConnectedCallback);
	void SetClientDisconnected(ClientDisconnectedCallback);

private:
    TcpServerUvPimpl *pimpl;
};
