#include <exception>
#include <stdexcept>

#include <HttpClient.h>
#include <http_parser.h>
#include <HttpClient.h>
#include <Tools.h>

void onClose(uv_handle_t*);
void afterWrite(uv_write_t*, int);


struct HttpClientPimpl
{
    std::string address;
    size_t port;
    std::string sendMessage;
    bool largeBufferStarted;
    std::stringstream tmpBuffer;

    HttpClient* tcpClient;

    // uv
    uv_async_t* asyncOperation;
    uv_async_t* closeAsync;
    uv_tcp_t* client;
    uv_connect_t* connect;

    // Callbacks
    HttpClient::MessageCallback messageCallback;
    HttpClient::ConnectCallback connectCallback;

    HttpClientPimpl(HttpClient* pTcpClient) : tcpClient(pTcpClient)
    {
        asyncOperation = NULL;
        client = NULL;
        connect = NULL;

        messageCallback = [](std::string const &, HttpClient *) { std::cout << "HttpClient 'messageCallback' Not Setted" << std::endl; };
        connectCallback = [](HttpClient *) { std::cout << "HttpClient 'connectCallback' Not Setted" << std::endl; };
    }

    ~HttpClientPimpl()
    {
        if (asyncOperation != NULL)
            delete asyncOperation;

        if (client != NULL)
            delete client;

        if (connect != NULL)
            delete connect;
    }

    /* libuv callbacks */
    static void closeCb(uv_handle_t* handle)
    {
        HttpClientPimpl * pimpl = (HttpClientPimpl*)handle->data;
        // pimpl->disconnectCallback(pimpl->tcpClient);
    }

    static void allocCb(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
        *buf = uv_buf_init((char*)malloc(size), size);
    }

    static void readCb(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
    {
        HttpClientPimpl * pimpl = (HttpClientPimpl*)tcp->data;

        if (nread >= 0) {
            char* tmpData = _strndup(buf->base, nread);
            pimpl->tmpBuffer << tmpData;

            if ((size_t)nread == buf->len && 65536 == nread)
                pimpl->largeBufferStarted = true;

            else if (pimpl->largeBufferStarted && (size_t)nread != buf->len)
                pimpl->largeBufferStarted = false;

            if (!pimpl->largeBufferStarted)
            {
                auto message = pimpl->tmpBuffer.str();
                pimpl->messageCallback(message, pimpl->tcpClient);
                pimpl->tmpBuffer.str(std::string());
            }

            delete tmpData;
        }
        else {
            uv_close((uv_handle_t*)tcp, closeCb);
        }

        free(buf->base);
    }

    static void afterSend(uv_write_t* handle, int status) {
        HttpClientPimpl * pimpl = (HttpClientPimpl*)handle->data;
        pimpl->sendMessage.clear();
    }

    static void afterSendAndClose(uv_write_t* handle, int status) {
        HttpClientPimpl * pimpl = (HttpClientPimpl*)handle->data;
        pimpl->sendMessage.clear();
        uv_close((uv_handle_t*)pimpl->client, closeCb);
    }

    static void async(uv_async_t *handle) {
        HttpClientPimpl * pimpl = (HttpClientPimpl*)handle->data;

        uv_buf_t resbuf;
        resbuf.base = const_cast<char *>(pimpl->sendMessage.c_str());
        resbuf.len = pimpl->sendMessage.size();

        uv_write_t *write_req = new uv_write_t;
        write_req->data = pimpl;

        uv_write(write_req, (uv_stream_t *)pimpl->client, &resbuf, 1, afterSendAndClose);
    }

    static void onConnect(uv_connect_t* connection, int status)
    {
        if (status < 0) {
            return;
        }

        HttpClientPimpl * pimpl = (HttpClientPimpl*)connection->data;

        pimpl->asyncOperation = new uv_async_t;
        pimpl->asyncOperation->data = pimpl;

        uv_stream_t* stream = connection->handle;
        stream->data = pimpl;
        uv_async_init(loop, ((uv_async_t*)pimpl->asyncOperation), HttpClientPimpl::async);
        uv_read_start(stream, allocCb, readCb);

        pimpl->connectCallback(pimpl->tcpClient);
    }
    /* libuv callbacks */
};



HttpClient::HttpClient(void* serverHandle)
{
    Parser = nullptr;

    uv_stream_t* server = static_cast<uv_stream_t*>(serverHandle);
    if (server == nullptr)
        throw std::invalid_argument("'serverHandle' is invalid");

    ContentType = "application/json charset=utf-8";
    Type = HttpResponseType::_200;

    pimpl = new HttpClientPimpl(this);
    pimpl->asyncOperation = new uv_async_t;
    pimpl->asyncOperation->data = pimpl;

    pimpl->client = new uv_tcp_t;
    pimpl->client->data = pimpl;

    uv_tcp_init(loop, pimpl->client);
    uv_accept((uv_stream_t*)serverHandle, (uv_stream_t*)pimpl->client);

    struct sockaddr_in name;
    int namelen = sizeof(name);
    if (uv_tcp_getpeername(pimpl->client, (struct sockaddr*) &name, &namelen))
        INFO << "uv_tcp_getpeername";

    char addr[16];
    uv_inet_ntop(AF_INET, &name.sin_addr, addr, sizeof(addr));

    pimpl->address = std::string(addr);
    pimpl->port = name.sin_port;


    uv_async_init(loop, ((uv_async_t*)pimpl->asyncOperation), HttpClientPimpl::async);
    uv_read_start((uv_stream_t*)pimpl->client, HttpClientPimpl::allocCb, HttpClientPimpl::readCb);
}

HttpClient::~HttpClient()
{

	if (Parser != nullptr)
	{
		delete Parser;
		Parser = nullptr;
	}
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

    pimpl->sendMessage = rep.str();
    uv_async_send(pimpl->asyncOperation);
}

void afterWrite(uv_write_t* req, int status) {
	if (!uv_is_closing((uv_handle_t*)req->handle))
		uv_close((uv_handle_t*)req->handle, onClose);
}

void onClose(uv_handle_t* handle) {
	HttpClient* client = (HttpClient*)handle->data;

	if (client->Parser != nullptr)
		delete (http_parser*)client->Parser;

	delete client;
}

std::string HttpClient::GetRemoteAddress()
{
    return pimpl->address;
}

size_t HttpClient::GetRemotePort()
{
    return pimpl->port;
}

void HttpClient::SetOnMessage(MessageCallback cb)
{
    pimpl->messageCallback = cb;
}
