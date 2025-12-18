# FTP Client-Server Application

## Mô tả

Dự án này là một ứng dụng FTP (File Transfer Protocol) client-server tuân thủ chuẩn RFC 959 với các chức năng:
- **Xác thực người dùng (USER/PASS)**: Đăng nhập với tài khoản trong file `account.txt`
- **Quản lý thư mục**: PWD, CWD, CDUP, MKD, RMD
- **Truyền file**: STOR (upload), RETR (download)
- **Liệt kê file**: LIST, NLST
- **Xóa file**: DELE, RMD
- **Passive mode (PASV)**: Hỗ trợ truyền dữ liệu qua passive connection
- **Compatible với FileZilla**: Server có thể làm việc với FileZilla client

## Cấu trúc dự án

```
.
├── FTP_Client/
│   └── ftp_client.c          # Mã nguồn FTP client
├── FTP_Server/
│   ├── ftp_server.c          # Mã nguồn chính FTP server
│   ├── ftp_server.h          # Header file
│   ├── ftp_commands.c        # Xử lý các FTP commands
│   ├── ftp_data.c            # Xử lý data connection
│   └── ftp_utils.c           # Các hàm tiện ích
├── utils/
│   ├── account.c/h           # Quản lý tài khoản
│   ├── logging.c/h           # Ghi log hoạt động
│   └── utils.c/h             # Các hàm tiện ích
├── build/
│   ├── ftp_server            # FTP server executable
│   └── ftp_client            # FTP client executable
├── logs/                     # Thư mục chứa log files
├── account.txt               # File chứa danh sách tài khoản
├── Makefile                  # File build project
└── README.md                 # File hướng dẫn này
```

## Cách sử dụng

### 1. Biên dịch

Sử dụng Makefile để build cả server và client:

```bash
make all      # Build cả server và client
make clean    # Xóa các file build
make rebuild  # Clean và build lại
make help     # Hiển thị hướng dẫn
```

### 2. Chuẩn bị file account.txt

Tạo file `account.txt` trong thư mục gốc của project với format:
```
username password root_dir status
```
Trong đó:
- `username`: Tên người dùng (không chứa khoảng trắng)
- `password`: Mật khẩu (không chứa khoảng trắng)
- `root_dir`: Thư mục gốc của user (đường dẫn tuyệt đối)
- `status`: 0 (banned) hoặc 1 (active)

Ví dụ:
```
admin 123456 /mnt/d/ftp-client/FTP_Server 1
user1 123456 /mnt/d/ftp-client/FTP_Client/user1 1
user2 123456 /mnt/d/ftp-client/FTP_Client 0
testuser 123456 /mnt/d/ftp-client/FTP_Client 1
```

### 3. Chạy FTP Server

```bash
make run-server
# hoặc
./build/ftp_server <port>
```

Ví dụ:
```bash
./build/ftp_server 2121
```

### 4. Chạy FTP Client

Mở terminal mới và chạy:

```bash
make run-client
# hoặc
./build/ftp_client 127.0.0.1 2121
```


### 5. Sử dụng với FileZilla Client

FTP Server cũng tương thích với FileZilla Client:

1. Mở FileZilla Client
2. Host: `127.0.0.1` hoặc `localhost`
3. Username: theo `account.txt` (ví dụ: `user1`)
4. Password: theo `account.txt` (ví dụ: `123456`)
5. Port: `2121`
6. Nhấn "Quickconnect"

### 6. Sử dụng FTP Client Menu

Sau khi kết nối thành công, client sẽ hiển thị menu:

```
=== FTP Client Menu ===
1. Login (USER/PASS)
2. Print Working Directory (PWD)
3. Change Working Directory (CWD)
4. List Files (LIST)
5. Download File (RETR)
6. Upload File (STOR)
7. Exit
```

#### Chức năng chi tiết:

**1. Login (USER/PASS)**
- Nhập username và password
- Server sẽ kiểm tra trong file `account.txt`
- Response codes:
  - `230 User logged in` - Đăng nhập thành công
  - `530 Not logged in` - Tài khoản bị banned hoặc không tồn tại
  - `530 Login incorrect` - Sai password

**2. Print Working Directory (PWD)**
- Hiển thị đường dẫn thư mục làm việc hiện tại (absolute path)
- **Yêu cầu đã đăng nhập**
- Response: `257 "/path/to/current/dir" is current directory`
- Ví dụ: `/mnt/d/ftp-client/FTP_Client/user1`

**3. Change Working Directory (CWD)**
- Thay đổi thư mục làm việc
- **Yêu cầu đã đăng nhập**
- Nhập đường dẫn tuyệt đối (ví dụ: `/mnt/d/ftp-client/utils`)
- Khi thay đổi thành công, root_dir của user trong `account.txt` cũng được cập nhật
- Response codes:
  - `250 File action okay, completed` - Thành công
  - `550 Directory not found` - Thư mục không tồn tại
  - `550 Permission denied` - Không có quyền truy cập

**4. List Files (LIST)**
- Liệt kê tất cả file và thư mục trong thư mục làm việc
- **Yêu cầu đã đăng nhập**
- Hiển thị thông tin chi tiết: permissions, owner, size, date, filename
- Sử dụng data connection (PASV mode)

**5. Download File (RETR)**
- Tải file từ server về client
- **Yêu cầu đã đăng nhập**
- Nhập tên file (relative to current directory)
- File sẽ được lưu vào thư mục hiện tại của client
- Response codes:
  - `150 File status okay` - Bắt đầu truyền
  - `226 Transfer complete` - Hoàn tất
  - `550 File unavailable` - File không tồn tại

**6. Upload File (STOR)**
- Upload file từ client lên server
- **Yêu cầu đã đăng nhập**
- Nhập đường dẫn file trên client
- File sẽ được lưu vào thư mục làm việc hiện tại trên server
- Response codes:
  - `150 File status okay` - Bắt đầu nhận
  - `226 Transfer complete` - Upload thành công
  - `550 Permission denied` - Không có quyền ghi

**7. Exit**
- Gửi lệnh QUIT và đóng kết nối

## FTP Protocol Implementation

### Các lệnh FTP được hỗ trợ

Server hỗ trợ các lệnh FTP chuẩn theo RFC 959:

#### Authentication Commands
- **USER <username>**: Gửi username để đăng nhập
- **PASS <password>**: Gửi password sau khi gửi USER

#### Directory Commands
- **PWD**: Hiển thị thư mục làm việc hiện tại (absolute path)
- **CWD <path>**: Thay đổi thư mục làm việc, cập nhật `account.txt`
- **CDUP**: Di chuyển lên thư mục cha
- **MKD <dirname>**: Tạo thư mục mới
- **RMD <dirname>**: Xóa thư mục

#### File Transfer Commands
- **PASV**: Chuyển sang passive mode để truyền dữ liệu
- **LIST [path]**: Liệt kê file và thư mục (chi tiết)
- **NLST [path]**: Liệt kê tên file đơn giản
- **RETR <filename>**: Download file từ server
- **STOR <filename>**: Upload file lên server
- **DELE <filename>**: Xóa file
- **SIZE <filename>**: Lấy kích thước file

#### Connection Commands
- **TYPE <type>**: Đặt kiểu truyền (I: binary, A: ASCII)
- **QUIT**: Đóng kết nối

### FTP Response Codes

Server trả về các response code chuẩn FTP theo RFC 959:

- **1xx - Positive Preliminary**: Đang xử lý
  - `150` - File status okay; about to open data connection
  
- **2xx - Positive Completion**: Thành công
  - `200` - Command okay
  - `220` - Service ready for new user
  - `221` - Service closing control connection
  - `226` - Closing data connection, transfer complete
  - `227` - Entering Passive Mode (h1,h2,h3,h4,p1,p2)
  - `230` - User logged in, proceed
  - `250` - Requested file action okay, completed
  - `257` - "PATHNAME" created

- **3xx - Positive Intermediate**: Cần thêm thông tin
  - `331` - User name okay, need password

- **4xx - Transient Negative**: Lỗi tạm thời
  - `425` - Can't open data connection
  - `426` - Connection closed; transfer aborted
  - `450` - Requested file action not taken

- **5xx - Permanent Negative**: Lỗi vĩnh viễn
  - `500` - Syntax error, command unrecognized
  - `501` - Syntax error in parameters or arguments
  - `502` - Command not implemented
  - `503` - Bad sequence of commands
  - `530` - Not logged in
  - `550` - Requested action not taken (file unavailable, permission denied, directory not found, etc.)

## Logging

Server ghi log tất cả các hoạt động vào file trong thư mục `logs/`. Log bao gồm:
- Thời gian
- Địa chỉ IP:Port của client
- Lệnh FTP được gửi
- Response code và kết quả

Format log: `[YYYY-MM-DD HH:MM:SS] <IP:Port> | Command: <FTP_COMMAND> | Response: <code>`

## Tính năng đặc biệt

1. **Multi-threaded Server**: Server sử dụng pthread để xử lý đồng thời nhiều client
2. **Passive Mode (PASV)**: Hỗ trợ data transfer qua passive connection
3. **Root Directory Management**: 
   - Mỗi user có `root_dir` riêng được định nghĩa trong `account.txt`
   - Khi CWD thành công, `root_dir` được cập nhật tự động trong `account.txt`
   - User không thể truy cập ngoài `root_dir` của mình (chroot jail)
4. **Binary và ASCII Mode**: Hỗ trợ cả TYPE I (binary) và TYPE A (ASCII)
5. **FileZilla Compatible**: Server tương thích với FileZilla Client và các FTP client khác

## Lưu ý quan trọng

1. File `account.txt` phải tồn tại với format: `username password root_dir status`
2. Thư mục `root_dir` của mỗi user phải tồn tại và có quyền truy cập
3. Thư mục `logs/` sẽ được tự động tạo để lưu log files
4. Server lắng nghe trên port được chỉ định (mặc định: 2121)
5. Tất cả lệnh FTP kết thúc bằng `\r\n` (CRLF)
6. Data connection sử dụng passive mode (PASV) để tránh firewall issues
7. **Bảo mật**: Server thực hiện path validation để ngăn directory traversal attacks

## Kiến trúc hệ thống

### FTP Server (FTP_Server/)

**Multi-threaded Architecture**:
- Server sử dụng pthread để xử lý đồng thời nhiều client connections
- Mỗi client được xử lý trong một thread riêng biệt
- Control connection và data connection được quản lý độc lập

**Cấu trúc module**:
- **ftp_server.c**: Main server, xử lý connections và authentication
- **ftp_commands.c**: Xử lý các FTP commands (PWD, CWD, CDUP, etc.)
- **ftp_data.c**: Xử lý data transfer (RETR, STOR, LIST)
- **ftp_utils.c**: Path validation, security checks
- **ftp_server.h**: Định nghĩa structures và constants

**Session Management**:
```c
typedef struct {
    int control_sock;           // Control connection socket
    int data_sock;              // Data connection socket
    char current_dir[1024];     // Current directory (relative to root)
    char root_dir[1024];        // User's root directory
    int logged_in;              // Authentication status
    char username[256];         // Logged in username
    int binary_mode;            // Transfer mode (1=binary, 0=ASCII)
    struct sockaddr_in addr;    // Client address
    int pasv_sock;              // Passive mode listening socket
} ftp_session_t;
```

### FTP Client (FTP_Client/)

**Chức năng chính**:
- Kết nối đến FTP server qua control connection
- Xử lý các lệnh FTP: USER, PASS, PWD, CWD, LIST, RETR, STOR, DELE
- Tự động thiết lập data connection qua PASV mode
- Interactive menu để người dùng dễ sử dụng

### Utils Module

**account.c/h**: Quản lý tài khoản
- `read_file_data()`: Đọc account.txt vào memory
- `update_user_root_dir()`: Cập nhật root_dir của user trong account.txt

**logging.c/h**: Logging system
- Ghi log tất cả các hoạt động vào `logs/` directory
- Format: `[TIMESTAMP] IP:Port | Command | Response`

**utils.c/h**: Utility functions
- `recv_line()`: Nhận dữ liệu đến khi gặp CRLF
- `send_all()`: Đảm bảo gửi hết dữ liệu
- Path manipulation và validation functions

### Makefile

**Targets chính**:
- **Compiler và Flags**:
  - `CC = gcc`: GCC compiler
  - `CFLAGS = -Wall -Wextra -g`: Enable warnings và debug symbols
  - `LDFLAGS = -lpthread`: Link với pthread library

- **Build Targets**:
  - `all`: Build cả FTP server và client
  - `run-server`: Build và chạy FTP server (port 2121)
  - `run-client`: Build và chạy FTP client
  - `run-both`: Chạy server (background) và client
  - `clean`: Xóa object files và executables
  - `rebuild`: Clean và build lại
  - `help`: Hiển thị trợ giúp

- **Source Files**:
  - **Server**: FTP_Server/*.c + utils/*.c
  - **Client**: FTP_Client/ftp_client.c
  - Object files được tạo trong cùng thư mục với source files

**Cách hoạt động**:
1. Khi chạy `make`:
   - Biên dịch tất cả `.c` thành `.o` files
   - Link server: `ftp_server.o + ftp_commands.o + ftp_data.o + ftp_utils.o + utils/*.o → build/ftp_server`
   - Link client: `ftp_client.o → build/ftp_client`
   - Tạo thư mục `build/` nếu chưa tồn tại

## Data Transfer Flow

### PASV Mode (Passive Mode)

Server sử dụng PASV mode cho data transfer:

```
Client                          Server
  |                               |
  |--- PASV ---------------------->|
  |                               | (create listening socket)
  |<--- 227 Entering Passive -----|
  |     Mode (h1,h2,h3,h4,p1,p2)  |
  |                               |
  | (connect to server's data port)|
  |                               |
  |--- LIST/RETR/STOR ----------->|
  |                               | (accept data connection)
  |<==== Data transfer =========>|
  |                               |
  |<--- 226 Transfer complete ----|
```

### File Transfer Example (RETR - Download)

```
Client                          Server
  |                               |
  |--- USER user1 --------------->|
  |<--- 331 Need password --------|
  |--- PASS 123456 -------------->|
  |<--- 230 Logged in ------------|
  |                               |
  |--- PASV ---------------------->|
  |<--- 227 Passive Mode ---------|
  |                               |
  |--- RETR file.txt ------------>|
  |                               | (check permissions, open file)
  |<--- 150 Opening connection ---|
  | (connect to data port)        |
  |<==== File data ==============>|
  |<--- 226 Transfer complete ----|
```

### File Upload Example (STOR)

```
Client                          Server
  |                               |
  |--- USER user1 --------------->|
  |<--- 331 Need password --------|
  |--- PASS 123456 -------------->|
  |<--- 230 Logged in ------------|
  |                               |
  |--- PASV ---------------------->|
  |<--- 227 Passive Mode ---------|
  |                               |
  |--- STOR newfile.txt --------->|
  |                               | (check permissions, create file)
  |<--- 150 Ready to receive -----|
  | (connect to data port)        |
  |===== File data ==============>|
  |<--- 226 Transfer complete ----|
```

## Bảo mật

1. **Path Validation**: Ngăn chặn directory traversal attacks (`../../../etc/passwd`)
2. **User Isolation**: Mỗi user chỉ truy cập được `root_dir` của mình
3. **Authentication**: Bắt buộc đăng nhập trước khi thực hiện operations
4. **Permission Checks**: Kiểm tra quyền truy cập file/directory trước khi thao tác

## Khác biệt so với FTP standard

1. **CWD behavior**: Khi CWD thành công, `root_dir` trong `account.txt` được cập nhật
2. **PWD output**: Hiển thị absolute path thay vì relative path
3. **Multi-user support**: Mỗi user có workspace riêng biệt



---

