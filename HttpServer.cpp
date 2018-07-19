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

#include <memory>

#include <HttpClient.h>
#include <HttpServer.h>
#include <Config.h>
#include <Controller.h>
#include <InlineController.h>
#include <set>


uv_loop_t* loop = NULL;
namespace
{
    static http_parser_settings* parser_settings;
}

bool staticFolderMiddleware(HttpClient* client)
{
    //std::cout << "staticFolderMiddleware " << client->Path << std::endl;
    return true;
}

bool sessionMiddleware(HttpClient* client)
{
    //client->Headers
    return true;
}

class file_system_watcher{
    public:
        uv_fs_event_t* Event;
        std::string Path;

        file_system_watcher(uv_fs_event_t* event, std::string const & path): Event(event), Path(path) {}
};

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

        std::unordered_map<std::string, std::string> staticFiles;
        std::unordered_map<std::string, std::string>::const_iterator staticFilesEnd;

        std::vector<std::unique_ptr<file_system_watcher>> staticFolders;
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
            loop = uv_default_loop();

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

        bool checkStaticFiles(HttpClient* client)
        {
            std::unordered_map<std::string, std::string>::const_iterator staticFileStatus = staticFiles.find(client->Path);
            if (staticFileStatus != staticFilesEnd)
            {
                std::fstream myfile;
                myfile.open (staticFileStatus->second);
                if (myfile.is_open()) {
                    std::string str;

                    myfile.seekg(0, std::ios::end);
                    str.reserve(myfile.tellg());
                    myfile.seekg(0, std::ios::beg);

                    str.assign((std::istreambuf_iterator<char>(myfile)), std::istreambuf_iterator<char>());
                    myfile.close();

                    std::string::size_type idx;
                    client->ContentType = "text/plain";

                    idx = staticFileStatus->second.rfind('.');

                    if(idx != std::string::npos)
                    {
                        std::string extension = staticFileStatus->second.substr(idx+1);

                        if (ContentTypes.find(extension) != ContentTypes.cend())
                        {
                            client->ContentType =  ContentTypes[extension];
                        }
                    }

                    client->ResponseBuffer << str;
                    client->Type = HttpResponseType::_200;
                }
                else {
                    client->ResponseBuffer << "File Not Found";
                    client->Type = HttpResponseType::_404;
                }

                return true;
            }

            return false;
        }

        bool checkRoutes(HttpClient* client)
        {
            std::unordered_map<std::string, HttpServer::RouteItem>::const_iterator routeCallback = routes.find(client->Path);
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

                return true;
            }

            return false;
        }

        bool checkController(HttpClient* client)
        {
            std::unordered_map<std::string, std::shared_ptr<Controller>>::const_iterator callback = controllers.find(client->Path);
            if (callback != controllersEnd)
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
                return true;
            }

            return false;
        }

        void parseRequest(HttpClient* client)
        {
            bool requestExecuted = checkStaticFiles(client) || checkRoutes(client) || checkController(client);

            if (!requestExecuted)
            {
                client->ResponseBuffer << "Page not found";
                client->ContentType = ContentTypes["html"];
                client->Type = HttpResponseType::_404;
            }

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

void run_command(uv_fs_event_t *handle, const char *filename, int events, int status) {
    char path[1024];
    size_t size = 1023;
    // Does not handle error if path is longer than 1023.
    uv_fs_event_getpath(handle, path, &size);
    path[size] = '\0';

    fprintf(stderr, "Change detected in %s: ", path);
    if (events & UV_RENAME)
        fprintf(stderr, "renamed");
    if (events & UV_CHANGE)
        fprintf(stderr, "changed");


    fprintf(stderr, " %s\n", filename ? filename : "");
    // system(command);
}

class ServerBehavior {
    public:
        virtual void Start() = 0;
        virtual void Finished() = 0;
};

class FolderScanner {
    public:
        struct __sub_folder_search {
                FolderScanner * Scanner;
                std::string SubFolderName;
        };

        using FileCallback = std::function<void(FolderScanner*, std::string const &)>;
        FileCallback Callback;
        uv_work_t Worker;

        std::string StartPath;
        std::string VirtualPath;
        std::set<std::string> ScannedFolders;
        size_t DirCounter;

        FolderScanner()
        {
            DirCounter = 1;
        }

        ~FolderScanner() { }

        void Start()
        {
            Worker.data = this;
            uv_queue_work(loop, &Worker, FolderScanner::WorkerFunction, nullptr);
        }

        static void WorkerFunction(uv_work_t *req) {
            FolderScanner* lister = static_cast<FolderScanner*>(req->data);

            int r;
            uv_fs_t* scanReq = new uv_fs_t;

            __sub_folder_search* search = new __sub_folder_search;
            search->Scanner = lister;
            search->SubFolderName = "";
            scanReq->data = search;

            r = uv_fs_scandir(loop, scanReq, lister->StartPath.c_str(), O_RDONLY, FolderScanner::Scan);
            if (r < 0) {
                printf("Error at opening file: %s\n", uv_strerror(r));
            }
        }

        static void Scan(uv_fs_t *req) {
            __sub_folder_search* search = static_cast<__sub_folder_search*>(req->data);
            --search->Scanner->DirCounter;

            int r, i;
            uv_dirent_t dent;

            while (UV__EOF != uv_fs_scandir_next(req, &dent)) {
                if (dent.type == UV_DIRENT_DIR)
                {
                    std::string fullPath = std::string(req->path) + "/" + std::string(dent.name);
                    if (search->Scanner->ScannedFolders.count(fullPath) == 0)
                    {
                        ++search->Scanner->DirCounter;
                        __sub_folder_search* subSearch = new __sub_folder_search;
                        subSearch->Scanner = search->Scanner;
                        subSearch->SubFolderName = search->SubFolderName + "/" + dent.name;

                        uv_fs_t* scanReq = new uv_fs_t;
                        scanReq->data = subSearch;

                        int r;
                        r = uv_fs_scandir(loop, scanReq, fullPath.c_str(), O_RDONLY, FolderScanner::Scan);
                        if (r < 0) {
                            printf("Error at opening file: %s\n", uv_strerror(r));
                        }
                    }
                }
                else if (dent.type == UV_DIRENT_FILE)
                {
                    std::string fullPath = search->SubFolderName + "/" + std::string(dent.name);

                    if (search->Scanner->Callback)
                        search->Scanner->Callback(search->Scanner, fullPath);
                }
            }

            uv_fs_t* closeReq = new uv_fs_t;
            r = uv_fs_close(loop, closeReq, req->file, FolderScanner::Close);
            if (search->Scanner->DirCounter == 0)
            {
                delete search->Scanner; search->Scanner = nullptr;
            }

            delete req;
            req = nullptr;

            delete search;
            search = nullptr;
        }

        static void Close(uv_fs_t* req)
        {
            FolderScanner* lister = static_cast<FolderScanner*>(req->data);
            delete req;
        }
};

void HttpServer::AddStaticFolder(std::string const & pFolderName, std::string const & virtualPath)
{
    uv_fs_event_t* fs_event_req = new uv_fs_event_t;
    fs_event_req->data = pimpl;
    uv_fs_event_init(loop, fs_event_req);
    uv_fs_event_start(fs_event_req, run_command, pFolderName.c_str(), UV_FS_EVENT_RECURSIVE);

    FolderScanner* lister = new FolderScanner;
    lister->StartPath = pFolderName;
    lister->VirtualPath = virtualPath;

    HttpServerPimpl* tmpPimpl = pimpl;
    lister->Callback = [tmpPimpl](FolderScanner* scanner, std::string const & file)
    {
        tmpPimpl->staticFiles["/" + scanner->VirtualPath + file] = scanner->StartPath + file;
        tmpPimpl->staticFilesEnd = tmpPimpl->staticFiles.cend();
    };
    lister->Start();

    pimpl->staticFolders.push_back(std::unique_ptr<file_system_watcher>(new file_system_watcher(fs_event_req, pFolderName)));
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
