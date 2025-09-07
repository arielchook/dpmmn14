// RequestHandler.h
// A class to handle client requests for a backup server.
// Author: Ariel Cohen ID 329599187

#include "RequestHandler.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <random>
#include <algorithm>
#include <string>
#include <iomanip>

#define DEBUG 1

namespace fs = std::filesystem;

#pragma pack(push, 1)
// Represents the header of a client request.
// The structure is packed to match the byte-by-byte layout of the protocol.
struct RequestHeader
{
    uint32_t userId;
    uint8_t version;
    uint8_t op;
};

// Represents the header of a server response.
struct ResponseHeader
{
    uint8_t version;
    uint16_t status;
};
#pragma pack(pop)

// Base path for storing all user backups.
const fs::path BASE_BACKUP_PATH = "C:\\backupsvr";
const uint8_t SERVER_VERSION = 1;

// Helper to print a byte buffer in hex format for debugging.
void print_hex(const char *description, const void *data, size_t length)
{
    if (DEBUG)
    {
        std::cout << description;
        const unsigned char *p = static_cast<const unsigned char *>(data);
        std::cout << std::hex << std::setfill('0');
        for (size_t i = 0; i < length; ++i)
        {
            std::cout << std::setw(2) << static_cast<int>(p[i]);
        }
        std::cout << std::dec << std::endl; // Reset to decimal for other outputs
    }
}

RequestHandler::RequestHandler(tcp::socket socket) : m_socket(std::move(socket)) {}

// The main handler function for a connection.
void RequestHandler::handleRequest()
{
    try
    {
        while (true) // Loop to handle multiple requests
        {
            RequestHeader header;
            if (!readBytes(&header, sizeof(header)))
            {
                // This will happen if the client closes the connection (EOF) or on a read error.
                // No error message needed for clean disconnect.
                break; // Exit the loop
            }

            m_userId = header.userId;
            std::cout << "Received request from user " << m_userId << " with op code " << (int)header.op << std::endl;

            // Ensure the base backup directory exists.
            fs::create_directories(BASE_BACKUP_PATH);

            // Dispatch to the correct handler based on the operation code.
            switch (static_cast<OpCode>(header.op))
            {
            case OpCode::BACKUP:
                handleBackup();
                break;
            case OpCode::RESTORE:
                handleRestore();
                break;
            case OpCode::DELETE_FILE:
                handleDelete();
                break;
            case OpCode::LIST_FILES:
                handleListFiles();
                break;
            default:
                std::cerr << "Unknown operation code: " << (int)header.op << std::endl;
                sendFullHeaderResponse(StatusCode::ERROR_GENERAL, ""); // Use empty filename for unknown op
                break;
            }
        }
    }
    catch (const boost::system::system_error &e)
    {
        // Don't log EOF as an error, it's a clean disconnect.
        if (e.code() != boost::asio::error::eof)
        {
            std::cerr << "Socket Error: " << e.what() << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        // Don't try to send a response if the socket is already broken.
    }
    // The socket is automatically closed when the RequestHandler object is destroyed.
    std::cout << "Client " << m_userId << " disconnected." << std::endl;
}

// Handles a request to back up a file.
void RequestHandler::handleBackup()
{
    std::cout << "Handling BACKUP request for user " << m_userId << std::endl;
    uint16_t nameLen;
    if (!readBytes(&nameLen, sizeof(nameLen)))
        return;

    std::cout << "Filename length: " << nameLen << std::endl;
    std::string filename(nameLen, '\0');
    if (!readBytes(&filename[0], nameLen))
        return;

    std::cout << "Filename: " << filename << std::endl;
    uint32_t payloadSize;
    if (!readBytes(&payloadSize, sizeof(payloadSize)))
        return;

    std::cout << "Payload size: " << payloadSize << " bytes" << std::endl;

    std::vector<char> fileContent(payloadSize);
    if (!readBytes(fileContent.data(), payloadSize))
        return;

    std::cout << "Received file content for " << filename << std::endl;
    // Construct the user's personal backup directory path.
    fs::path userDir = BASE_BACKUP_PATH / std::to_string(m_userId);
    fs::create_directories(userDir); // Create it if it doesn't exist.

    fs::path filePath = userDir / filename;

    std::cout << "Storing file at: " << filePath << std::endl;

    // Write the received file content to disk.
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile)
    {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        sendFullHeaderResponse(StatusCode::ERROR_GENERAL, filename);
        return;
    }
    outFile.write(fileContent.data(), fileContent.size());
    outFile.close();

    std::cout << "Successfully backed up file: " << filePath << std::endl;
    sendFullHeaderResponse(StatusCode::GENERAL_SUCCESS, filename);
}

// Handles a request to restore a file.
void RequestHandler::handleRestore()
{
    uint16_t nameLen;
    if (!readBytes(&nameLen, sizeof(nameLen)))
        return;

    std::string filename(nameLen, '\0');
    if (!readBytes(&filename[0], nameLen))
        return;

    fs::path filePath = BASE_BACKUP_PATH / std::to_string(m_userId) / filename;

    if (!fs::exists(filePath))
    {
        std::cerr << "File not found for restore: " << filePath << std::endl;
        sendFullHeaderResponse(StatusCode::ERROR_NO_FILE, filename);
        return;
    }

    std::ifstream inFile(filePath, std::ios::binary | std::ios::ate);
    if (!inFile)
    {
        sendFullHeaderResponse(StatusCode::ERROR_GENERAL, filename);
        return;
    }

    std::streamsize size = inFile.tellg();
    inFile.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    inFile.read(buffer.data(), size);

    std::cout << "Restoring file: " << filePath << " (" << size << " bytes)" << std::endl;
    sendFileResponse(StatusCode::RESTORE_SUCCESS, filename, buffer);
}

// Handles a request to delete a file.
void RequestHandler::handleDelete()
{
    uint16_t nameLen;
    if (!readBytes(&nameLen, sizeof(nameLen)))
        return;

    std::string filename(nameLen, '\0');
    if (!readBytes(&filename[0], nameLen))
        return;

    fs::path filePath = BASE_BACKUP_PATH / std::to_string(m_userId) / filename;

    if (!fs::exists(filePath))
    {
        std::cerr << "File not found for deletion: " << filePath << std::endl;
        // The protocol doesn't have a specific error for "delete failed because not found",
        // so we send a general success as per the spec for delete operation completion.
        // A client trying to restore it later would get ERROR_NO_FILE.
        sendFullHeaderResponse(StatusCode::GENERAL_SUCCESS, filename);
        return;
    }

    if (fs::remove(filePath))
    {
        std::cout << "Successfully deleted file: " << filePath << std::endl;
        sendFullHeaderResponse(StatusCode::GENERAL_SUCCESS, filename);
    }
    else
    {
        std::cerr << "Failed to delete file: " << filePath << std::endl;
        sendFullHeaderResponse(StatusCode::ERROR_GENERAL, filename);
    }
}

// Handles a request to list all files for the user.
void RequestHandler::handleListFiles()
{
    fs::path userDir = BASE_BACKUP_PATH / std::to_string(m_userId);
    std::string fileListStr;
    bool filesFound = false;

    if (fs::exists(userDir) && fs::is_directory(userDir))
    {
        for (const auto &entry : fs::directory_iterator(userDir))
        {
            if (fs::is_regular_file(entry.path()))
            {
                fileListStr += entry.path().filename().string() + "\n";
                filesFound = true;
            }
        }
    }

    if (!filesFound)
    {
        sendSimpleStatusResponse(StatusCode::ERROR_NO_FILES_FOR_CLIENT);
        return;
    }

    // Convert string to vector<char> for sending
    std::vector<char> fileListContent(fileListStr.begin(), fileListStr.end());
    // Generate a random name for this temporary list
    std::string randomFilename = generateRandomString(32);

    std::cout << "Sending file list for user " << m_userId << std::endl;
    sendFileResponse(StatusCode::LIST_SUCCESS, randomFilename, fileListContent);
}

// Sends a simple status response (for 1002, 1003).
void RequestHandler::sendSimpleStatusResponse(StatusCode status)
{
    ResponseHeader response;
    response.version = SERVER_VERSION;
    response.status = static_cast<uint16_t>(status);
    sendBytes(&response, sizeof(response));
}

// Sends a response with version, status, name_len, and filename (for 212, 1001).
void RequestHandler::sendFullHeaderResponse(StatusCode status, const std::string &filename)
{
    ResponseHeader header;
    header.version = SERVER_VERSION;
    header.status = static_cast<uint16_t>(status);

    uint16_t nameLen = filename.length();

    sendBytes(&header, sizeof(header));
    sendBytes(&nameLen, sizeof(nameLen));
    sendBytes(filename.c_str(), nameLen);
}
// Sends a complex response containing file data.
void RequestHandler::sendFileResponse(StatusCode status, const std::string &filename, const std::vector<char> &fileContent)
{
    ResponseHeader header;
    header.version = SERVER_VERSION;
    header.status = static_cast<uint16_t>(status);

    uint16_t nameLen = filename.length();
    uint32_t contentSize = fileContent.size();

    // Send all parts of the response sequentially.
    sendBytes(&header, sizeof(header));
    sendBytes(&nameLen, sizeof(nameLen));
    sendBytes(filename.c_str(), nameLen);
    sendBytes(&contentSize, sizeof(contentSize));
    sendBytes(fileContent.data(), contentSize);
}

// Helper to read a specific number of bytes from the socket.
bool RequestHandler::readBytes(void *buffer, size_t length)
{
    boost::system::error_code ec;
    boost::asio::read(m_socket, boost::asio::buffer(buffer, length), ec);
    if (ec)
    {
        return false; // Let the caller handle the error (including EOF)
    }
    print_hex("  < Received: ", buffer, length);
    return true;
}

// Helper to send a specific number of bytes to the socket.
bool RequestHandler::sendBytes(const void *buffer, size_t length)
{
    print_hex("  > Sent: ", buffer, length);
    boost::system::error_code ec;
    boost::asio::write(m_socket, boost::asio::buffer(buffer, length), ec);
    if (ec)
    {
        std::cerr << "Write error: " << ec.message() << std::endl;
        return false;
    }
    return true;
}

// Generates a random string for the LIST_FILES response filename.
std::string RequestHandler::generateRandomString(size_t length)
{
    const std::string chars =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);
    std::string random_string;
    for (size_t i = 0; i < length; ++i)
    {
        random_string += chars[distribution(generator)];
    }
    return random_string;
}
