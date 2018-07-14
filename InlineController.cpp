//
// Created by Erhan on 9.07.2018.
//

#include "InlineController.h"

InlineController::InlineController()
{

}

InlineController::~InlineController()
{

}

 std::string InlineController::GetControllerName()
{
    return "__inline";
}

Controller* InlineController::Create()
{
    return new InlineController;
}

void InlineController::GenerateActions()
{
    ADD_ACTION(hello, RequestType::GET);
}

void InlineController::hello(HttpClient* client)
{
    client->ResponseBuffer << "hello worldasdasdasd";
    client->Send();
}

void InlineController::BeginRequest(HttpClient* client)
{
    std::cout << "Begin Request" << std::endl;
}

void InlineController::EndRequest(HttpClient* client)
{
    std::cout << "End Request" << std::endl;
}
