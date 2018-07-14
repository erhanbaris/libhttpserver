//
// Created by Erhan on 9.07.2018.
//

#include "Controller.h"

Controller::Controller()
{

}

Controller::~Controller()
{

}

void Controller::BeginRequest(HttpClient* client)
{

}

void Controller::EndRequest(HttpClient* client)
{

}

void Controller::Index()
{

}


std::unordered_map<std::string, std::shared_ptr<Action>> Controller::Actions()
{
    return actions;
}

void Controller::AddAction(std::string const & actionName, ActionFunction function, RequestType type)
{
    std::shared_ptr<Action> action = std::make_shared<Action>();
    action->Function = function;
    action->Name = actionName;
    action->Types = type;

    actions[actionName] = action;
    actionsEnd = actions.end();
}

bool Controller::HasAction(std::string const & actionName)
{
    return actions.find(actionName) != actionsEnd;
}

std::shared_ptr<Action> Controller::GetAction(std::string const & actionName)
{
    return actions.find(actionName)->second;
}
