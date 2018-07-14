#pragma once

#include <string>
#include <deque>

#include <functional>

struct TcpClientUvPimpl;
class TcpClientUv {
public:
    typedef std::function<void(std::string const &, TcpClientUv *)> MessageCallback;
    typedef std::function<void(TcpClientUv *)> DisconnectCallback;
    typedef std::function<void(TcpClientUv *)> ConnectCallback;

    TcpClientUv();
    TcpClientUv(void*);
    virtual ~TcpClientUv();

    void Disconnect();
    bool IsConnected();
    void Send(std::string const &);
    std::string GetRemoteAddress();
    size_t GetRemotePort();
    
    void SetOnMessage(MessageCallback);
    void SetOnDisconnect(DisconnectCallback);
    void SetOnConnect(ConnectCallback);

    void* Data1; //Reserved for future operations
    void* Data2; //Reserved for future operations

private:
    TcpClientUvPimpl *pimpl;
};
