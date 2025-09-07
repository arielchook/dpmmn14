// RequestHandler.h
// Main server application for handling client connections.
// Author: Ariel Cohen ID 329599187

#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include "RequestHandler.h"

using boost::asio::ip::tcp;

// The port number the server will listen on.
const int DEFAULT_PORT = 1234;

// This function is executed in a new thread for each client.
// It creates a RequestHandler to process the client's requests.
void session(tcp::socket sock)
{
    try
    {
        RequestHandler handler(std::move(sock));
        handler.handleRequest();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

int main(int argc, char* argv[])
{
    try
    {
        int port = DEFAULT_PORT;
        if (argc > 1)
        {
            port = std::atoi(argv[1]);
        }

        // Boost.Asio requires an io_context object.
        boost::asio::io_context io_context;

        // The acceptor object listens for new connections.
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));

        std::cout << "Server listening on port " << port << "..." << std::endl;

        // The main server loop.
        while (true)
        {
            // Wait for a client to connect. accept() is a blocking call.
            tcp::socket socket = acceptor.accept();

            // When a connection is accepted, create a new thread to handle it.
            // std::move(socket) transfers ownership of the socket to the thread.
            // .detach() allows the main loop to continue listening without waiting for the thread to finish.
            std::cout << "New connection accepted." << std::endl;
            std::thread(session, std::move(socket)).detach();
        }
    }

    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
