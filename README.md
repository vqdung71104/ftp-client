# FTP Client-Server Application

## Mô tả

Dự án này là một ứng dụng client-server đơn giản với các chức năng:
- **Đăng nhập (LOGIN)**: Xác thực người dùng với tài khoản trong file `account.txt`
- **Đăng xuất (LOGOUT)**: Đăng xuất khỏi hệ thống
- **Đăng bài (POST)**: Đăng một tin nhắn lên server (yêu cầu đã đăng nhập)
- **Upload file (UPLD)**: Upload file từ client lên thư mục làm việc của user trên server
- **Download file (DNLD)**: Tải file từ thư mục làm việc của user trên server về client
- **Thay đổi thư mục làm việc (CHDIR)**: Thay đổi root directory của user
- **Liệt kê file (LIST)**: Hiển thị danh sách file trong thư mục làm việc của user

## Cấu trúc dự án

```
.
├── TCP_Client/
│   └── TCP_Client.c          # Mã nguồn client
├── TCP_Server/
│   └── TCP_Server.c          # Mã nguồn server
├── utils/
│   ├── account.c/h           # Quản lý tài khoản
│   ├── service.c/h           # Xử lý các service (login, post, upload)
│   ├── logging.c/h           # Ghi log hoạt động
│   └── utils.c/h             # Các hàm tiện ích
├── account.txt               # File chứa danh sách tài khoản
├── Makefile                  # File build project
└── README.md                 # File hướng dẫn này
```

## Cách sử dụng

### 1. Biên dịch

```bash
make clean
make
```

### 2. Chuẩn bị file account.txt

Tạo file `account.txt` trong thư mục gốc của project với format:
```
username status
```
Trong đó:
- `username`: Tên người dùng (không chứa khoảng trắng)
- `status`: 0 (banned) hoặc 1 (active)

Ví dụ:
```
admin 123456 /mnt/d/ftp-client/TCP_Server 1
user1 123456 /mnt/d/ftp-client/TCP_Client 1
user2 123456 /mnt/d/ftp-client/TCP_Client 0
banned_user 123456 /mnt/d/ftp-client/TCP_Client 0
testuser 123456 /mnt/d/ftp-client/TCP_Client 1

```

### 3. Chạy server

```bash
./build/server <port> <storage_directory>
```

Ví dụ:
```bash
./build/server 8080 build/storage
```

### 4. Chạy client

```bash
./build/client <server_ip> <port>
```

Ví dụ:
```bash
./build/client 127.0.0.1 8080
```

### 5. Sử dụng menu client

Sau khi kết nối thành công, client sẽ hiển thị menu:

```
--- Simple Menu ---
1. Login
2. Upload file
3. Download file
4. List files in working directory
5. Change working directory
6. Logout
7. Exit
```

#### Chức năng chi tiết:

**1. Login**
- Nhập username để đăng nhập
- Server sẽ kiểm tra trong file `account.txt`
- Phản hồi:
  - `110: Account exist and active` - Đăng nhập thành công
  - `212: Account does NOT exist` - Tài khoản không tồn tại
  - `211: Account IS banned` - Tài khoản bị khóa
  - `213: You have already logged in` - Đã đăng nhập rồi

p

**2. Upload file**
- Nhập đường dẫn file cần upload
- **Yêu cầu đã đăng nhập**
- File sẽ được lưu vào **thư mục làm việc (root_dir) của user** đã đăng nhập
- Ví dụ: Nếu đăng nhập là user1 với root_dir `/mnt/d/ftp-client/TCP_Client`, file sẽ được upload vào `/mnt/d/ftp-client/TCP_Client/`
- Phản hồi:
  - `+OK Please send file` - Server sẵn sàng nhận file
  - `OK Successful upload` - Upload thành công
  - `ERR Upload failed` - Upload thất bại
  - `221: You have NOT logged in` - Chưa đăng nhập

**3. Download file**
- Tải file từ thư mục làm việc của user trên server về thư mục TCP_Client
- **Yêu cầu đã đăng nhập**
- Nhập tên file cần tải (file phải tồn tại trong root_dir của user trên server)
- File sẽ được lưu vào thư mục hiện tại của client
- Phản hồi:
  - `+OK <filesize>` - Server gửi file với kích thước
  - `ERR: File not found` - File không tồn tại
  - `221: You have NOT logged in` - Chưa đăng nhập

  **4. List files in working directory**
- Liệt kê tất cả file và thư mục trong thư mục làm việc
- **Yêu cầu đã đăng nhập**
- Hiển thị:
  - `[DIR]` cho thư mục
  - `[FILE]` cho file kèm theo kích thước
- Phản hồi:
  - `150: Listing directory: <path>` - Bắt đầu liệt kê
  - `END` - Kết thúc danh sách
  - `ERR: Cannot open directory` - Lỗi mở thư mục
  - `221: You have NOT logged in` - Chưa đăng nhập

**5. Change working directory**
- Thay đổi thư mục làm việc
- **Yêu cầu đã đăng nhập**
- Hiển thị thư mục làm việc hiện tại
- Nếu muốn thay đổi thì ấn "y", nếu không ấn "n"
- Nhập đường dẫn thư mục làm việc mới
- Hệ thống cập nhật lại và sửa file account.txt
- Phản hồi:
  - `140: Directory changed successfully` - Thay đổi thành công
  - `240: Failed to update account file` - Lỗi cập nhật file
  - `221: You have NOT logged in` - Chưa đăng nhập

**6. Logout**
- Đăng xuất khỏi hệ thống
- Phản hồi:
  - `130: Logout successfully` - Đăng xuất thành công
  - `221: You have NOT logged in` - Chưa đăng nhập

**7. Exit**
- Thoát chương trình client

## Protocol

### Format lệnh

Tất cả các lệnh gửi lên server đều kết thúc bằng `\r\n`

1. **USER command**: `USER <username>\r\n`
2. **PASS command**: `PASS <password>\r\n`
3. **POST command**: `POST <message>\r\n`
4. **UPLD command**: `UPLD <filename> <filesize>\r\n`
5. **DNLD command**: `DNLD <filename>\r\n`
6. **CHDIR command**: `CHDIR <new_directory>\r\n`
7. **GETDIR command**: `GETDIR\r\n`
8. **LIST command**: `LIST\r\n`
9. **BYE command**: `BYE\r\n`

### Response codes

- `1xx`: Thành công
  - `110`: Login successful
  - `111`: Username OK, need password
  - `120`: Post successfully
  - `130`: Logout successfully
  - `140`: Directory changed successfully
  - `150`: Listing directory / Current directory
- `2xx`: Lỗi liên quan đến user
  - `211`: Account IS banned
  - `212`: Account does NOT exist
  - `213`: You have already logged in
  - `214`: Incorrect password
  - `220`: Please send username first
  - `221`: You have NOT logged in
  - `240`: Failed to update account file
- `3xx`: Lỗi command không hợp lệ
  - `300`: Unknown command
- `+OK`: Thành công (dùng cho file transfer)
  - `+OK Please send file` - Upload file
  - `+OK <filesize>` - Download file (theo sau là dữ liệu file)
  - `OK Successful upload` - Hoàn tất upload
- `ERR`: Lỗi (dùng cho file transfer)
  - `ERR Invalid command`
  - `ERR Invalid filesize`
  - `ERR Cannot create file`
  - `ERR Upload failed`
  - `ERR: File not found`
  - `ERR: Cannot open directory`
- `END`: Đánh dấu kết thúc danh sách file

## Logging

Server ghi log tất cả các hoạt động vào file trong thư mục `logs/`. Log bao gồm:
- Thời gian
- Địa chỉ IP:Port của client
- Lệnh được gửi
- Kết quả xử lý

Format log: `[YYYY-MM-DD HH:MM:SS] <IP:Port> | Request: <command> | Response: <result>`

## Lưu ý

1. Server chạy ở chế độ iterative (xử lý từng client một lần)
2. File `account.txt` phải tồn tại trước khi chạy server với format: `username password root_dir status`
3. Thư mục storage sẽ được tự động tạo nếu chưa tồn tại
4. Client sử dụng `recv_line()` để đọc response từ server (kết thúc bằng `\r\n`)
5. **File upload và download đều yêu cầu đã đăng nhập**
6. File được upload sẽ lưu vào `root_dir` của user đã đăng nhập
7. File download sẽ lấy từ `root_dir` của user đã đăng nhập
8. Mỗi user có `root_dir` riêng, được định nghĩa trong `account.txt`
9. Chức năng CHDIR cho phép thay đổi `root_dir` và cập nhật vào `account.txt`

## Mô tả chi tiết cách hoạt động

### Makefile

**Mục đích**: Tự động hóa quá trình biên dịch và liên kết project

**Cấu trúc**:
- **Compiler và Flags**:
  - `CC = gcc`: Sử dụng GCC compiler
  - `CFLAGS = -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200112L -Iutils`: Cờ biên dịch (C11 standard, hiển thị warnings, định nghĩa POSIX, include thư mục utils)
  - `LDFLAGS`: Cờ cho linker (hiện tại trống)

- **VPATH**: Chỉ định các thư mục chứa source files (`utils:TCP_Client:TCP_Server`)

- **Directories và Targets**:
  - `BUILD_DIR = build`: Thư mục chứa file output
  - `TARGET_CLIENT = $(BUILD_DIR)/client`: File thực thi client
  - `TARGET_SERVER = $(BUILD_DIR)/server`: File thực thi server

- **Source và Object Files**:
  - `UTILS_SRC`: Tất cả file `.c` trong thư mục `utils/` (sử dụng `wildcard`)
  - `CLIENT_SRC`: `TCP_Client/TCP_Client.c`
  - `SERVER_SRC`: `TCP_Server/TCP_Server.c`
  - Tạo danh sách object files (`.o`) tương ứng với mỗi source file

- **Quy tắc biên dịch**:
  1. `all`: Biên dịch cả client và server, tạo thư mục `build/storage` và `logs`
  2. Linking rules: Liên kết object files thành executable
     - Client: `$(CLIENT_OBJ) + $(UTILS_OBJS) → client`
     - Server: `$(SERVER_OBJ) + $(UTILS_OBJS) → server`
  3. Pattern rule `$(BUILD_DIR)/%.o: %.c`: Biên dịch mọi file `.c` thành `.o`
  4. `clean`: Xóa thư mục build
  5. `package`: Đóng gói source code thành file zip

**Cách hoạt động**:
1. Khi chạy `make`, nó sẽ:
   - Tìm tất cả file `.c` trong `utils/`
   - Biên dịch từng file `.c` thành `.o` và lưu vào `build/`
   - Liên kết các object files thành 2 executable: `build/client` và `build/server`
   - Tạo thư mục `build/storage` và `logs` nếu chưa tồn tại

### TCP_Client/TCP_Client.c

**Mục đích**: Chương trình client kết nối đến server để thực hiện các thao tác file transfer

**Cấu trúc chính**:

1. **main()**:
   - Nhận tham số: `<IP_Address> <Port_number>`
   - Tạo socket TCP
   - Kết nối đến server
   - Nhận welcome message
   - Gọi `simple_menu()` để hiển thị menu tương tác

2. **simple_menu()**:
   - Hiển thị menu với 8 tùy chọn
   - Đọc lựa chọn từ người dùng
   - Gọi hàm xử lý tương ứng
   - Lặp lại cho đến khi user chọn Exit (option 8)

3. **send_login_command()**:
   - Gửi `USER <username>` đến server
   - Nếu server trả về `111` (Username OK, need password):
     - Gửi `PASS <password>` đến server
   - Xử lý các response codes khác (212, 211, 213, 214)

4. **send_article_command()**:
   - Nhận nội dung tin nhắn từ user
   - Gửi `POST <message>` đến server
   - Nhận và hiển thị response

5. **send_upload_command()**:
   - Nhận đường dẫn file từ user
   - Mở file và lấy kích thước
   - Gửi `UPLD <filename> <filesize>` đến server
   - Chờ server response `+OK Please send file`
   - Đọc file theo từng chunk (16KB) và gửi qua socket
   - Nhận response cuối cùng (`OK Successful upload` hoặc `ERR Upload failed`)

6. **send_download_command()**:
   - Nhận tên file từ user
   - Gửi `DNLD <filename>` đến server
   - Nhận response `+OK <filesize>` từ server
   - Tạo file mới trong thư mục hiện tại
   - Nhận dữ liệu file theo từng chunk và ghi vào file
   - Kiểm tra tổng số byte nhận được có khớp với filesize không

7. **send_chdir_command()**:
   - Gửi `GETDIR` để lấy thư mục làm việc hiện tại
   - Hỏi user có muốn thay đổi không (y/n)
   - Nếu có: gửi `CHDIR <new_directory>` đến server
   - Nhận và hiển thị response

8. **send_list_command()**:
   - Gửi `LIST` đến server
   - Nhận danh sách file/thư mục từng dòng
   - Hiển thị cho đến khi nhận được marker `END`

9. **send_bye_command()**:
   - Gửi `BYE` đến server
   - Nhận response (130: Logout successfully hoặc 221: Not logged in)

**Các hàm tiện ích sử dụng** (từ `utils/`):
- `send_all()`: Đảm bảo gửi hết toàn bộ dữ liệu
- `recv_line()`: Nhận dữ liệu cho đến khi gặp `\r\n`
- `get_file_size()`: Lấy kích thước file

### TCP_Server/TCP_Server.c

**Mục đích**: Chương trình server lắng nghe kết nối từ client và xử lý các request

**Cấu trúc chính**:

1. **main()**:
   - Nhận tham số: `<Port_number> <Directory_name>`
   - Đọc file `account.txt` bằng `read_file_data()` (từ `utils/account.c`)
   - Tạo listening socket
   - Set `SO_REUSEADDR` để tránh lỗi "Address already in use"
   - Bind socket vào địa chỉ `INADDR_ANY:<port>`
   - Listen với backlog = 2
   - Vòng lặp chấp nhận kết nối:
     - `accept()` kết nối mới
     - Gọi `welcome_to_server()` để gửi welcome message
     - Gọi `handle_client()` để xử lý request
     - Đóng connection socket

2. **welcome_to_server()**:
   - Lấy IP và port của client từ `struct sockaddr_in`
   - Format thành string `<IP>:<Port>`
   - Gửi welcome message: `+OK Welcome to file server`
   - Log sự kiện connect

3. **handle_client()**:
   - Vòng lặp nhận và xử lý commands:
     - Gọi `recv_line()` để nhận command (kết thúc bằng `\r\n`)
     - Parse command thành 2 phần: `command` và `command_value`
     - Dùng `strchr()` để tìm khoảng trắng đầu tiên
   
   - **Xử lý các commands**:
     - `USER <username>`: Gọi `login()` từ `utils/service.c`
     - `PASS <password>`: Gọi `verify_password()` từ `utils/service.c`
     - `UPLD <filename> <filesize>`: Gọi `handle_upload_file()` từ `utils/service.c`
     - `DNLD <filename>`: Gọi `handle_download_file()` từ `utils/service.c`
     - `CHDIR <new_dir>`: Gọi `change_directory()` từ `utils/service.c`
     - `GETDIR`: Gọi `get_current_directory()` từ `utils/service.c`
     - `LIST`: Gọi `list_files()` từ `utils/service.c`
     - `BYE`: Xử lý logout (set `is_logged_in = 0`, clear user info)
     - Unknown command: Trả về `300: Unknown command`
   
   - Mỗi command được log vào file trong `logs/`

**Global variables** (từ `utils/account.h`):
- `users[]`: Mảng chứa thông tin tài khoản
- `count`: Số lượng tài khoản
- `is_logged_in`: Trạng thái đăng nhập (0/1)
- `current_username[]`: Username hiện tại
- `current_root_dir[]`: Thư mục làm việc của user hiện tại
- `pending_username[]`: Username đang chờ nhập password

### utils/service.c - Các hàm xử lý logic

1. **user_validation(username)**:
   - Duyệt mảng `users[]` để tìm username
   - Trả về: 1 (valid & active), 0 (not exist), -1 (banned)

2. **password_validation(username, password)**:
   - Tìm user và so sánh password
   - Trả về: 1 (correct), 0 (incorrect hoặc user not found)

3. **login(username, client_socket)**:
   - Kiểm tra nếu đã logged in → trả về 213
   - Gọi `user_validation()`
   - Nếu valid → trả về 111 (need password), lưu vào `pending_username`
   - Nếu not exist → trả về 212
   - Nếu banned → trả về 211

4. **verify_password(password, client_socket)**:
   - Kiểm tra nếu đã logged in → trả về 213
   - Kiểm tra nếu chưa gửi username → trả về 220
   - Gọi `password_validation()`
   - Nếu đúng → trả về 110, set `is_logged_in = 1`, lưu user info vào `current_username` và `current_root_dir`
   - Nếu sai → trả về 214, xóa `pending_username`

5. **handle_upload_file(conn_sock, client_addr_str, command_value, request_log)**:
   - Kiểm tra `is_logged_in` → nếu chưa: trả về 221
   - Parse `command_value` thành `filename` và `filesize`
   - Tạo đường dẫn: `<current_root_dir>/<filename>`
   - Mở file để ghi (`fopen(..., "wb")`)
   - Gửi `+OK Please send file`
   - Nhận dữ liệu file theo chunk (16KB) cho đến khi đủ `filesize` bytes
   - Ghi vào file
   - Trả về `OK Successful upload` hoặc `ERR Upload failed`
   - Log kết quả

6. **handle_download_file(conn_sock, client_addr_str, filename, request_log)**:
   - Kiểm tra `is_logged_in` → nếu chưa: trả về 221
   - Tạo đường dẫn: `<current_root_dir>/<filename>`
   - Mở file để đọc (`fopen(..., "rb")`)
   - Nếu không tồn tại → trả về `ERR: File not found`
   - Lấy kích thước file bằng `get_file_size()`
   - Gửi `+OK <filesize>`
   - Đọc file theo chunk (16KB) và gửi qua socket
   - Log kết quả

7. **get_current_directory(client_socket)**:
   - Kiểm tra `is_logged_in`
   - Trả về `150: Current directory: <current_root_dir>`

8. **change_directory(new_dir, client_socket)**:
   - Kiểm tra `is_logged_in`
   - Cập nhật `current_root_dir` và `users[i].root_dir`
   - Ghi lại toàn bộ mảng `users[]` vào file `account.txt`
   - Trả về 140 (success) hoặc 240 (failed)

9. **list_files(client_socket)**:
    - Kiểm tra `is_logged_in`
    - Mở thư mục `current_root_dir` bằng `opendir()`
    - Duyệt từng entry bằng `readdir()`
    - Bỏ qua `.` và `..`
    - Lấy thông tin file bằng `stat()`
    - Gửi từng dòng: `[DIR]` hoặc `[FILE] filename (size bytes)`
    - Gửi marker `END` để đánh dấu kết thúc

### Flow hoạt động tổng thể

**Upload file (Client → Server)**:
```
Client                          Server
  |                               |
  |--- UPLD file.txt 1024 ------->|
  |                               | (check login, parse command)
  |<---- +OK Please send file ----|
  |                               |
  |--- [file data chunk 1] ------>|
  |--- [file data chunk 2] ------>|
  |--- [file data chunk N] ------>|
  |                               | (save to current_root_dir/file.txt)
  |<---- OK Successful upload ----|
```

**Download file (Server → Client)**:
```
Client                          Server
  |                               |
  |--- DNLD file.txt ------------->|
  |                               | (check login, open file)
  |<---- +OK 1024 ----------------|
  |                               |
  |<--- [file data chunk 1] ------|
  |<--- [file data chunk 2] ------|
  |<--- [file data chunk N] ------|
  |                               |
  | (save to current directory)   |
```

**Session management**:
- Mỗi user có `root_dir` riêng định nghĩa trong `account.txt`
- Khi login thành công, `current_root_dir` được set bằng `root_dir` của user
- Mọi thao tác file (upload/download/list) đều thực hiện trong `current_root_dir`
- Có thể thay đổi `root_dir` bằng command CHDIR

## Tác giả

Vũ Quang Dũng

