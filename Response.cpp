#include "Response.h"

Response::Response(std::string const & pContent) : content(std::move(pContent)) {}
Response::Response(std::string const & pContent, std::string pContentType) : content(std::move(pContent)), contentType(std::move(pContentType)) {}

std::string Response::GetContent() const
{
    return content;
}

void Response::SetContent(std::string const & pContent)
{
    content = std::move(pContent);
}

std::string Response::GetContentType() const
{
    return contentType;
}

void Response::SetContentType(std::string const & pContentType)
{
    contentType = std::move(pContentType);
}

HttpResponseType Response::GetResponseType() const
{
    return type;
}

void Response::SetRequestType(HttpResponseType pRequestType)
{
    type = pRequestType;
}


EmptyResponse::EmptyResponse()
{
    type = HttpResponseType::_200;
    contentType = "text/html; charset=UTF-8";
}

std::string EmptyResponse::Render()
{
    return "";
}

ErrorResponse::ErrorResponse()
{
    type = HttpResponseType::_500;
    contentType = "text/html; charset=UTF-8";
}

ErrorResponse::ErrorResponse(std::string const & pContent) : ErrorResponse()
{
    content = std::move(pContent);
}

std::string ErrorResponse::Render()
{
    return content;
}

JsonResponse::JsonResponse(std::string const & pContent)
{
    content = std::move(pContent);
    type = HttpResponseType::_200;
    contentType = "application/json; charset=UTF-8";
}

std::string JsonResponse::Render()
{
    return content;
}

ViewResponse::ViewResponse(std::string const & file, std::shared_ptr<kainjow::mustache::data> pModel) :
    fileName(file)
{
    type = HttpResponseType::_200;
    contentType = "text/html; charset=UTF-8";
    model = pModel;
}

std::string ViewResponse::Render()
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

ContentResponse::ContentResponse(std::string const & pContent)
{
    content = std::move(pContent);
    type = HttpResponseType::_200;
    contentType = "text/html; charset=UTF-8";
}

std::string ContentResponse::Render()
{
    return content;
}

