#pragma once

#include <unordered_map>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <deque>

#include <uv.h>
#include <Config.h>
#include <Response.h>

class HttpServer;
class HttpClientPimpl;

class HttpClient
{
    public:
        void * Parser; // http_parser

        typedef std::function<void(std::string const &, HttpClient *)> MessageCallback;
        typedef std::function<void(HttpClient *)> ConnectCallback;

        void SetOnMessage(MessageCallback);

        std::string GetRemoteAddress();
        size_t GetRemotePort();

        std::unordered_map<std::string, std::string> Headers;
        std::unordered_map<std::string, std::string>::const_iterator HeadersEnd;

        std::string LastHeaderItem;

        std::string Schema;
        std::string Host;
        std::string Port;
        std::string Path;
        std::string Query;
        std::string Fragment;
        std::string UserInfo;
        std::string Max;

        std::string RequestBuffer;
        std::stringstream ResponseBuffer;

        HttpResponseType Type;
        std::string ContentType;

        std::unordered_map<std::string, AnyType> ViewMap;
        std::unordered_map<std::string, AnyType> Session;

        HttpClient(void*);

        virtual ~HttpClient();
        virtual void Send();

    private:
        HttpClientPimpl* pimpl;
};
