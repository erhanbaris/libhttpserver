#include <vector>
#include <thread>
#include <memory>

#include <Config.h>

#include <TcpServerUv.h>
#include <TcpClientUv.h>

#include <json11.hpp>


struct TcpServerUvPimpl
{
	size_t port;
    std::vector<TcpClientUv*> clients;

	// uv
	uv_tcp_t* tcpServer;

	// callbacks
    TcpServerUv::MessageReceivedCallback messageReceivedCallback;
    TcpServerUv::ClientConnectedCallback clientConnectedCallback;
    TcpServerUv::ClientDisconnectedCallback clientDisconnectedCallback;

	TcpServerUvPimpl()
	{
		tcpServer = new uv_tcp_t;
		tcpServer->data = this;


		/* Set empty function for ignoring null check */ 
		messageReceivedCallback = [](std::string const &, TcpClientUv *) { std::cout << "TcpServerUv 'messageReceivedCallback' Not Setted" << std::endl; };
		clientConnectedCallback = [](TcpClientUv *) { std::cout << "TcpServerUv 'clientConnectedCallback' Not Setted" << std::endl; };
		clientDisconnectedCallback = [](TcpClientUv *) { std::cout << "TcpServerUv 'clientDisconnectedCallback' Not Setted" << std::endl; };
	}

	~TcpServerUvPimpl()
	{

	}

	void Start() {
		uv_tcp_init(loop, tcpServer);
		struct sockaddr_in address;
		uv_ip4_addr("0.0.0.0", port, &address);
		uv_tcp_bind(tcpServer, (const struct sockaddr*)&address, 0);
		uv_listen((uv_stream_t*)tcpServer, 1000, onConnect);
	}

    void onDisconnect(TcpClientUv* client)
	{
		clientDisconnectedCallback(client);
	}

	static void onConnect(uv_stream_t* serverHandle, int status) {
		TcpServerUvPimpl* pimpl = (TcpServerUvPimpl*)serverHandle->data;

        TcpClientUv* tcpClient = new TcpClientUv(serverHandle);
        tcpClient->SetOnMessage([pimpl](std::string const& message, TcpClientUv* client)
        {
			pimpl->messageReceivedCallback(message, client);
        });

		tcpClient->SetOnDisconnect([](TcpClientUv* client)
		{
			
		});

		pimpl->clientConnectedCallback(tcpClient);
		pimpl->clients.push_back(tcpClient);
	}
};

TcpServerUv::TcpServerUv() {
	pimpl = new TcpServerUvPimpl;
}

void TcpServerUv::Start(size_t port) {
	pimpl->port = port;
	pimpl->Start();
}

size_t TcpServerUv::GetPort()
{
	return pimpl->port;
}

void TcpServerUv::SetMessageReceived(MessageReceivedCallback cb)
{
	pimpl->messageReceivedCallback = cb;
}

void TcpServerUv::SetClientConnected(ClientConnectedCallback cb)
{
	pimpl->clientConnectedCallback = cb;
}
void TcpServerUv::SetClientDisconnected(ClientDisconnectedCallback cb)
{
	pimpl->clientDisconnectedCallback = cb;
}

void TcpServerUv::Stop()
{

}

TcpClientUv *TcpServerUv::CreateClient() {
	return new TcpClientUv();
}
