#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <functional>
#include <memory>

#include <Config.h>
#include <Controller.h>
#include <HttpClient.h>

struct HttpServerPimpl;
class HttpServer
{
public:

    using RouteItem = std::function<Response*(HttpClient*)>;
    using MiddlewareItem = std::function<bool(HttpClient*)>;

	typedef std::function<void(HttpClient *)> MessageReceivedCallback;
	typedef std::function<void(HttpClient *)> ClientConnectedCallback;
	typedef std::function<void(HttpClient *)> ClientDisconnectedCallback;


	HttpServer();
	virtual ~HttpServer();
	virtual void Start(size_t port);
	virtual void Stop();
	virtual size_t GetPort();

    virtual void AddStaticFolder(std::string const & pFolderName);
    virtual void SetWorkingFolder(std::string const & pFolderName);
    virtual void SetApplicationFolder(std::string const & pFolderName);

    virtual void AddController(std::shared_ptr<Controller>);
    virtual void AddPreMiddleware(MiddlewareItem middlewareItem);
    virtual void AddPostMiddleware(MiddlewareItem middlewareItem);
    virtual void AddRoute(std::string const & route, RequestType type, RouteItem routeItem);

    virtual void SetMessageReceived(MessageReceivedCallback);
	virtual void SetClientConnected(ClientConnectedCallback);
	virtual void SetClientDisconnected(ClientDisconnectedCallback);

private:
	HttpServerPimpl* pimpl;
};
