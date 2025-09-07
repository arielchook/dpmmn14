// RequestHandler.h
// A class to handle client requests for a backup server.
// Author: Ariel Cohen ID 329599187

#pragma once

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <filesystem>
#include <istream>


using boost::asio::ip::tcp;

// A class to handle a single client connection.
// An instance of this class is created for each new connection in its own thread.
class RequestHandler
{
public:
    // Constructor that takes ownership of the connected socket.
    RequestHandler(tcp::socket socket);

    // The main entry point for handling the client. This function will be
    // executed by a thread.
    void handleRequest();

private:
    // Protocol operation codes
    enum class OpCode : uint8_t
    {
        BACKUP = 100,
        RESTORE = 200,
        DELETE_FILE = 201,
        LIST_FILES = 202
    };

    // Protocol status codes for responses
    enum class StatusCode : uint16_t
    {
        RESTORE_SUCCESS = 210,
        LIST_SUCCESS = 211,
        GENERAL_SUCCESS = 212,
        ERROR_NO_FILE = 1001,
        ERROR_NO_FILES_FOR_CLIENT = 1002,
        ERROR_GENERAL = 1003
    };

    // Reads exactly 'length' bytes from the socket into the provided buffer.
    // Handles cases where the data arrives in multiple packets.
    bool readBytes(void *buffer, size_t length);

    // Sends exactly 'length' bytes from the buffer over the socket.
    bool sendBytes(const void *buffer, size_t length);

    // Handlers for specific client requests
    void handleBackup();
    void handleRestore();
    void handleDelete();
    void handleListFiles();

    // Sends a simple status response (for 1002, 1003).
    void sendSimpleStatusResponse(StatusCode status);

    // Sends a response with version, status, name_len, and filename (for 212, 1001).
    void sendFullHeaderResponse(StatusCode status, const std::string &filename);

    // Sends a response that includes content, read from a stream.
    void sendStreamResponse(StatusCode status, const std::string &filename, uint32_t contentSize, std::istream &contentStream);


    tcp::socket m_socket; // The socket for this client connection.
    uint32_t m_userId;    // The user ID from the request header.
};