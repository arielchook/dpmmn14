# Maman 14: Defensive Systems Programming - Backup Server & Client

This project contains a C++ backup server and a Python client that communicate over a custom TCP protocol to store, retrieve, list, and delete files.

## 1. Server Setup (C++)

The server is written in C++ and uses the **Boost.Asio** library for networking.

### Prerequisites

- **Visual Studio 2019 or later:** With the "Desktop development with C++" workload installed.
- **Boost Library:** You need to download and set up the Boost C++ libraries.
  1.  Go to the [Boost website](https://www.boost.org/users/download/) and download the latest version.
  2.  Extract the downloaded archive to a location on your computer, for example, `C:\local\boost_1_85_0`.
  3.  You don't need to build the libraries for this project, as we are only using the header-only parts of Boost.Asio.

### Configuring Visual Studio

1.  Create a new C++ Console App project in Visual Studio.
2.  Add the three server files (`main.cpp`, `RequestHandler.h`, `RequestHandler.cpp`) to the project.
3.  Configure the project to find the Boost headers:
    - Right-click on your project in the Solution Explorer and go to **Properties**.
    - Make sure the Configuration is set to **All Configurations** and Platform is **All Platforms**.
    - Navigate to **C/C++ -> General**.
    - In the **Additional Include Directories** field, add the path to your Boost directory (e.g., `C:\local\boost_1_85_0`).
4.  Enable C++17 or later:
    - Navigate to **C/C++ -> Language**.
    - Set **C++ Language Standard** to `ISO C++17 Standard (/std:c++17)` or newer.

### Running the Server

1.  Compile and run the project from within Visual Studio (press `F5`).
2.  A console window will appear, indicating that the server is running and listening for connections on port 1234.
3.  The server will create a `C:\backupsvr` directory if it doesn't exist. This is where all client files will be stored.

## 2. Client Setup (Python)

The client is a simple Python script.

### Prerequisites

- **Python 3.9 or later**.

### Configuration

Before running, make sure the `mmn14client` folder is set up correctly:

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
2.  Navigate to the `mmn14client` directory.
3.  Run the script using the command:
    ```sh
    python mmn14client.py
    ```
4.  The client will execute the predefined sequence of operations (list, backup, backup, list, restore, delete, restore) and print the server's response for each step. The restored file will be saved as `tmp` in the client directory.
