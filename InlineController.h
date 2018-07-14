//
// Created by Erhan on 9.07.2018.
//

#ifndef REMOTESERVER_INLINECONTROLLER_H
#define REMOTESERVER_INLINECONTROLLER_H

#include <Config.h>
#include <Controller.h>
#include <string>

class InlineController : public Controller {
    public:
        InlineController();
        ~InlineController();

        std::string GetControllerName() override;
        Controller* Create() override;
        void GenerateActions() override;
        void hello(HttpClient* client);
        void BeginRequest(HttpClient* client) override;
        void EndRequest(HttpClient* client) override;
};


#endif //REMOTESERVER_INLINECONTROLLER_H
