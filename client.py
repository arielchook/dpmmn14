import socket
import struct
import random
import os
from io import BytesIO

# Client protocol version
CLIENT_VERSION = 1

# DEBUG flag - if set to True it prints all sent and received bytes in hex (noisy)
DEBUG = False

CHUNK_SIZE = 4096

# Operation codes for requests
OP_BACKUP = 100
OP_RESTORE = 200
OP_DELETE = 201
OP_LIST_FILES = 202

# Status codes from server responses
STATUS_MAP = {
    210: "SUCCESS: File restored.",
    211: "SUCCESS: File list received.",
    212: "SUCCESS: Backup or delete operation successful.",
    1001: "ERROR: File not found on the server.",
    1002: "ERROR: No files found for this client on the server.",
    1003: "ERROR: General server error occurred."
}

class BackupClient:
    """
    A client to interact with the backup server.
    It handles packing requests and unpacking responses according to the protocol.
    """

    def __init__(self, server_info_file, backup_info_file):
        """Initializes the client by reading configuration files and generating a user ID."""
        self.server_ip, self.server_port = self._read_server_info(server_info_file)
        self.files_to_backup = self._read_backup_info(backup_info_file)
        # Generate a unique 4-byte random user ID
        self.user_id = random.randint(0, 0xFFFFFFFF)
        self.sock = None
        print(f"Client started. User ID: {self.user_id}")
        print(f"Will connect to server at {self.server_ip}:{self.server_port}")
        print(f"Files to process: {self.files_to_backup}")

    def _read_server_info(self, filename):
        """Reads server IP and port from the specified file."""
        with open(filename, 'r') as f:
            ip, port = f.read().strip().split(':')
            return ip, int(port)

    def _read_backup_info(self, filename):
        """Reads the list of filenames to back up."""
        with open(filename, 'r') as f:
            return [line.strip() for line in f if line.strip()]

    def connect(self):
        """Connects to the server."""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.server_ip, self.server_port))
            print("Successfully connected to the server.")
            return True
        except socket.error as e:
            print(f"Failed to connect to server: {e}")
            return False

    def close(self):
        """Closes the connection."""
        if self.sock:
            self.sock.close()
            self.sock = None
            print("Connection closed.")

    def _send_all(self, data):
        """Helper to ensure all data is sent over the socket."""
        if DEBUG:
            print(f"  < Sent: {data.hex()}")
        self.sock.sendall(data)

    def _recv_all(self, length):
        """Helper to ensure all expected data is received from the socket."""
        chunks = []
        bytes_recd = 0
        while bytes_recd < length:
            chunk = self.sock.recv(min(length - bytes_recd, CHUNK_SIZE))
            if not chunk:
                raise RuntimeError("Socket connection broken")
            chunks.append(chunk)
            bytes_recd += len(chunk)
        return b''.join(chunks)

    def _read_payload_in_chunks(self, size, file_handle):
        """Reads a payload of a given size in chunks and writes it to a file handle."""
        remaining_bytes = size
        while remaining_bytes > 0:
            chunk = self.sock.recv(min(remaining_bytes, CHUNK_SIZE))
            if not chunk:
                raise RuntimeError("Socket connection broken")
            file_handle.write(chunk)
            remaining_bytes -= len(chunk)

    def _read_response(self):
        """Reads and parses a response from the server."""
        # Read the fixed-size part of the header
        header_part1 = self._recv_all(3)  # version (1) + status (2)
        if DEBUG:
            print(f"  > Received: {header_part1.hex()}")
        version, status = struct.unpack('<BH', header_part1)

        print(f"  > Received response: Version={version}, Status={status} ({STATUS_MAP.get(status, 'Unknown')})")

        # Case 1: Only version and status are sent (1002, 1003)
        if status in [1002, 1003]:
            return status, None, 0

        # Case 2: Full header (version, status, name_len, filename) is sent (212, 1001)
        # and also for 210, 211 which have full header + body
        name_len_data = self._recv_all(2)
        if DEBUG:
            print(f"  > Received: {name_len_data.hex()}")
        name_len = struct.unpack('<H', name_len_data)[0]

        filename_bytes = self._recv_all(name_len)
        if DEBUG:
            print(f"  > Received: {filename_bytes.hex()}")
        filename = filename_bytes.decode('ascii')
        print(f"  > Response Filename: {filename}")

        # Case 3: Full header + body (size, payload) is sent (210, 211)
        if status in [210, 211]:
            size_data = self._recv_all(4)
            if DEBUG:
                print(f"  > Received: {size_data.hex()}")
            size = struct.unpack('<I', size_data)[0]
            print(f"  > Response Payload Size: {size} bytes")
            return status, filename, size
        else: # This covers 212, 1001 where only full header is sent
            return status, filename, 0


    def request_list_files(self):
        """Sends a request to list files on the server."""
        print("\n[Action] Requesting file list...")
        # Header: user_id, version, op
        header = struct.pack('<IBB', self.user_id, CLIENT_VERSION, OP_LIST_FILES)
        self._send_all(header)
        status, _, payload_size = self._read_response()
        if payload_size > 0:
            payload_buffer = BytesIO()
            self._read_payload_in_chunks(payload_size, payload_buffer)
            payload = payload_buffer.getvalue()
            print("--- Server File List ---")
            print(payload.decode('ascii').strip())
            print("------------------------")
        return status

    def request_backup_file(self, filename):
        """Sends a request to back up a file."""
        print(f"\n[Action] Backing up file: '{filename}'...")
        if not os.path.exists(filename):
            print(f"  > Error: File '{filename}' not found locally.")
            return None

        file_size = os.path.getsize(filename)
        filename_bytes = filename.encode('ascii')
        
        # Header: user_id, version, op
        header = struct.pack('<IBB', self.user_id, CLIENT_VERSION, OP_BACKUP)
        # File info: name_len, filename, size
        file_info = struct.pack(f'<H{len(filename_bytes)}sI', len(filename_bytes), filename_bytes, file_size)
        
        self._send_all(header + file_info)

        with open(filename, 'rb') as f:
            while True:
                chunk = f.read(CHUNK_SIZE)
                if not chunk:
                    break
                self._send_all(chunk)
        
        status, _, _ = self._read_response()
        return status
        
    def request_restore_file(self, filename):
        """Sends a request to restore a file."""
        print(f"\n[Action] Restoring file: '{filename}'...")
        filename_bytes = filename.encode('ascii')
        
        # Header: user_id, version, op
        header = struct.pack('<IBB', self.user_id, CLIENT_VERSION, OP_RESTORE)
        # File info: name_len, filename
        file_info = struct.pack(f'<H{len(filename_bytes)}s', len(filename_bytes), filename_bytes)
        
        self._send_all(header + file_info)
        status, response_filename, payload_size = self._read_response()
        
        # Note: seems there's a conflict in the spec about the filename in the response to RESTORE.
        # page 3 says it should save the file in the same folder with the name "tmp". This doesn't make sense as it only allows to restore one file
        # at a time, and it doesn't restore the file to its original name (assuming the file is gone from local machine, hence I need to restore it).
        # Then, on page 5 the protocol spec for RESTORE says the response includes the filename.
        # I'll assume the latter is correct and the server sends back the original filename.
        if status == 210 and payload_size > 0:
            # Sanitize the filename from the server to prevent path traversal.
            # This strips all directory parts (e.g., "../../") and keeps only the filename.
            save_as = os.path.basename(response_filename)
            
            # As an extra precaution, check if the resulting filename is empty.
            if not save_as:
                print("  > ERROR: Server sent an invalid or empty filename for saving.")
                return status

            with open(save_as, 'wb') as f:
                self._read_payload_in_chunks(payload_size, f)
            print(f"  > File successfully restored and saved as '{save_as}'")
        return status

    def request_delete_file(self, filename):
        """Sends a request to delete a file."""
        print(f"\n[Action] Deleting file: '{filename}'...")
        filename_bytes = filename.encode('ascii')

        # Header: user_id, version, op
        header = struct.pack('<IBB', self.user_id, CLIENT_VERSION, OP_DELETE)
        # File info: name_len, filename
        file_info = struct.pack(f'<H{len(filename_bytes)}s', len(filename_bytes), filename_bytes)
        
        self._send_all(header + file_info)
        status, _, _ = self._read_response()
        return status

def main():
    """
    Main function to run the client's operational flow.
    """
    client = BackupClient('server.info', 'backup.info')
    
    if not client.connect():
        return

    try:
        # Step 4: Request list of existing files (should be empty or non-existent)
        client.request_list_files()

        # Step 5: Send the first file for backup
        if len(client.files_to_backup) > 0:
            client.request_backup_file(client.files_to_backup[0])
        else:
            print("No files listed in backup.info to back up.")
        
        # Step 6: Send the second file for backup
        if len(client.files_to_backup) > 1:
            client.request_backup_file(client.files_to_backup[1])
        else:
            print("No second file listed in backup.info to back up.")

        # Step 7: Request file list again (should now show the backed-up files)
        client.request_list_files()

        # Step 8: Request to restore the first file
        if len(client.files_to_backup) > 0:
            client.request_restore_file(client.files_to_backup[0])

        # Step 9: Request to delete the first file
        if len(client.files_to_backup) > 0:
            client.request_delete_file(client.files_to_backup[0])
        
        # Step 10: Request to restore the first file again (should fail)
        if len(client.files_to_backup) > 0:
            client.request_restore_file(client.files_to_backup[0])

    except Exception as e:
        print(f"\nAn unexpected error occurred: {e}")
    finally:
        # Step 11: Close connection
        client.close()


if __name__ == "__main__":
    main()