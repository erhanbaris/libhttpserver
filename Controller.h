//
// Created by Erhan on 9.07.2018.
//

#ifndef REMOTESERVER_CONTROLLER_H
#define REMOTESERVER_CONTROLLER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <HttpClient.h>

#define ADD_ACTION(name, type) AddAction( #name , [=](HttpClient* client)\
{\
    name (client);\
}, type );

#define ADD_ACTION_WITH_REGEX(name, regex, type) AddAction( #name , [=](HttpClient* client)\
{\
    name (client);\
}, type );

enum class RequestType : int
{
    GET = 0,
    POST = 1,
    DELETE = 2,
    PUT = 3,
};

using ActionFunction = std::function<void(HttpClient *)>;
class Action final {
    public:
        std::string Name;
        ActionFunction Function;
        RequestType Types;
};

class Controller {
    public:
        Controller();
        virtual ~Controller();
        virtual void BeginRequest(HttpClient* client);
        virtual void EndRequest(HttpClient* client);
        virtual void Index();
        std::unordered_map<std::string, std::shared_ptr<Action>> Actions();
        bool HasAction(std::string const & actionName);
        std::shared_ptr<Action> GetAction(std::string const & actionName);


        virtual Controller* Create() = 0;
        virtual std::string GetControllerName() = 0;
        virtual void GenerateActions() = 0;

    protected:
        void AddAction(std::string const & actionName, ActionFunction function, RequestType type);

    private:
        std::unordered_map<std::string, std::shared_ptr<Action>> actions;
        std::unordered_map<std::string, std::shared_ptr<Action>>::iterator actionsEnd;
};

#endif //REMOTESERVER_CONTROLLER_H
