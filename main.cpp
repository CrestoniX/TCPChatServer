#include <iostream>
#include <optional>
#include <boost/asio.hpp>
#include <unordered_set>
#include <queue>
#include <functional>
#include <sstream>

using Socket = boost::asio::ip::tcp::socket;
using Endpoint = boost::asio::ip::tcp::endpoint;
using Acceptor = boost::asio::ip::tcp::acceptor;
using namespace std::placeholders;

class session : public std::enable_shared_from_this<session>
{
public:
    session(Socket&& socket):
    socket(std::move(socket))
    {

    }
    void start(std::function<void(std::string)>&& message_handler, std::function<void()>&& error_handler)
    {
        this->message_handler = std::move(message_handler);
        this->error_handler = std::move(error_handler);
        async_read();
    }
    void post(std::string const& message)
    {
        bool flag = message_queue.empty();
        message_queue.emplace(message);
        if(flag)
        {
            async_write();
        }
    }
private:
    void async_read()
    {
        boost::asio::async_read_until(socket, streambuf, '\n', std::bind(&session::on_read, shared_from_this(), _1, _2));
    }
    void on_read(boost::system::error_code error_code, std::uint16_t bytes_transfered)
    {
        if(!error_code)
        {
            std::stringstream message;
            message << socket.remote_endpoint(error_code) << " : " <<std::istream(&streambuf).rdbuf();
            streambuf.consume(bytes_transfered);
            message_handler(message.str());
            async_read();

        }
        else
        {
            socket.close();
            error_handler();
        }
    }
    void async_write()
    {
        boost::asio::async_write(socket, boost::asio::buffer(message_queue.front()), std::bind(&session::on_write, shared_from_this(), _1, _2));
    }
    void on_write(boost::system::error_code errorCode, std::uint16_t bytes_transfered)
    {
        if(!errorCode)
        {
            message_queue.pop();
            if(!message_queue.empty())
            {
                async_write();
            }
        }
        else
        {
            socket.close();
            error_handler();
        }

    }
    Socket socket;
    boost::asio::streambuf streambuf;
    std::function<void(std::string)> message_handler;
    std::function<void()> error_handler;
    std::queue<std::string> message_queue;
};
class Server
{
public:
    Server(boost::asio::io_context& io_context, std::uint16_t port):
    io_context(io_context), acceptor(io_context, Endpoint(boost::asio::ip::tcp::v4(), port))
    {}
private:
    std::optional<Socket> socket;
    Acceptor acceptor;
    boost::asio::io_context& io_context;
    std::unordered_set<std::shared_ptr<session>> clients;

public:
    void async_accept()
    {
        socket.emplace(io_context);
        acceptor.async_accept(*socket, [&](boost::system::error_code errorCode){
           auto client = std::make_shared<session>(std::move(*socket));
           client->post("Welcome to chat\n\r");
           post("We have a new user connected!\n\r");
           clients.insert(client);
           client->start(std::bind(&Server::post, this, _1), [&, weak = std::weak_ptr(client)](){
               if(auto shared = weak.lock(); shared && clients.erase(shared))
               {
                   post("One user has disconnected\n\r");
               }
           });
        async_accept();

        });
    }
    void post(const std::string& message)
    {
        for(auto& client : clients)
        {
            client->post(message);
        }
    }
};
int main()
{
    boost::asio::io_context io_context;
    Server srv(io_context, 15001);
    srv.async_accept();
    io_context.run();
    return 0;
}
