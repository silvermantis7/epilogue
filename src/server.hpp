#pragma once

#include <iostream>
#include <asio.hpp>

#include "process_messages.hpp"

using asio::ip::tcp;

namespace epilogue
{
    struct Connection : public std::enable_shared_from_this<Connection>
    {
        typedef std::shared_ptr<Connection> pointer;
        static pointer create(asio::io_context& io_context)
        {
            return pointer(new Connection(io_context));
        }

        void connect(std::string host, std::string port);
        std::vector<epilogue::Command> read_messages();
        void send_message(std::string message);
        void display_message(const asio::error_code& error,
            std::size_t bytes_transferred);

        void close()
        {
            send_mutex.lock();
            socket.shutdown(asio::ip::tcp::socket::shutdown_both);
            socket.close();
            closed_ = true;
            send_mutex.unlock();
        }

        bool closed() { return closed_; }

    private:
        Connection(asio::io_context& io_context)
            : io_context_{io_context}
            , resolver(io_context_)
            , socket(io_context_)
        {
        }

        asio::io_context& io_context_;
        tcp::resolver resolver;
        tcp::socket socket;

        std::vector<std::string> channels = { "*global*" };

        std::mutex send_mutex;

        // used for when message recieved is too big for read_messages() buffer
        std::string read_overflow = "";

        bool closed_ = false;
    };
}

void epilogue::Connection::connect(std::string host, std::string port)
{
    tcp::resolver::results_type endpoints = resolver.resolve(host, port);
    asio::connect(socket, endpoints);
}

std::vector<epilogue::Command> epilogue::Connection::read_messages()
{
    constexpr unsigned int READ_BUF_SIZE = 512;

    std::array<char, READ_BUF_SIZE> buf;
    std::error_code error;
    size_t len;

    try
    {
        len = socket.read_some(asio::buffer(buf), error);

        if (!len)
        {
            return { };
        }
    }
    catch (std::exception& e)
    {
        return { };
    }

    if (error) { return { }; }

    std::string buffer_data = read_overflow + std::string(buf.data(), len);
    read_overflow = "";

    std::stringstream ss(buffer_data);
    std::string message;
    std::vector<epilogue::Command> messages;

    while(std::getline(ss, message, '\n'))
    {
        if (message.back() == '\r')
        {
            message.pop_back();
        }

        epilogue::Command command = epilogue::process_message(message);

        if (command.cmd_id == epilogue::Command_ID::PING)
        {
            send_message("PONG " + command.body + "\r\n");
        }

        messages.push_back(command);
        std::cout << ">>> " << message << "\n";
    }

    // if last line overruns buffer
    if (buf.data()[len - 1] != '\n')
    {
        read_overflow = message;
        messages.pop_back();
    }

    return messages;
}

void epilogue::Connection::send_message(std::string message)
{
    std::error_code ignored_error;

    send_mutex.lock();

    if (!closed_)
    {
        asio::write(socket, asio::buffer(message), ignored_error);
    }

    send_mutex.unlock();

    if (ignored_error)
    {
        throw std::system_error(ignored_error);
    }
}
