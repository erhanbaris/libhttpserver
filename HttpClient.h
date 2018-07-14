#pragma once

#include <unordered_map>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <deque>

#include <uv.h>
#include <TcpClientUv.h>
#include <Config.h>

class HttpServer;
class TcpClientUv;

class HttpClient : public TcpClientUv
{
public:
    void * Handle; // uv_tcp_t
    void * Parser; // http_parser
    void * Async; // uv_async_t

    TcpClientUv* TcpClient;
    void * Data2;

    std::unordered_map<std::string, std::string> Headers;
    std::unordered_map<std::string, std::string>::const_iterator HeadersEnd;

    std::string LastHeaderItem;
    std::string Url;
    std::string RequestBuffer;
    std::stringstream ResponseBuffer;
    HttpServer* ServerPimpl;
    HttpRequestType Type;
    std::string ContentType;

    std::unordered_map<std::string, AnyType> ViewMap;
    std::unordered_map<std::string, AnyType> Session;

    HttpClient( /*std::unordered_map<std::string, AnyType>& pSession*/ );
    virtual ~HttpClient();
    virtual void Send();
};
