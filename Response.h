#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <mustache.hpp>

#include <Config.h>


enum class HttpResponseType : int {
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

static std::map<HttpResponseType, std::string> HttpMessages =
{
    {HttpResponseType::_100,"Continue"},
    {HttpResponseType::_101,"Switching Protocols"},
    {HttpResponseType::_102,"Processing"},
    {HttpResponseType::_200,"OK"},
    {HttpResponseType::_201,"Created"},
    {HttpResponseType::_202,"Accepted"},
    {HttpResponseType::_203,"Non-Authoritative Information"},
    {HttpResponseType::_204,"No Content"},
    {HttpResponseType::_205,"Reset Content"},
    {HttpResponseType::_206,"Partial Content"},
    {HttpResponseType::_207,"Multi-Status"},
    {HttpResponseType::_208,"Already Reported"},
    {HttpResponseType::_226,"IM Used"},
    {HttpResponseType::_300,"Multiple Choices"},
    {HttpResponseType::_301,"Moved Permanently"},
    {HttpResponseType::_302,"Found"},
    {HttpResponseType::_303,"See Other"},
    {HttpResponseType::_304,"Not Modified"},
    {HttpResponseType::_305,"Use Proxy"},
    {HttpResponseType::_307,"Temporary Redirect"},
    {HttpResponseType::_308,"Permanent Redirect"},
    {HttpResponseType::_400,"Bad Request"},
    {HttpResponseType::_401,"Unauthorized"},
    {HttpResponseType::_402,"Payment Required"},
    {HttpResponseType::_403,"Forbidden"},
    {HttpResponseType::_404,"Not Found"},
    {HttpResponseType::_405,"Method Not Allowed"},
    {HttpResponseType::_406,"Not Acceptable"},
    {HttpResponseType::_407,"Proxy Authentication Required"},
    {HttpResponseType::_408,"Request Timeout"},
    {HttpResponseType::_409,"Conflict"},
    {HttpResponseType::_410,"Gone"},
    {HttpResponseType::_411,"Length Required"},
    {HttpResponseType::_412,"Precondition Failed"},
    {HttpResponseType::_413,"Payload Too Large"},
    {HttpResponseType::_414,"URI Too Long"},
    {HttpResponseType::_415,"Unsupported Media Type"},
    {HttpResponseType::_416,"Range Not Satisfiable"},
    {HttpResponseType::_417,"Expectation Failed"},
    {HttpResponseType::_421,"Misdirected Request"},
    {HttpResponseType::_422,"Unprocessable Entity"},
    {HttpResponseType::_423,"Locked"},
    {HttpResponseType::_424,"Failed Dependency"},
    {HttpResponseType::_426,"Upgrade Required"},
    {HttpResponseType::_428,"Precondition Required"},
    {HttpResponseType::_429,"Too Many Requests"},
    {HttpResponseType::_431,"Request Header Fields Too Large"},
    {HttpResponseType::_451,"Unavailable For Legal Reasons"},
    {HttpResponseType::_500,"Internal Server Error"},
    {HttpResponseType::_501,"Not Implemented"},
    {HttpResponseType::_502,"Bad Gateway"},
    {HttpResponseType::_503,"Service Unavailable"},
    {HttpResponseType::_504,"Gateway Timeout"},
    {HttpResponseType::_505,"HTTP Version Not Supported"},
    {HttpResponseType::_506,"Variant Also Negotiates"},
    {HttpResponseType::_507,"Insufficient Storage"},
    {HttpResponseType::_508,"Loop Detected"},
    {HttpResponseType::_510,"Not Extended"},
    {HttpResponseType::_511,"Network Authentication Required"}
};


class Response {
public:
    Response() = default;
    Response(std::string const & pContent);
    Response(std::string const & pContent, std::string pContentType);

    virtual ~Response() = default;
    virtual std::string Render() = 0;

    std::string GetContent() const;
    virtual void SetContent(std::string const & pContent);
    std::string GetContentType() const;
    virtual void SetContentType(std::string const & pContentType);
    HttpResponseType GetResponseType() const;
    virtual void SetRequestType(HttpResponseType pRequestType);

protected:
    std::string content;
    std::string contentType;
    HttpResponseType type;
};

class EmptyResponse : public Response{
public:
    EmptyResponse();
    virtual std::string Render() override;

private:
    void SetContent(std::string const &) override {}
};

class ErrorResponse : public Response{
public:
    ErrorResponse();
    ErrorResponse(std::string const & pContent);
    virtual std::string Render() override;
};

class JsonResponse : public Response {
public:
    JsonResponse(std::string const & pContent);
    virtual std::string Render() override;
};

class ViewResponse : public Response {
public:
    ViewResponse(std::string const & file, std::shared_ptr<kainjow::mustache::data> pModel);
    virtual std::string Render() override;

private:
    std::shared_ptr<kainjow::mustache::data> model;
    std::string fileName;
};

class ContentResponse : public Response {
public:
    ContentResponse(std::string const & pContent);
    virtual std::string Render() override;
};

