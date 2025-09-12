# Maman 14: Defensive Programming - Backup Server & Client

This project contains a C++ backup server and a Python client that communicate over a custom TCP protocol to store, retrieve, list, and delete files.

## 1. Server Setup (C++)

The server is written in C++ and uses the **Boost.Asio** library for networking. It is built using a `Makefile` with Microsoft's C++ compiler (`cl.exe`).

### Prerequisites

- **Microsoft C++ Build Tools:** You must install the build tools which include the C++ compiler and `nmake`. You can download them from the [Visual Studio website](https://visualstudio.microsoft.com/visual-cpp-build-tools/).

### Dependencies

- **Boost:** The project has a dependency on the Boost C++ libraries. However, a subset of the required header files (from Boost version 1.89.0) is already included in the `boost/` directory of this repository. This means you **do not** need to download or install Boost separately.

### Building the Server

The server can be built from the command line using `nmake`.

1.  **Set Up Environment:** Before building, you need to set up the Visual Studio environment variables. The `Makefile` is configured to do this automatically by calling `vcvarsall.bat`.

    - Ensure the environment variable `VCINSTALLDIR` is set to the root of your Visual Studio installation's VC directory (e.g., `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC`).
    - The `Makefile` uses this variable to find `vcvarsall.bat`. If your path is different, you may need to adjust the `build` target in the `Makefile`.

2.  **Build Command:** Open a command prompt, navigate to the project's root directory, and run:
    ```cmd
    nmake build
    ```
    This command will first set up the environment and then compile the source files, creating `server.exe`.

### Running the Server

- The server executable is `server.exe`.
- It accepts an optional command-line argument for the port number.
- If no port is provided, it defaults to `1234`.

**Examples:**

- Run on the default port:
  ```cmd
  server.exe
  ```
- Run on a specific port (e.g., 8080):
  ```cmd
  server.exe 8080
  ```
- The server will create a `C:\backupsvr` directory if it doesn't exist. This is where all client files will be stored.

## 2. Client Setup (Python)

The client is a simple Python script.

### Prerequisites

- **Python 3.9 or later**.

### Configuration

Before running, make sure the client directory is set up correctly:

1.  **`server.info`**: This file must contain the server's address and port. For a server running on the same machine, the content should be:
    ```
    127.0.0.1:1234
    ```
2.  **`backup.info`**: This file lists the files to be backed up, one per line. The files must exist in the same directory as the client script.
    ```
    file1.txt
    file2.txt
    ```
3.  **Create Sample Files**: Create two text files named `file1.txt` and `file2.txt` in the client directory.
    - `file1.txt`: "This is the first file for backup."
    - `file2.txt`: "This is the second file, it is a bit different."

### Running the Client

1.  Open a terminal or command prompt.
2.  Navigate to the project directory.
3.  Run the script using the command:
    ```cmd
    python client.py
    ```
4.  The client will execute the predefined sequence of operations (list, backup, backup, list, restore, delete, restore) and print the server's response for each step. The restored file will be saved as `tmp` in the client directory.

## Potential Security Issues

### Attacking The Server

#### Connection Flooding

The server is designed to create a new thread for every incoming connection. An attacker could open thousands of TCP connections to the server and keep them open without sending any data. Each connection and thread consumes server memory and operating system resources, which could eventually prevent legitimate users from connecting.

#### Disk Space Exhaustion

An attacker can use the backup functionality as intended but do so maliciously. By creating many different client instances (each with a unique random user ID) and uploading very large files, an attacker could fill the server's hard drive. Once the disk is full, no one else can back up their files.

#### Path Traversal

When handling requests for file operations (backup, restore, delete), the server combines a base directory path with the filename provided by the client. An attacker could provide a malicious filename like

../../../../../boot.ini. If the server's code is not secure, this could allow the attacker to:
Delete arbitrary files on the server (e.g., deleting important system files).
Overwrite arbitrary files by backing up a malicious file to a sensitive location.
Retrieve arbitrary files by requesting to restore a file from outside the designated backup directory.
I've addressed this in RequestHandler::getValidatedFilePath .

### Attacking The Client

#### Disk Space Exhaustion

An attacker can have their server use the backup functionality as intended but do so maliciously. It can return extremely large files that will be saved by the client to their hard drive, exhausting disk space.

#### Malicious Content

An attacker may run their server with the normal backup functionality. It can then contaminate each executable in users folder with a virus/trojan. Once restored, the user's machine will be infected or exposed.

#### Path Traversal

Same idea as in the server. A malicious server may send a relative file path in a response to RESTORE, which in turn may overwrite a system file on the client's machine.
I've addressed this in BaclupClient.request_restore_file() .
