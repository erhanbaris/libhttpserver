#include <HttpClient.h>
#include <http_parser.h>
#include <TcpClientUv.h>

void onClose(uv_handle_t*);
void sendAsync(uv_async_t *);
void afterWrite(uv_write_t*, int);

HttpClient::HttpClient( /*std::unordered_map<std::string, AnyType>& pSession*/ )
    /*: Session(pSession)*/
{
	Handle = nullptr;
	Parser = nullptr;
	Async = nullptr;
    TcpClient = nullptr;
    Data2 = nullptr;

    ContentType = "application/json charset=utf-8";
    Type = HttpRequestType::_200;
}

HttpClient::~HttpClient()
{
	if (Handle != nullptr)
	{
		delete Handle;
		Handle = nullptr;
	}

	if (Parser != nullptr)
	{
		delete Parser;
		Parser = nullptr;
	}

	if (Async != nullptr)
	{
		delete Async;
		Async = nullptr;
	}

	if (TcpClient != nullptr)
	{
		delete TcpClient;
		TcpClient = nullptr;
	}

	if (Data2 != nullptr)
	{
		delete Data2;
		Data2 = nullptr;
	}
}

void sendAsync(uv_async_t *handle) {
	HttpClient * client = (HttpClient*)handle->data;

	std::string bufferStr = client->ResponseBuffer.str();
	std::ostringstream rep;
	rep << "HTTP/1.1 200 OK\r\n"
		<< "Content-Type: application/json charset=utf-8\r\n"
		<< "Connection: close\r\n"
		<< "Content-Length: " << bufferStr.size() << "\r\n"
		<< "Access-Control-Allow-Origin: *" << "\r\n"
        << "Set-Cookie: ID=" << "" << ";path=/\r\n"
		<< "\r\n";
	rep << bufferStr;
	std::string res = rep.str();

    client->TcpClient->Send(std::move(res));
}

void HttpClient::Send()
{
    this->ResponseBuffer.seekg(0, this->ResponseBuffer.end);
	std::ostringstream rep;
    rep << "HTTP/1.1 " << std::to_string((int)Type) << " " << HttpMessages[Type] <<"\r\n"
        << "Content-Type: " << ContentType << "\r\n"
		<< "Connection: close\r\n"
        << "Server: " << SERVER_NAME << " " << SERVER_VERSION << "\r\n"
        << "Content-Length: " << this->ResponseBuffer.tellg() << "\r\n"
        << "Access-Control-Allow-Origin: *\r\n"
        << "Set-Cookie: ID=" << "" << ";path=/\r\n"
		<< "\r\n";

    this->ResponseBuffer.seekg(0, this->ResponseBuffer.beg);
    rep << ResponseBuffer.str();
    this->TcpClient->Send(rep.str());
}

void afterWrite(uv_write_t* req, int status) {
	if (!uv_is_closing((uv_handle_t*)req->handle))
		uv_close((uv_handle_t*)req->handle, onClose);
}

void onClose(uv_handle_t* handle) {
	HttpClient* client = (HttpClient*)handle->data;

	if (client->Parser != nullptr)
		delete (http_parser*)client->Parser;

	if (client->Handle != nullptr)
		delete (uv_tcp_t*)client->Handle;

	if (client->Async != nullptr)
		delete (uv_async_t*)client->Async;

	delete client;
}
