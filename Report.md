# Giao thức FTP Client-Server

## Thông tin đề tài

**Tên đề tài**: FTP Client-Server Application  
**Giao thức**: File Transfer Protocol (FTP) - RFC 959  
**Ngôn ngữ lập trình**: C  
**Kiến trúc**: Client-Server model với multi-threading  

---

## 1. Tổng quan giao thức FTP

### 1.1. Giới thiệu

FTP (File Transfer Protocol) là một giao thức mạng chuẩn được sử dụng để truyền tải file giữa client và server trên mạng TCP/IP. FTP được định nghĩa trong RFC 959 và hoạt động theo mô hình client-server.

### 1.2. Đặc điểm chính

- **Giao thức tầng ứng dụng**: FTP hoạt động ở tầng Application trong mô hình OSI
- **Dựa trên TCP**: FTP sử dụng TCP để đảm bảo truyền dữ liệu tin cậy
- **Hai kết nối độc lập**:
  - **Control Connection**: Để gửi lệnh và nhận response (persistent)
  - **Data Connection**: Để truyền dữ liệu file, directory listing (temporary)
- **Hai chế độ truyền dữ liệu**:
  - **Active Mode**: Server kết nối đến client
  - **Passive Mode (PASV)**: Client kết nối đến server (được sử dụng trong dự án này)

---

## 2. Kiến trúc và cổng kết nối

### 2.1. Cổng mặc định

| Thành phần | Cổng | Mô tả |
|------------|------|-------|
| FTP Server - Control | 2121 | Lắng nghe các kết nối control từ client |
| FTP Server - Data (PASV) | Dynamic (>1024) | Được cấp phát động khi client yêu cầu PASV |
| FTP Client - Control | Ephemeral (>1024) | Được hệ điều hành cấp phát tự động |
| FTP Client - Data | Ephemeral (>1024) | Được hệ điều hành cấp phát tự động |

**Lưu ý**: Port 2121 được sử dụng thay vì port 21 chuẩn để tránh xung đột với FTP server hệ thống và không cần quyền root.

### 2.2. Mô hình kết nối

```
┌─────────────┐                           ┌─────────────┐
│             │   Control Connection      │             │
│   Client    │◄─────────────────────────►│   Server    │
│             │    Port 2121 (TCP)        │             │
│             │                            │             │
│             │   Data Connection (PASV)  │             │
│             │◄─────────────────────────►│             │
│             │    Dynamic Port (TCP)     │             │
└─────────────┘                           └─────────────┘
```

---

## 3. Cấu trúc gói tin

### 3.1. Control Connection Messages

#### Format lệnh từ Client

```
<COMMAND> [<PARAMETER>]\r\n
```

**Ví dụ**:
```
USER admin\r\n
PASS 123456\r\n
CWD /home/user\r\n
RETR file.txt\r\n
```

**Đặc điểm**:
- Lệnh viết HOA (uppercase)
- Kết thúc bằng CRLF (`\r\n`)
- Parameter tùy chọn, phân cách bởi dấu cách
- Độ dài tối đa: 512 bytes (theo RFC 959)

#### Format Response từ Server

```
<CODE> <MESSAGE>\r\n
```

**Ví dụ**:
```
220 FTP Server Ready.\r\n
331 Username okay, need password.\r\n
230 User logged in.\r\n
150 File status okay; about to open data connection.\r\n
226 Transfer complete.\r\n
550 File unavailable.\r\n
```

**Cấu trúc Response Code**:
- **3 chữ số**: Mã trạng thái
  - Chữ số đầu (1-5): Loại response
    - `1xx`: Positive Preliminary (đang xử lý)
    - `2xx`: Positive Completion (thành công)
    - `3xx`: Positive Intermediate (cần thêm thông tin)
    - `4xx`: Transient Negative (lỗi tạm thời)
    - `5xx`: Permanent Negative (lỗi vĩnh viễn)
- **Sau code**: Thông điệp mô tả
- **Kết thúc**: CRLF (`\r\n`)

### 3.2. Data Connection Transfer

Data connection truyền **raw binary data** hoặc **ASCII text** không có header đặc biệt:

```
┌────────────────────────────────────────┐
│         File Content / Data            │
│         (Raw bytes stream)             │
└────────────────────────────────────────┘
```

**Đặc điểm**:
- Không có header hay framing đặc biệt
- Data được truyền dưới dạng byte stream liên tục
- Kích thước file được biết trước qua lệnh `SIZE` hoặc được xác định khi đóng connection
- Transfer mode (binary/ASCII) được set bởi lệnh `TYPE`

---

## 4. Quy trình kết nối và trao đổi dữ liệu

### 4.1. Sequence Diagram - Kết nối và Authentication

```
Client                                          Server
  │                                               │
  │─────────── TCP Connect (Port 2121) ─────────►│
  │                                               │
  │◄──────── 220 FTP Server Ready ───────────────│
  │                                               │
  │─────────── USER admin ──────────────────────►│
  │                                               │
  │◄──────── 331 Username okay, need password ───│
  │                                               │
  │─────────── PASS 123456 ─────────────────────►│
  │                                               │
  │◄──────── 230 User logged in ─────────────────│
  │                                               │
```

### 4.2. Sequence Diagram - PASV Mode Setup

```
Client                                          Server
  │                                               │
  │─────────── PASV ────────────────────────────►│
  │                                               │
  │                                           (create data
  │                                            listening socket,
  │                                            bind to dynamic port)
  │                                               │
  │◄─── 227 Entering Passive Mode ───────────────│
  │     (h1,h2,h3,h4,p1,p2)                      │
  │                                               │
  │  Parse IP: h1.h2.h3.h4                       │
  │  Parse Port: p1*256 + p2                     │
  │                                               │
```

**Giải thích PASV Response**:
```
227 Entering Passive Mode (127,0,0,1,234,56)
```
- IP Address: `127.0.0.1`
- Port Number: `234*256 + 56 = 59960`
- Client sẽ kết nối đến `127.0.0.1:59960` cho data connection

### 4.3. Sequence Diagram - Download File (RETR)

```
Client                                          Server
  │                                               │
  │─────────── PASV ────────────────────────────►│
  │◄─── 227 Entering Passive Mode ───────────────│
  │         (h1,h2,h3,h4,p1,p2)                  │
  │                                               │
  │──── TCP Connect to Data Port ───────────────►│
  │                                               │
  │─────────── RETR file.txt ───────────────────►│
  │                                               │
  │                                          (open file,
  │                                           check permissions)
  │                                               │
  │◄─── 150 File status okay ────────────────────│
  │                                               │
  │                                          (accept data
  │                                           connection)
  │                                               │
  │◄═══════ File Data Stream ════════════════════│
  │         (binary data)                        │
  │                                               │
  │         (data connection closed)             │
  │                                               │
  │◄─── 226 Transfer complete ───────────────────│
  │                                               │
```

### 4.4. Sequence Diagram - Upload File (STOR)

```
Client                                          Server
  │                                               │
  │─────────── PASV ────────────────────────────►│
  │◄─── 227 Entering Passive Mode ───────────────│
  │         (h1,h2,h3,h4,p1,p2)                  │
  │                                               │
  │──── TCP Connect to Data Port ───────────────►│
  │                                               │
  │─────────── STOR newfile.txt ────────────────►│
  │                                               │
  │                                          (create file,
  │                                           check permissions)
  │                                               │
  │◄─── 150 Ready to receive ────────────────────│
  │                                               │
  │                                          (accept data
  │                                           connection)
  │                                               │
  │═══════ File Data Stream ═════════════════════►│
  │         (binary data)                        │
  │                                               │
  │         (close data connection)              │
  │                                               │
  │◄─── 226 Transfer complete ───────────────────│
  │                                               │
```

### 4.5. Sequence Diagram - List Directory (LIST)

```
Client                                          Server
  │                                               │
  │─────────── PASV ────────────────────────────►│
  │◄─── 227 Entering Passive Mode ───────────────│
  │                                               │
  │──── TCP Connect to Data Port ───────────────►│
  │                                               │
  │─────────── LIST ─────────────────────────────►│
  │                                               │
  │                                          (read directory)
  │                                               │
  │◄─── 150 Opening ASCII mode data connection ──│
  │                                               │
  │◄═══════ Directory Listing ═══════════════════│
  │    -rw-r--r-- 1 user group 1024 Dec 16 file1│
  │    drwxr-xr-x 2 user group 4096 Dec 16 dir1 │
  │                                               │
  │         (data connection closed)             │
  │                                               │
  │◄─── 226 Directory send OK ───────────────────│
  │                                               │
```

### 4.6. Sequence Diagram - Change Directory (CWD)

```
Client                                          Server
  │                                               │
  │─────────── CWD /home/user/docs ─────────────►│
  │                                               │
  │                                          (validate path,
  │                                           check permissions,
  │                                           update session state,
  │                                           update account.txt)
  │                                               │
  │◄─── 250 Directory successfully changed ──────│
  │                                               │
```

**Lưu ý đặc biệt**: Trong implementation này, khi CWD thành công, server tự động cập nhật `root_dir` của user trong file `account.txt`.

---

## 5. Chi tiết các lệnh FTP được hỗ trợ

### 5.1. Authentication Commands

#### USER - Gửi username

**Request**:
```
USER <username>\r\n
```

**Response**:
- `331 Username okay, need password.` - Username hợp lệ, cần password
- `530 Not logged in.` - Username không tồn tại hoặc bị banned

**Ví dụ**:
```
C: USER admin\r\n
S: 331 Username okay, need password.\r\n
```

#### PASS - Gửi password

**Request**:
```
PASS <password>\r\n
```

**Response**:
- `230 User logged in, proceed.` - Đăng nhập thành công
- `530 Login incorrect.` - Password sai

**Ví dụ**:
```
C: PASS 123456\r\n
S: 230 User logged in, proceed.\r\n
```

### 5.2. Directory Commands

#### PWD - Print Working Directory

**Request**:
```
PWD\r\n
```

**Response**:
- `257 "<path>" is current directory.` - Trả về đường dẫn tuyệt đối

**Ví dụ**:
```
C: PWD\r\n
S: 257 "/mnt/d/ftp-client/FTP_Client/user1" is current directory.\r\n
```

#### CWD - Change Working Directory

**Request**:
```
CWD <path>\r\n
```

**Response**:
- `250 Directory successfully changed.` - Thành công
- `550 Directory not found.` - Thư mục không tồn tại
- `550 Permission denied.` - Không có quyền truy cập

**Ví dụ**:
```
C: CWD /mnt/d/ftp-client/utils\r\n
S: 250 Directory successfully changed.\r\n
```

#### CDUP - Change to Parent Directory

**Request**:
```
CDUP\r\n
```

**Response**:
- `250 Directory successfully changed.` - Thành công
- `550 Permission denied.` - Không thể lên thư mục cha (đã ở root)

**Ví dụ**:
```
C: CDUP\r\n
S: 250 Directory successfully changed.\r\n
```

#### MKD - Make Directory

**Request**:
```
MKD <dirname>\r\n
```

**Response**:
- `257 "<path>" created.` - Tạo thành công
- `550 Permission denied.` - Không có quyền tạo

**Ví dụ**:
```
C: MKD newfolder\r\n
S: 257 "/mnt/d/ftp-client/utils/newfolder" created.\r\n
```

#### RMD - Remove Directory

**Request**:
```
RMD <dirname>\r\n
```

**Response**:
- `250 Directory removed.` - Xóa thành công
- `550 Directory not empty or not found.` - Thất bại

**Ví dụ**:
```
C: RMD olddir\r\n
S: 250 Directory removed.\r\n
```

### 5.3. File Transfer Commands

#### PASV - Enter Passive Mode

**Request**:
```
PASV\r\n
```

**Response**:
```
227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).\r\n
```

**Ví dụ**:
```
C: PASV\r\n
S: 227 Entering Passive Mode (127,0,0,1,195,123).\r\n
```

Calculation:
- IP: `127.0.0.1`
- Port: `195*256 + 123 = 50043`

#### TYPE - Set Transfer Type

**Request**:
```
TYPE <type>\r\n
```
- `TYPE I` - Binary mode (8-bit)
- `TYPE A` - ASCII mode (7-bit)

**Response**:
- `200 Type set to <type>.` - Thành công

**Ví dụ**:
```
C: TYPE I\r\n
S: 200 Type set to I.\r\n
```

#### SIZE - Get File Size

**Request**:
```
SIZE <filename>\r\n
```

**Response**:
- `213 <size>` - Kích thước file (bytes)
- `550 File not found.` - File không tồn tại

**Ví dụ**:
```
C: SIZE document.pdf\r\n
S: 213 1048576\r\n
```

#### RETR - Retrieve (Download) File

**Request**:
```
RETR <filename>\r\n
```

**Response**:
- `150 Opening BINARY mode data connection for <filename>.` - Bắt đầu
- `226 Transfer complete.` - Hoàn tất
- `550 File unavailable.` - File không tồn tại

**Ví dụ**:
```
C: PASV\r\n
S: 227 Entering Passive Mode (127,0,0,1,200,50).\r\n
C: (connects to 127.0.0.1:51250)
C: RETR report.pdf\r\n
S: 150 Opening BINARY mode data connection for report.pdf (2048 bytes).\r\n
S: (sends file data over data connection)
S: 226 Transfer complete.\r\n
```

#### STOR - Store (Upload) File

**Request**:
```
STOR <filename>\r\n
```

**Response**:
- `150 Ok to send data.` - Sẵn sàng nhận
- `226 Transfer complete.` - Hoàn tất
- `550 Permission denied.` - Không có quyền ghi

**Ví dụ**:
```
C: PASV\r\n
S: 227 Entering Passive Mode (127,0,0,1,200,100).\r\n
C: (connects to 127.0.0.1:51300)
C: STOR newfile.txt\r\n
S: 150 Ok to send data.\r\n
C: (sends file data over data connection)
S: 226 Transfer complete.\r\n
```

#### LIST - List Directory

**Request**:
```
LIST [<path>]\r\n
```

**Response**:
- `150 Here comes the directory listing.` - Bắt đầu
- `226 Directory send OK.` - Hoàn tất

**Format dữ liệu (Unix-style)**:
```
-rw-r--r--   1 user  group      1024 Dec 16 10:30 file.txt
drwxr-xr-x   2 user  group      4096 Dec 15 15:20 docs
```

**Ví dụ**:
```
C: PASV\r\n
S: 227 Entering Passive Mode (127,0,0,1,201,50).\r\n
C: LIST\r\n
S: 150 Here comes the directory listing.\r\n
S: (sends listing over data connection)
S: 226 Directory send OK.\r\n
```

#### NLST - Name List

**Request**:
```
NLST [<path>]\r\n
```

**Response**: Tương tự LIST nhưng chỉ trả về tên file

**Format dữ liệu**:
```
file1.txt
file2.pdf
directory1
```

#### DELE - Delete File

**Request**:
```
DELE <filename>\r\n
```

**Response**:
- `250 File deleted.` - Xóa thành công
- `550 File not found.` - File không tồn tại

**Ví dụ**:
```
C: DELE oldfile.txt\r\n
S: 250 File deleted.\r\n
```

### 5.4. Connection Commands

#### QUIT - Logout and Disconnect

**Request**:
```
QUIT\r\n
```

**Response**:
- `221 Goodbye.` - Đóng kết nối

**Ví dụ**:
```
C: QUIT\r\n
S: 221 Goodbye.\r\n
(connection closed)
```

---

## 6. Cơ chế ACK và xử lý lỗi

### 6.1. TCP Level ACK

FTP dựa trên TCP, do đó:
- **Mỗi gói tin TCP** được ACK tự động bởi TCP protocol
- **Reliable transmission**: TCP đảm bảo dữ liệu được truyền đúng thứ tự và không bị mất
- **Flow control**: TCP window size điều chỉnh tốc độ truyền
- **Congestion control**: TCP tự động phát hiện và xử lý nghẽn mạng

### 6.2. Application Level Response

Mỗi lệnh FTP từ client nhận được **một response code** từ server:

```
Client                Server
  │                     │
  │─── USER admin ─────►│
  │                     │ (process request)
  │                     │
  │◄─── 331 OK ─────────│  ← Application level ACK
  │                     │
```

**Đặc điểm**:
- **Synchronous**: Client gửi lệnh, chờ response, rồi tiếp tục
- **One command at a time**: Không gửi multiple commands đồng thời trên control connection
- **Response code** cho biết kết quả: thành công, lỗi, hoặc cần thêm thông tin

### 6.3. Xử lý lỗi

#### Network Errors

```
Client                           Server
  │                                │
  │─── RETR file.txt ─────────────►│
  │                                │
  │◄─── 150 Opening ───────────────│
  │                                │
  │ (TCP connection lost)         X
  │                                │
  │ (timeout, retry or           │
  │  report error to user)       │
```

**Xử lý**:
- Client detect bằng `recv()` return 0 hoặc -1
- Timeout mechanism
- Retry logic (nếu có)

#### File Not Found

```
C: RETR nonexistent.txt\r\n
S: 550 File unavailable.\r\n
```

#### Permission Denied

```
C: CWD /root\r\n
S: 550 Permission denied.\r\n
```

#### Invalid Command

```
C: INVALID\r\n
S: 500 Syntax error, command unrecognized.\r\n
```

---

## 7. Đóng gói và truyền dữ liệu

### 7.1. Control Connection

**Đóng gói**:
- ASCII text commands
- Kết thúc bằng CRLF (`\r\n`)
- Buffer size: Thường 1024-8192 bytes

**Truyền**:
```c
// Client gửi lệnh
char command[1024];
snprintf(command, sizeof(command), "USER %s\r\n", username);
send(control_sock, command, strlen(command), 0);

// Client nhận response
char response[1024];
recv(control_sock, response, sizeof(response), 0);
```

### 7.2. Data Connection

**Đóng gói**:
- Raw binary data (TYPE I) hoặc ASCII text (TYPE A)
- Không có framing hay header
- Buffer size: Thường 8192-65536 bytes

**Truyền file (Client → Server)**:
```c
#define BUFFER_SIZE 8192

FILE* fp = fopen(filename, "rb");
char buffer[BUFFER_SIZE];
size_t bytes_read;

// Đọc và gửi từng chunk
while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
    send(data_sock, buffer, bytes_read, 0);
}

fclose(fp);
close(data_sock);  // Đóng connection để báo hiệu kết thúc
```

**Nhận file (Server → Client)**:
```c
#define BUFFER_SIZE 8192

FILE* fp = fopen(filename, "wb");
char buffer[BUFFER_SIZE];
ssize_t bytes_received;

// Nhận và ghi từng chunk cho đến khi connection đóng
while ((bytes_received = recv(data_sock, buffer, BUFFER_SIZE, 0)) > 0) {
    fwrite(buffer, 1, bytes_received, fp);
}

fclose(fp);
close(data_sock);
```

### 7.3. Buffer và Performance

**Chiến lược buffering**:
- Control connection: Small buffer (1024-4096 bytes) vì chỉ truyền text commands
- Data connection: Large buffer (8192-65536 bytes) để tối ưu throughput

**TCP Socket Options**:
```c
// Set send buffer size
int sendbuf = 65536;
setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));

// Set receive buffer size
int recvbuf = 65536;
setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));

// Disable Nagle's algorithm for control connection (low latency)
int flag = 1;
setsockopt(control_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
```

---

## 8. Security và Path Validation

### 8.1. Directory Traversal Protection

**Vấn đề**: Client có thể cố gắng truy cập file ngoài root directory bằng `../`

**Ví dụ tấn công**:
```
C: CWD ../../../../etc\r\n
C: RETR passwd\r\n
```

**Giải pháp**:
```c
int is_path_allowed(const char* root, const char* path) {
    char real_root[PATH_MAX];
    char real_path[PATH_MAX];
    
    // Resolve to canonical paths
    realpath(root, real_root);
    realpath(path, real_path);
    
    // Check if path starts with root
    if (strncmp(real_path, real_root, strlen(real_root)) != 0) {
        return 0;  // Path outside root - DENIED
    }
    
    return 1;  // OK
}
```

### 8.2. User Isolation

Mỗi user có `root_dir` riêng:

```
account.txt:
admin 123456 /mnt/d/ftp-client/FTP_Server 1
user1 123456 /mnt/d/ftp-client/FTP_Client/user1 1
user2 123456 /mnt/d/ftp-client/FTP_Client/user2 1
```

User chỉ có thể truy cập trong `root_dir` của mình.

### 8.3. Permission Checks

```c
// Check file/directory permissions before access
if (access(filepath, R_OK) != 0) {
    send_response(sock, "550 Permission denied.\r\n");
    return;
}
```

---

## 9. Multi-threading Architecture

### 9.1. Server Threading Model

```
Main Thread                Client Threads
     │                          
     │  ┌────────────┐
     ├──┤ Thread 1   │──► Handle Client 1
     │  └────────────┘
     │
     │  ┌────────────┐
     ├──┤ Thread 2   │──► Handle Client 2
     │  └────────────┘
     │
     │  ┌────────────┐
     └──┤ Thread 3   │──► Handle Client 3
        └────────────┘
```

**Main Thread**:
```c
while (1) {
    client_sock = accept(listening_sock, ...);
    
    // Create new thread for each client
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, handle_client, &client_sock);
    pthread_detach(thread_id);  // Auto-cleanup
}
```

**Client Thread**:
```c
void* handle_client(void* arg) {
    int sock = *(int*)arg;
    ftp_session_t session;
    
    // Initialize session
    init_session(&session, sock);
    
    // Command loop
    while (1) {
        char command[1024];
        recv_line(sock, command, sizeof(command));
        
        if (strcmp(command, "QUIT") == 0) {
            break;
        }
        
        process_command(&session, command);
    }
    
    // Cleanup
    close(sock);
    pthread_exit(NULL);
}
```

### 9.2. Thread Safety

**Shared Resources**:
- `account.txt` file (đọc/ghi user information)
- Log files (ghi logging)

**Synchronization**:
```c
pthread_mutex_t account_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Protect account.txt writes
void update_user_root_dir(const char* username, const char* new_root) {
    pthread_mutex_lock(&account_mutex);
    
    // Update file
    FILE* fp = fopen("account.txt", "w");
    // ... write data ...
    fclose(fp);
    
    pthread_mutex_unlock(&account_mutex);
}

// Protect log writes
void write_log(const char* message) {
    pthread_mutex_lock(&log_mutex);
    
    FILE* log = fopen("logfile.txt", "a");
    fprintf(log, "%s\n", message);
    fclose(log);
    
    pthread_mutex_unlock(&log_mutex);
}
```

---

## 10. So sánh với các giao thức khác

### 10.1. FTP vs TFTP

| Đặc điểm | FTP | TFTP |
|----------|-----|------|
| Transport Protocol | TCP | UDP |
| Số lượng connection | 2 (control + data) | 1 |
| Authentication | Có (USER/PASS) | Không |
| Directory operations | Có | Không |
| Độ phức tạp | Cao | Thấp |
| Reliability | TCP đảm bảo | Phải tự implement ACK |
| Port | 21 (control), 20 (data) | 69 |

### 10.2. FTP vs HTTP

| Đặc điểm | FTP | HTTP |
|----------|-----|------|
| Mục đích | File transfer | Web content delivery |
| Persistent connection | Có (control) | HTTP/1.1 keep-alive |
| State | Stateful (session) | Stateless |
| Directory listing | Built-in | Không chuẩn |
| Transfer mode | Binary/ASCII | Chủ yếu text/binary |

### 10.3. FTP vs SFTP

| Đặc điểm | FTP | SFTP |
|----------|-----|------|
| Security | Plain text | Encrypted (SSH) |
| Port | 21 | 22 |
| Authentication | Plain text password | SSH key/password |
| Firewall friendly | Khó (2 ports) | Dễ (1 port) |

---

## 11. Ví dụ Session hoàn chỉnh

### 11.1. Kịch bản: Login, List, Download, Upload, Logout

```
--- CONNECTION ESTABLISHMENT ---
C: (TCP connect to server:2121)
S: 220 FTP Server Ready.\r\n

--- AUTHENTICATION ---
C: USER admin\r\n
S: 331 Username okay, need password.\r\n

C: PASS 123456\r\n
S: 230 User logged in, proceed.\r\n

--- DIRECTORY OPERATIONS ---
C: PWD\r\n
S: 257 "/mnt/d/ftp-client/FTP_Server" is current directory.\r\n

C: CWD /mnt/d/ftp-client/utils\r\n
S: 250 Directory successfully changed.\r\n

--- LIST FILES ---
C: TYPE A\r\n
S: 200 Type set to A.\r\n

C: PASV\r\n
S: 227 Entering Passive Mode (127,0,0,1,195,88).\r\n

C: (connect to 127.0.0.1:49992 for data)

C: LIST\r\n
S: 150 Here comes the directory listing.\r\n
S: (sends directory listing via data connection)
   -rw-r--r--   1 user  group   1524 Dec 16 10:20 account.c
   -rw-r--r--   1 user  group    986 Dec 16 10:20 account.h
   -rw-r--r--   1 user  group   2048 Dec 16 10:21 logging.c
S: (closes data connection)
S: 226 Directory send OK.\r\n

--- DOWNLOAD FILE ---
C: TYPE I\r\n
S: 200 Type set to I.\r\n

C: PASV\r\n
S: 227 Entering Passive Mode (127,0,0,1,196,100).\r\n

C: (connect to 127.0.0.1:50276 for data)

C: RETR account.c\r\n
S: 150 Opening BINARY mode data connection for account.c (1524 bytes).\r\n
S: (sends 1524 bytes of file data via data connection)
S: (closes data connection)
S: 226 Transfer complete.\r\n

--- UPLOAD FILE ---
C: PASV\r\n
S: 227 Entering Passive Mode (127,0,0,1,197,50).\r\n

C: (connect to 127.0.0.1:50482 for data)

C: STOR newfile.txt\r\n
S: 150 Ok to send data.\r\n
C: (sends file data via data connection)
C: (closes data connection)
S: 226 Transfer complete.\r\n

--- LOGOUT ---
C: QUIT\r\n
S: 221 Goodbye.\r\n
(connection closed)
```

### 11.2. Timeline Diagram

```
Time    Client                          Server
0.00s   TCP SYN                    ─────►
0.01s                              ◄───── TCP SYN-ACK
0.02s   TCP ACK                    ─────►
0.02s                              ◄───── 220 Ready
0.10s   USER admin                 ─────►
0.11s                              ◄───── 331 Need password
0.20s   PASS 123456                ─────►
0.21s                              ◄───── 230 Logged in
0.30s   PASV                       ─────►
0.31s                              ◄───── 227 Passive Mode
0.32s   (data connect)             ─────►
0.33s   RETR file.txt              ─────►
0.34s                              ◄───── 150 Opening
0.35s   (data accept)              ◄─────
0.36s                              ◄═════ File data (stream)
0.40s   (data close)               
0.41s                              ◄───── 226 Complete
...
1.00s   QUIT                       ─────►
1.01s                              ◄───── 221 Goodbye
1.02s   (close connection)
```

---

## 12. Implementation Notes

### 12.1. Key Design Decisions

1. **Port 2121 thay vì 21**: Tránh cần quyền root và xung đột với system FTP
2. **Chỉ PASV mode**: Đơn giản hóa, dễ qua firewall, không implement Active mode
3. **Multi-threading**: Hỗ trợ nhiều client đồng thời
4. **Root directory per user**: User isolation và security
5. **Auto-update account.txt**: Persistent CWD changes

### 12.2. Limitations

1. **No TLS/SSL**: Plain text transmission (có thể thêm FTPS)
2. **Simple authentication**: Plain text password (không hash)
3. **No resume capability**: Không hỗ trợ REST command
4. **No bandwidth limiting**: Không giới hạn tốc độ transfer

### 12.3. Future Improvements

- [ ] Thêm FTPS (FTP over TLS)
- [ ] Hash passwords trong account.txt
- [ ] Hỗ trợ REST command (resume transfers)
- [ ] Bandwidth throttling
- [ ] Connection timeout management
- [ ] Better error handling và logging
- [ ] IPv6 support

---

## 13. Testing và Debugging

### 13.1. Công cụ test

**1. Custom FTP Client**:
```bash
./build/ftp_client
```

**2. FileZilla Client**:
- Host: 127.0.0.1
- Port: 2121
- Username: admin
- Password: 123456

**3. Command-line FTP client**:
```bash
ftp localhost 2121
```

**4. Telnet (test control connection)**:
```bash
telnet localhost 2121
USER admin
PASS 123456
PWD
QUIT
```

### 13.2. Debugging Tools

**1. Wireshark**: Capture FTP traffic
```
Filter: tcp.port == 2121
```

**2. tcpdump**: Command-line packet capture
```bash
tcpdump -i lo -A 'port 2121'
```

**3. strace**: System call tracing
```bash
strace -e trace=network ./build/ftp_server 2121
```

**4. gdb**: Debug server
```bash
gdb ./build/ftp_server
(gdb) run 2121
```

### 13.3. Common Issues

**Issue 1: Port already in use**
```
Error: bind: Address already in use
```
**Solution**: Kill existing process hoặc đổi port

**Issue 2: Permission denied**
```
550 Permission denied.
```
**Solution**: Check file/directory permissions và root_dir path

**Issue 3: Connection timeout**
```
425 Can't open data connection.
```
**Solution**: Check firewall, ensure PASV port accessible

---

## 14. Tài liệu tham khảo

1. **RFC 959**: File Transfer Protocol (FTP)
   - https://tools.ietf.org/html/rfc959

2. **RFC 2228**: FTP Security Extensions
   - https://tools.ietf.org/html/rfc2228

3. **RFC 2389**: Feature negotiation mechanism for FTP
   - https://tools.ietf.org/html/rfc2389

4. **Beej's Guide to Network Programming**
   - https://beej.us/guide/bgnet/

5. **POSIX Socket Programming**
   - https://pubs.opengroup.org/onlinepubs/9699919799/

---

## Phụ lục: Response Code Reference

### Complete FTP Response Codes

| Code | Message | Meaning |
|------|---------|---------|
| 150 | File status okay; about to open data connection | Bắt đầu data transfer |
| 200 | Command okay | Lệnh thành công |
| 220 | Service ready for new user | Server sẵn sàng |
| 221 | Service closing control connection | Goodbye |
| 226 | Closing data connection, transfer complete | Transfer hoàn tất |
| 227 | Entering Passive Mode (h1,h2,h3,h4,p1,p2) | PASV response |
| 230 | User logged in, proceed | Login thành công |
| 250 | Requested file action okay, completed | Action thành công |
| 257 | "PATHNAME" created | Directory/file tạo thành công |
| 331 | User name okay, need password | Cần password |
| 425 | Can't open data connection | Lỗi data connection |
| 426 | Connection closed; transfer aborted | Transfer bị hủy |
| 450 | Requested file action not taken | File busy |
| 500 | Syntax error, command unrecognized | Lệnh không hợp lệ |
| 501 | Syntax error in parameters or arguments | Tham số sai |
| 502 | Command not implemented | Lệnh chưa hỗ trợ |
| 503 | Bad sequence of commands | Thứ tự lệnh sai |
| 530 | Not logged in | Chưa đăng nhập |
| 550 | Requested action not taken | File/dir không tồn tại hoặc no permission |

---


