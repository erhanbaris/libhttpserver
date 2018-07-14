#pragma once
#include <vector>
#include <string>

#define INFO std::cout << std::endl
#define ALERT std::cout << std::endl

#include <uv.h>
#include <map>

extern uv_loop_t* loop;
extern size_t HTTP_PORT;
extern size_t TCP_PORT;

static std::string SERVER_NAME = "Remote Server";
static std::string SERVER_VERSION = "0.1";

enum class HttpRequestType : int {
    _100 = 100,
    _101 = 101,
    _102 = 102,
    _200 = 200,
    _201 = 201,
    _202 = 202,
    _203 = 203,
    _204 = 204,
    _205 = 205,
    _206 = 206,
    _207 = 207,
    _208 = 208,
    _226 = 226,
    _300 = 300,
    _301 = 301,
    _302 = 302,
    _303 = 303,
    _304 = 304,
    _305 = 305,
    _307 = 307,
    _308 = 308,
    _400 = 400,
    _401 = 401,
    _402 = 402,
    _403 = 403,
    _404 = 404,
    _405 = 405,
    _406 = 406,
    _407 = 407,
    _408 = 408,
    _409 = 409,
    _410 = 410,
    _411 = 411,
    _412 = 412,
    _413 = 413,
    _414 = 414,
    _415 = 415,
    _416 = 416,
    _417 = 417,
    _421 = 421,
    _422 = 422,
    _423 = 423,
    _424 = 424,
    _426 = 426,
    _428 = 428,
    _429 = 429,
    _431 = 431,
    _451 = 451,
    _500 = 500,
    _501 = 501,
    _502 = 502,
    _503 = 503,
    _504 = 504,
    _505 = 505,
    _506 = 506,
    _507 = 507,
    _508 = 508,
    _510 = 510,
    _511 = 511
};

static std::map<HttpRequestType, std::string> HttpMessages =
{
    {HttpRequestType::_100,"Continue"},
    {HttpRequestType::_101,"Switching Protocols"},
    {HttpRequestType::_102,"Processing"},
    {HttpRequestType::_200,"OK"},
    {HttpRequestType::_201,"Created"},
    {HttpRequestType::_202,"Accepted"},
    {HttpRequestType::_203,"Non-Authoritative Information"},
    {HttpRequestType::_204,"No Content"},
    {HttpRequestType::_205,"Reset Content"},
    {HttpRequestType::_206,"Partial Content"},
    {HttpRequestType::_207,"Multi-Status"},
    {HttpRequestType::_208,"Already Reported"},
    {HttpRequestType::_226,"IM Used"},
    {HttpRequestType::_300,"Multiple Choices"},
    {HttpRequestType::_301,"Moved Permanently"},
    {HttpRequestType::_302,"Found"},
    {HttpRequestType::_303,"See Other"},
    {HttpRequestType::_304,"Not Modified"},
    {HttpRequestType::_305,"Use Proxy"},
    {HttpRequestType::_307,"Temporary Redirect"},
    {HttpRequestType::_308,"Permanent Redirect"},
    {HttpRequestType::_400,"Bad Request"},
    {HttpRequestType::_401,"Unauthorized"},
    {HttpRequestType::_402,"Payment Required"},
    {HttpRequestType::_403,"Forbidden"},
    {HttpRequestType::_404,"Not Found"},
    {HttpRequestType::_405,"Method Not Allowed"},
    {HttpRequestType::_406,"Not Acceptable"},
    {HttpRequestType::_407,"Proxy Authentication Required"},
    {HttpRequestType::_408,"Request Timeout"},
    {HttpRequestType::_409,"Conflict"},
    {HttpRequestType::_410,"Gone"},
    {HttpRequestType::_411,"Length Required"},
    {HttpRequestType::_412,"Precondition Failed"},
    {HttpRequestType::_413,"Payload Too Large"},
    {HttpRequestType::_414,"URI Too Long"},
    {HttpRequestType::_415,"Unsupported Media Type"},
    {HttpRequestType::_416,"Range Not Satisfiable"},
    {HttpRequestType::_417,"Expectation Failed"},
    {HttpRequestType::_421,"Misdirected Request"},
    {HttpRequestType::_422,"Unprocessable Entity"},
    {HttpRequestType::_423,"Locked"},
    {HttpRequestType::_424,"Failed Dependency"},
    {HttpRequestType::_426,"Upgrade Required"},
    {HttpRequestType::_428,"Precondition Required"},
    {HttpRequestType::_429,"Too Many Requests"},
    {HttpRequestType::_431,"Request Header Fields Too Large"},
    {HttpRequestType::_451,"Unavailable For Legal Reasons"},
    {HttpRequestType::_500,"Internal Server Error"},
    {HttpRequestType::_501,"Not Implemented"},
    {HttpRequestType::_502,"Bad Gateway"},
    {HttpRequestType::_503,"Service Unavailable"},
    {HttpRequestType::_504,"Gateway Timeout"},
    {HttpRequestType::_505,"HTTP Version Not Supported"},
    {HttpRequestType::_506,"Variant Also Negotiates"},
    {HttpRequestType::_507,"Insufficient Storage"},
    {HttpRequestType::_508,"Loop Detected"},
    {HttpRequestType::_510,"Not Extended"},
    {HttpRequestType::_511,"Network Authentication Required"}
};

class Request {
    public:
        Request() = default;
        virtual ~Request() = default;
};

class Response {
    public:
        Response() = default;
        Response(std::string const & pContent) : content(std::move(pContent)) {}
        Response(std::string const & pContent, std::string pContentType) : content(std::move(pContent)), contentType(std::move(pContentType)) {}

        virtual ~Response() = default;
        virtual std::string Render() = 0;

        std::string GetContent()
        {
            return content;
        }

        virtual void SetContent(std::string const & pContent)
        {
            content = std::move(pContent);
        }

        std::string GetContentType()
        {
            return contentType;
        }

        virtual void SetContentType(std::string const & pContentType)
        {
            contentType = std::move(pContentType);
        }

        HttpRequestType GetRequestType()
        {
            return type;
        }

        virtual void SetRequestType(HttpRequestType pRequestType)
        {
            type = pRequestType;
        }

    protected:
        std::string content;
        std::string contentType;
        HttpRequestType type;
};

class EmptyResponse : public Response{
    public:
        EmptyResponse()
        {
            type = HttpRequestType::_200;
            contentType = "text/html; charset=UTF-8";
        }

        virtual std::string Render() override
        {
            return "";
        }

    private:
        void SetContent(std::string const &) override {}
};

class ErrorResponse : public Response{
    public:
        ErrorResponse()
        {
            type = HttpRequestType::_500;
            contentType = "text/html; charset=UTF-8";
        }

        ErrorResponse(std::string const & pContent) : ErrorResponse()
        {
            content = std::move(pContent);
        }

        virtual std::string Render() override
        {
            return content;
        }
};

class JsonResponse : public Response {
    public:
        JsonResponse(std::string const & pContent)
        {
            content = std::move(pContent);
            type = HttpRequestType::_200;
            contentType = "application/json; charset=UTF-8";
        }

        virtual std::string Render() override
        {
            return content;
        }
};

#include <mustache.hpp>
#include <iostream>
#include <fstream>

class ViewResponse : public Response {
    public:
        ViewResponse(std::string const & file, std::shared_ptr<kainjow::mustache::data> pModel) :
            fileName(file)
        {
            type = HttpRequestType::_200;
            contentType = "text/html; charset=UTF-8";
            model = pModel;
        }

        virtual std::string Render() override
        {
            std::fstream myfile;
            myfile.open (fileName);
            if (myfile.is_open()) {
                std::string str;

                myfile.seekg(0, std::ios::end);
                str.reserve(myfile.tellg());
                myfile.seekg(0, std::ios::beg);

                str.assign((std::istreambuf_iterator<char>(myfile)), std::istreambuf_iterator<char>());
                kainjow::mustache::mustache tmpl{str};
                myfile.close();
                std::string data = tmpl.render(*model.get());
                return data;
            }

            myfile.close();
            return "VIEW NOT FOUND!";
        }

    private:
        std::shared_ptr<kainjow::mustache::data> model;
        std::string fileName;
};

class ContentResponse : public Response {
    public:
        ContentResponse(std::string const & pContent)
        {
            content = std::move(pContent);
            type = HttpRequestType::_200;
            contentType = "text/html; charset=UTF-8";
        }

        virtual std::string Render() override
        {
            return content;
        }
};

class AnyType
{
    public:
        enum class AnyTypeEnum
        {
            EMPTY,
            STRING,
            INT,
            DOUBLE,
            POINTER,
            BOOL
        };

        union {
                int    Int;
                std::string String;
                double Double;
                bool Bool;
                void* Pointer;
        };

        AnyTypeEnum Type;

        AnyType() { Type = AnyTypeEnum::EMPTY; }
        AnyType(int data) { Int = data; Type = AnyTypeEnum::EMPTY; }
        AnyType(std::string & data): String(std::move(data)) { Type = AnyTypeEnum::STRING; }
        AnyType(double data) { Double = data; Type = AnyTypeEnum::DOUBLE; }
        AnyType(bool data) { Bool = data; Type = AnyTypeEnum::BOOL; }
        AnyType(void* data) { Pointer = data; Type = AnyTypeEnum::POINTER; }

        ~AnyType()
        {
            if (Type == AnyTypeEnum::STRING)
                String.~basic_string();
        }
};

#define BEGIN_INIT_WITH_NAME(name) namespace { class __class_##name { public: __class_##name () {
#define END_INIT_WITH_NAME(name) } }; __class_##name init_##name; }

#define ADD_ROUTE(name, route, type, app, function) Response * name function \
    namespace { \
    BEGIN_INIT_WITH_NAME(name)\
    addRouteToApp( app , #route , type , name );\
    END_INIT_WITH_NAME(name)\
    }
