#include <vector>
#include <thread>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <sstream>
#include <memory>
#include <utility>
#include <unordered_map>

#include <http_parser.h>
#include <json11.hpp>
#include <uv.h>
#include <json11.hpp>

#include <HttpClient.h>
#include <HttpServer.h>
#include <Config.h>
#include <Controller.h>
#include <InlineController.h>

uv_loop_t* loop = NULL;
namespace
{
    static http_parser_settings* parser_settings;
}

bool staticFolderMiddleware(HttpClient* client)
{
    std::cout << "staticFolderMiddleware " << client->Path << std::endl;
    return true;
}

bool sessionMiddleware(HttpClient* client)
{
    //client->Headers
    return true;
}

struct HttpServerPimpl
{
    std::unique_ptr<uv_tcp_t> tcpServer;
    uv_timer_t elapsedTimer;
    HttpServer& httpServer;
    size_t port;
    std::vector<HttpClient*> clients;
    std::unordered_map<std::string, std::shared_ptr<Controller>> controllers;
    std::unordered_map<std::string, std::shared_ptr<Controller>>::const_iterator controllersEnd;

    std::vector<HttpServer::MiddlewareItem> preMiddlewareItems;
    std::vector<HttpServer::MiddlewareItem>::const_iterator preMiddlewareItemsEnd;

    std::vector<HttpServer::MiddlewareItem> postMiddlewareItems;
    std::vector<HttpServer::MiddlewareItem>::const_iterator postMiddlewareItemsEnd;

    std::unordered_map<std::string, HttpServer::RouteItem> routes;
    std::unordered_map<std::string, HttpServer::RouteItem>::const_iterator routesEnd;

    // callbacks
    HttpServer::MessageReceivedCallback messageReceivedCallback;
    HttpServer::ClientConnectedCallback clientConnectedCallback;
    HttpServer::ClientDisconnectedCallback clientDisconnectedCallback;

    std::vector<std::string> staticFolders;
    std::string workingDir;
    std::string applicatonDir;

    void Start() {
        uv_tcp_init(loop, tcpServer.get());
        struct sockaddr_in address;
        uv_ip4_addr("0.0.0.0", port, &address);
        uv_tcp_bind(tcpServer.get(), (const struct sockaddr*)&address, 0);
        uv_listen((uv_stream_t*)tcpServer.get(), 1000, onConnect);
        uv_run(loop, UV_RUN_DEFAULT);
    }

    static void onConnect(uv_stream_t* serverHandle, int status) {
        HttpServerPimpl* pimpl = (HttpServerPimpl*)serverHandle->data;

        HttpClient* tcpClient = new HttpClient(serverHandle);
        tcpClient->SetOnMessage([pimpl](std::string const& message, HttpClient* client)
        {
            pimpl->messageReceived(message, client);
        });

        pimpl->clientConnected(tcpClient);
        pimpl->clients.push_back(tcpClient);
    }

    HttpServerPimpl(HttpServer* pHttpServer): httpServer(*pHttpServer)
    {
        parser_settings = new http_parser_settings;
        tcpServer = std::unique_ptr<uv_tcp_t>(new uv_tcp_t());
        tcpServer->data = this;

        messageReceivedCallback = [](HttpClient *) { std::cout << "HttpServer 'messageReceivedCallback' Not Setted" << std::endl; };
        clientConnectedCallback = [](HttpClient*) { std::cout << "HttpServer 'clientConnectedCallback' Not Setted" << std::endl; };
        clientDisconnectedCallback = [](HttpClient *) { std::cout << "HttpServer 'clientDisconnectedCallback' Not Setted" << std::endl; };
    }

    ~HttpServerPimpl()
    {
        if (parser_settings != NULL)
            delete parser_settings;
    }

    void messageReceived(std::string const& message, HttpClient* client)
    {
        auto parsedSize = http_parser_execute((http_parser*)client->Parser, parser_settings, message.c_str(), message.size());

        if (parsedSize == message.size())
        {
            struct http_parser_url u;
            char const * url = client->Headers["Host"].c_str();
            int result = http_parser_parse_url(url, strlen(url), 1, &u);

            if (!result)
            {
                if ((u.field_set & (1 << UF_HOST))) {
                    const char * data = url + u.field_data[UF_HOST].off;
                    client->Host = std::string(data, u.field_data[UF_HOST].len);
                }

                if ((u.field_set & (1 << UF_PORT))) {
                    const char * data = url + u.field_data[UF_PORT].off;
                    client->Port = std::string(data, u.field_data[UF_PORT].len);
                }
            }

            http_parser_execute((http_parser*)client->Parser, parser_settings, client->Headers["Host"].c_str(), client->Headers["Host"].size());
            parseRequest(client);
        }
    }

    void parseRequest(HttpClient* client)
    {
        std::unordered_map<std::string, HttpServer::RouteItem>::const_iterator routeCallback = routes.find(client->Path);
        std::unordered_map<std::string, std::shared_ptr<Controller>>::const_iterator callback = controllers.find(client->Path);
        if (routeCallback != routesEnd)
        {
            Response* response = routeCallback->second(client);

            if (response != nullptr)
            {
                bool continueToRequest = true;
                for(auto it = preMiddlewareItems.begin(); it != preMiddlewareItemsEnd && continueToRequest; ++it)
                    continueToRequest = (*it)(client);

                if (continueToRequest)
                {
                    client->ResponseBuffer << response->Render();
                    client->ContentType = response->GetContentType();
                    client->Type = response->GetResponseType();

                    for(auto it = preMiddlewareItems.begin(); it != preMiddlewareItemsEnd && continueToRequest; ++it)
                        continueToRequest = (*it)(client);
                }

                delete response; response = nullptr;
            }
        }
        else if (callback != controllersEnd)
        {
            Controller* controller = callback->second.get();
            Controller* newController = controller->Create();

            if (controller->HasAction("hello"))
            {
                controller->BeginRequest(client);
                controller->GetAction("hello")->Function(client);
                controller->EndRequest(client);
            }
            else
                client->ResponseBuffer << "{\"Status\":true,\"Message\":\"" << newController->GetControllerName() << "\"}";

            delete newController; newController = nullptr;
        }
        else
            client->ResponseBuffer << "{\"Status\":false,\"Message\":\"Page not found\"}";

        client->Send();
    }

    void clientConnected(HttpClient* client)
    {
        http_parser* httpParser = new http_parser;
        httpParser->data = client;
        client->Parser = httpParser;
        http_parser_init(httpParser, HTTP_REQUEST);
    }

    void clientDisconnected(HttpClient* client)
    {
        delete (http_parser*)client->Parser;
        delete client;

        clientDisconnectedCallback(client);
    }


    static int on_message_begin(http_parser* /*parser*/) {
        return 0;
    }

    static int on_url(http_parser* parser, const char* url, size_t length) {

        HttpClient* client = (HttpClient*)parser->data;
        struct http_parser_url u;
        int result = http_parser_parse_url(url, length, 0, &u);
        if (result) {
            fprintf(stderr, "\n\n*** failed to parse URL %s ***\n\n", url);
            return -1;
        }
        else {
            if ((u.field_set & (1 << UF_PATH))) {
                const char * data = url + u.field_data[UF_PATH].off;
                client->Path = std::string(data, u.field_data[UF_PATH].len);
            }

            if ((u.field_set & (1 << UF_QUERY))) {
                const char * data = url + u.field_data[UF_QUERY].off;
                client->Query = std::string(data, u.field_data[UF_QUERY].len);
            }

            if ((u.field_set & (1 << UF_FRAGMENT))) {
                const char * data = url + u.field_data[UF_FRAGMENT].off;
                client->Fragment = std::string(data, u.field_data[UF_FRAGMENT].len);
            }

            if ((u.field_set & (1 << UF_SCHEMA))) {
                const char * data = url + u.field_data[UF_SCHEMA].off;
                client->Schema = std::string(data, u.field_data[UF_SCHEMA].len);
            }

            if ((u.field_set & (1 << UF_USERINFO))) {
                const char * data = url + u.field_data[UF_USERINFO].off;
                client->UserInfo = std::string(data, u.field_data[UF_USERINFO].len);
            }
        }
        return 0;
    }

    static int on_header_field(http_parser* parser, const char* at, size_t length) {
        HttpClient* client = (HttpClient*)parser->data;
        client->LastHeaderItem = std::string(at, length);
        return 0;
    }

    static int on_header_value(http_parser* parser, const char* at, size_t length) {
        HttpClient* client = (HttpClient*)parser->data;
        client->Headers[client->LastHeaderItem] = std::string(at, length);
        return 0;
    }

    static int on_headers_complete(http_parser* parser) {
        HttpClient* client = (HttpClient*)parser->data;
        client->HeadersEnd = client->Headers.cend();
        client->LastHeaderItem.clear();
        return 0;
    }

    static int on_body(http_parser* parser, const char* at, size_t length) {
        HttpClient* client = (HttpClient*)parser->data;
        client->RequestBuffer = std::string(at, length);
        return 0;
    }

    static int on_message_complete(http_parser* parser) {
        return 0;
    }
};

/* BLOCK CHAIN SERVER */
HttpServer::HttpServer()
    :pimpl(new HttpServerPimpl(this))
{
    parser_settings->on_url = pimpl->on_url;
    parser_settings->on_message_begin = pimpl->on_message_begin;
    parser_settings->on_message_complete = pimpl->on_message_complete;
    parser_settings->on_header_field = pimpl->on_header_field;
    parser_settings->on_header_value = pimpl->on_header_value;
    parser_settings->on_headers_complete = pimpl->on_headers_complete;
    parser_settings->on_body = pimpl->on_body;

    AddController(std::make_shared<InlineController>());
}

void HttpServer::Start(size_t port) {
    AddPreMiddleware(sessionMiddleware);
    AddPreMiddleware(staticFolderMiddleware);

    pimpl->port = port;
    pimpl->Start();
}

void HttpServer::Stop()
{

}

size_t HttpServer::GetPort()
{
    return pimpl->port;
}

void HttpServer::AddStaticFolder(std::string const & pFolderName)
{
    pimpl->staticFolders.push_back(pFolderName);
}

void HttpServer::SetWorkingFolder(std::string const & pFolderName)
{
    pimpl->workingDir = pFolderName;
}

void HttpServer::SetApplicationFolder(std::string const & pFolderName)
{
    pimpl->applicatonDir = pFolderName;
}

void HttpServer::AddController(std::shared_ptr<Controller> controller)
{
    pimpl->controllers["/" + controller->GetControllerName()] = controller;
    controller->GenerateActions();
    pimpl->controllersEnd = pimpl->controllers.cend();
}

void HttpServer::AddPreMiddleware(MiddlewareItem middlewareItem)
{
    pimpl->preMiddlewareItems.push_back(middlewareItem);
    pimpl->preMiddlewareItemsEnd = pimpl->preMiddlewareItems.cend();
}

void HttpServer::AddPostMiddleware(MiddlewareItem middlewareItem)
{
    pimpl->postMiddlewareItems.push_back(middlewareItem);
    pimpl->postMiddlewareItemsEnd = pimpl->postMiddlewareItems.cend();
}

void HttpServer::AddRoute(std::string const & route, RequestType type, RouteItem routeItem)
{
    pimpl->routes["/" + route] = routeItem;
    pimpl->routesEnd = pimpl->routes.cend();
}

void HttpServer::SetMessageReceived(MessageReceivedCallback cb)
{
    pimpl->messageReceivedCallback = cb;
}

void HttpServer::SetClientConnected(ClientConnectedCallback cb)
{
    pimpl->clientConnectedCallback = cb;
}
void HttpServer::SetClientDisconnected(ClientDisconnectedCallback cb)
{
    pimpl->clientDisconnectedCallback = cb;
}

HttpServer::~HttpServer()
{
    delete pimpl;
}
