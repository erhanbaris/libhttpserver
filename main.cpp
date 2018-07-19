#include <iostream>

#include <Config.h>
#include <string>
#include <sstream>
#include <Tools.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <HttpServer.h>
#include <Controller.h>
#include <Tools.h>
#include <mustache.hpp>
#include <sstream>

using namespace std;
using namespace kainjow::mustache;

size_t HTTP_PORT;
size_t TCP_PORT;

class RemoteApp{};
class Response;

void addRouteToApp(HttpServer & app, string const & route, RequestType type, HttpServer::RouteItem function)
{
    app.AddRoute(route, type, function);
}

HttpServer server;
ADD_ROUTE(test2, test2, RequestType::GET, server, (HttpClient* client){
    return new JsonResponse("{'merhaba':'dünya'}");
});


int main(int argc, char *argv[])
{
    HTTP_PORT = 9090;

    addRouteToApp(server, "1", RequestType::GET, [=](HttpClient* client){
            return new ErrorResponse("HATA OLUSTU");
    });

    addRouteToApp(server, "2", RequestType::GET, [=](HttpClient* client){
            return new ContentResponse("CONTENT VARR");
    });

    addRouteToApp(server, "files", RequestType::GET, [=](HttpClient* client){
        std::vector<ReadDirResult> files;
        read_directory("/", files);
        std::shared_ptr<kainjow::mustache::data> model = std::make_shared<kainjow::mustache::data>(data::type::object);
        kainjow::mustache::data items {data::type::list};

        for(auto const& item: files)
        {
            if (item.IsDir == false)
                items << kainjow::mustache::data{"name", item.Name};
        }

        (*model.get()).set("files", items);
        (*model.get()).set("adim", "baris");

        return new ViewResponse("/Users/erhanbaris/ClionProjects/RemoteServer/webcontent/files.tpl", model);
    });

    addRouteToApp(server, "3", RequestType::GET, [=](HttpClient* client){
            return new JsonResponse("{'merhaba':'dünya'}");
    });

    INFO << "Http Server Port : " << HTTP_PORT << std::endl;
    //server.AddMiddleware(testMiddleware);
    server.AddStaticFolder("/Users/erhanbaris/ClionProjects/RemoteServer/webcontent", "static");
    server.AddStaticFolder("/Volumes/BOOTCAMP/", "film");
    server.Start(HTTP_PORT);

    return 0;
}
