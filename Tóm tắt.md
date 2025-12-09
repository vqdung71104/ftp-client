# TÓM TẮT DỰ ÁN FTP CLIENT-SERVER

## 1. TỔNG QUAN DỰ ÁN

### 1.1. Mục đích
Xây dựng ứng dụng client-server đơn giản cho phép:
- Đăng nhập/đăng xuất với xác thực tài khoản
- Truyền file hai chiều giữa client và server
- Quản lý thư mục làm việc của từng user
- Liệt kê file/thư mục
- Ghi log toàn bộ hoạt động

### 1.2. Kiến trúc tổng thể
```
┌─────────────────────────────────────────────────────────────────┐
│                         CLIENT (TCP_Client.c)                    │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Menu Interface:                                          │   │
│  │  1. Login            5. Change directory                 │   │
│  │  2. Upload file      6. Logout                           │   │
│  │  3. Download file    7. Exit                             │   │
│  │  4. List files                                           │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              ↓↑                                  │
│                      TCP Socket Connection                       │
│                              ↓↑                                  │
└──────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                        SERVER (TCP_Server.c)                     │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Command Handler:                                         │   │
│  │  USER, PASS → service.c (login, verify_password)        │   │
│  │  UPLD       → service.c (handle_upload_file)             │   │
│  │  DNLD       → service.c (handle_download_file)           │   │
│  │  LIST       → service.c (list_files)                     │   │
│  │  CHDIR      → service.c (change_directory)               │   │
│  │  GETDIR     → service.c (get_current_directory)          │   │
│  │  BYE        → logout logic                               │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              ↓↑                                  │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────────┐     │
│  │ account.txt │  │  User's      │  │  logs/             │     │
│  │ (accounts)  │  │  root_dir/   │  │  (activity logs)   │     │
│  └─────────────┘  └──────────────┘  └────────────────────┘     │
└──────────────────────────────────────────────────────────────────┘
```

### 1.3. Cấu trúc thư mục
```
ftp-client/
├── TCP_Client/
│   └── TCP_Client.c          # Client application
├── TCP_Server/
│   └── TCP_Server.c          # Server application
├── utils/
│   ├── account.c/h           # Quản lý tài khoản
│   ├── service.c/h           # Xử lý các service chính
│   ├── logging.c/h           # Ghi log
│   └── utils.c/h             # Các hàm tiện ích (send_all, recv_line, get_file_size)
├── build/
│   ├── client                # Executable client
│   ├── server                # Executable server
│   └── storage/              # Thư mục lưu trữ
├── logs/                     # Log files
├── account.txt               # Database tài khoản
└── Makefile                  # Build configuration
```

---

## 2. LUỒNG HOẠT ĐỘNG TỔNG THỂ

### 2.1. Khởi động Server
**File**: `TCP_Server/TCP_Server.c`

**Bước 1: Khởi tạo** (`main()` - dòng 36-80)
```c
int main(int argc, char *argv[])
```
- Nhận tham số: `./build/server <port> <storage_dir>`
- Đọc file `account.txt` → `read_file_data()` (utils/account.c)
  - Parse mỗi dòng: `username password root_dir status`
  - Lưu vào mảng global `users[]`, biến `count`
- Tạo listening socket với `socket(AF_INET, SOCK_STREAM, 0)`
- Set `SO_REUSEADDR` để tránh lỗi "Address already in use"
- Bind socket vào `INADDR_ANY:port`
- Listen với backlog = 2

**Bước 2: Accept loop** (dòng 110-125)
```c
while (1) {
    conn_sock = accept(listen_sock, ...);
    welcome_to_server(conn_sock, client_addr, client_addr_str);
    handle_client(conn_sock, client_addr_str, storage_dir);
    close(conn_sock);
}
```

**Bước 3: Handle client** (`handle_client()` - dòng 136-250)
- Vòng lặp nhận commands từ client
- Parse command thành 2 phần: `command` và `command_value`
- Dispatch đến các handler tương ứng

### 2.2. Khởi động Client
**File**: `TCP_Client/TCP_Client.c`

**Bước 1: Kết nối** (`main()` - dòng 99-150)
```c
int main(int argc, char *argv[])
```
- Nhận tham số: `./build/client <server_ip> <port>`
- Tạo socket và kết nối đến server
- Nhận welcome message từ server
- Gọi `simple_menu()` để hiển thị giao diện

**Bước 2: Menu loop** (`simple_menu()` - dòng 154-250)
```c
while (1) {
    // Hiển thị menu 7 options
    // Đọc lựa chọn từ user
    // Gọi handler tương ứng
}
```

---

## 3. CHI TIẾT CÁC CHỨC NĂNG

### 3.1. ĐĂNG NHẬP (LOGIN)

#### 3.1.1. Luồng hoạt động
```
Client                                    Server
  |                                         |
  |------ USER <username>\r\n ------------->| login()
  |                                         | - user_validation()
  |                                         | - Lưu pending_username
  |<----- 111: Need password ---------------|
  |                                         |
  |------ PASS <password>\r\n ------------->| verify_password()
  |                                         | - password_validation()
  |                                         | - Set is_logged_in = 1
  |                                         | - Lưu current_username
  |                                         | - Lưu current_root_dir
  |<----- 110: Login successful ------------|
```

#### 3.1.2. Code thực hiện

**Client side**: `TCP_Client/TCP_Client.c`
- `send_login_command()` (dòng 252-280)
  ```c
  void send_login_command(int sockfd, char *recv_buf)
  {
      // 1. Nhập username
      // 2. Gửi "USER <username>\r\n"
      // 3. Nhận response
      // 4. Nếu code 111 → nhập password
      // 5. Gửi "PASS <password>\r\n"
      // 6. Nhận và hiển thị response
  }
  ```

**Server side**: `utils/service.c`

1. **login()** (dòng 27-62)
   ```c
   void login(char *username, int client_socket)
   {
       // Kiểm tra is_logged_in
       if (is_logged_in == 1) → return 213
       
       // Gọi user_validation(username)
       int validation = user_validation(username);
       
       if (validation == 1) {
           // User valid & active
           strcpy(pending_username, username);
           return "111: Username OK, need password";
       }
       else if (validation == 0) {
           return "212: Account does NOT exist";
       }
       else {
           return "211: Account IS banned";
       }
   }
   ```

2. **user_validation()** (dòng 64-84)
   ```c
   int user_validation(char *username)
   {
       // Duyệt mảng users[]
       for (int i = 0; i < count; i++) {
           if (strcmp(users[i].username, username) == 0) {
               if (users[i].status == 1)
                   return 1;  // Valid & active
               else
                   return -1; // Banned
           }
       }
       return 0; // Not exist
   }
   ```

3. **verify_password()** (dòng 107-130)
   ```c
   void verify_password(char *password, int client_socket)
   {
       // Kiểm tra đã logged in
       if (is_logged_in == 1) → return 213
       
       // Kiểm tra pending_username
       if (strlen(pending_username) == 0) → return 220
       
       // Gọi password_validation()
       if (password_validation(pending_username, password) == 1) {
           // Password correct
           is_logged_in = 1;
           strcpy(current_username, pending_username);
           
           // Tìm và lưu root_dir
           for (int i = 0; i < count; i++) {
               if (strcmp(users[i].username, current_username) == 0) {
                   strcpy(current_root_dir, users[i].root_dir);
                   break;
               }
           }
           
           return "110: Login successful";
       }
       else {
           // Password incorrect
           memset(pending_username, 0, sizeof(pending_username));
           return "214: Incorrect password";
       }
   }
   ```

4. **password_validation()** (dòng 86-105)
   ```c
   int password_validation(char *username, char *password)
   {
       // Duyệt mảng users[]
       for (int i = 0; i < count; i++) {
           if (strcmp(users[i].username, username) == 0) {
               if (strcmp(users[i].password, password) == 0)
                   return 1; // Correct
               else
                   return 0; // Incorrect
           }
       }
       return 0; // User not found
   }
   ```

#### 3.1.3. Biến global sử dụng
**File**: `utils/account.h`
```c
extern Account users[MAX_USER];        // Mảng tài khoản
extern int count;                      // Số lượng user
extern int is_logged_in;               // Trạng thái login (0/1)
extern char current_username[100];     // Username hiện tại
extern char current_root_dir[1000];    // Root directory hiện tại
extern char pending_username[100];     // Username chờ nhập password
```

---

### 3.2. UPLOAD FILE (Tải từ Server về Client)

#### 3.2.1. Luồng hoạt động
```
Client                                    Server
  |                                         |
  |------ UPLD <filename>\r\n ------------->| handle_upload_file()
  |                                         | - Kiểm tra is_logged_in
  |                                         | - Mở file: root_dir/filename
  |                                         | - Lấy filesize
  |<----- +OK <filesize>\r\n ---------------|
  |                                         |
  |<----- [file chunk 1] -------------------|
  |<----- [file chunk 2] -------------------|
  |<----- [file chunk N] -------------------|
  |                                         |
  | (Lưu vào thư mục hiện tại)              |
```

#### 3.2.2. Code thực hiện

**Client side**: `TCP_Client/TCP_Client.c`
- `send_upload_command()` (dòng 284-380)
  ```c
  void send_upload_command(int sockfd, char *recv_buf)
  {
      // 1. Nhập tên file cần tải về
      char filename[256];
      printf("Enter filename to download from server: ");
      fgets(filename, sizeof(filename), stdin);
      
      // 2. Gửi command "UPLD <filename>\r\n"
      snprintf(command, sizeof(command), "UPLD %s\r\n", filename);
      send(sockfd, command, strlen(command), 0);
      
      // 3. Nhận response bằng recv_line()
      recv_line(sockfd, buffer, BUFF_SIZE);
      
      // 4. Parse response "+OK <filesize>"
      sscanf(buffer, "+OK %ld", &filesize);
      
      // 5. Tạo file local để ghi
      FILE *fp = fopen(filename, "wb");
      
      // 6. Nhận dữ liệu theo chunks
      while (total_received < filesize) {
          bytes_received = recv(sockfd, buffer, to_recv, 0);
          fwrite(buffer, 1, bytes_received, fp);
          total_received += bytes_received;
      }
      
      // 7. Đóng file và hiển thị kết quả
      fclose(fp);
  }
  ```

**Server side**: `utils/service.c`
- `handle_upload_file()` (dòng 132-206)
  ```c
  void handle_upload_file(int conn_sock, const char *client_addr_str, 
                          char *command_value, const char *request_log)
  {
      // 1. Kiểm tra is_logged_in
      if (is_logged_in == 0) {
          send "221: You have NOT logged in";
          return;
      }
      
      // 2. command_value = filename
      strncpy(filename, command_value, sizeof(filename) - 1);
      
      // 3. Tạo đường dẫn đầy đủ
      snprintf(filepath, sizeof(filepath), "%s/%s", 
               current_root_dir, filename);
      
      // 4. Mở file để đọc
      FILE *fp = fopen(filepath, "rb");
      if (fp == NULL) {
          send "ERR: File not found";
          return;
      }
      
      // 5. Lấy kích thước file
      long file_size = get_file_size(fp);
      
      // 6. Gửi response "+OK <filesize>\r\n"
      snprintf(return_msg, sizeof(return_msg), "+OK %ld\r\n", file_size);
      send_all(conn_sock, return_msg, strlen(return_msg));
      usleep(10); // Small delay để tránh dính packet
      
      // 7. Đọc và gửi file theo chunks (16KB)
      while ((bytes_read = fread(file_buffer, 1, 16384, fp)) > 0) {
          send_all(conn_sock, file_buffer, bytes_read);
          total_sent += bytes_read;
      }
      
      // 8. Đóng file và log
      fclose(fp);
      log_message(client_addr_str, request_log, response_log);
  }
  ```

#### 3.2.3. Hàm tiện ích sử dụng

1. **get_file_size()** - `utils/utils.c` (dòng 144-155)
   ```c
   long get_file_size(FILE *fp)
   {
       fseek(fp, 0L, SEEK_END);    // Di chuyển con trỏ đến cuối file
       long size = ftell(fp);       // Lấy vị trí = kích thước
       fseek(fp, 0L, SEEK_SET);    // Quay lại đầu file
       return size;
   }
   ```

2. **send_all()** - `utils/utils.c` (dòng 83-102)
   ```c
   int send_all(int socket, const char *buffer, int length)
   {
       int total_sent = 0;
       while (total_sent < length) {
           int n = send(socket, buffer + total_sent, 
                       length - total_sent, 0);
           if (n == -1) return -1;
           total_sent += n;
       }
       return 0;
   }
   ```

3. **recv_line()** - `utils/utils.c` (dòng 116-142)
   ```c
   int recv_line(int sockfd, char *buf, int max_len)
   {
       int i = 0;
       char c;
       
       // Đọc từng byte cho đến khi gặp \r\n
       while (1) {
           int n = recv(sockfd, &c, 1, 0);
           if (n <= 0) return n;
           
           if (c == '\r') {
               // Đọc tiếp \n
               recv(sockfd, &c, 1, 0);
               break;
           }
           buf[i++] = c;
       }
       buf[i] = '\0';
       return i;
   }
   ```

---

### 3.3. DOWNLOAD FILE (Gửi từ Client lên Server)

#### 3.3.1. Luồng hoạt động
```
Client                                    Server
  |                                         |
  |------ DNLD <filename> <size>\r\n ------>| handle_download_file()
  |                                         | - Kiểm tra is_logged_in
  |                                         | - Parse filename & filesize
  |                                         | - Tạo file: root_dir/filename
  |<----- +OK Please send file -------------|
  |                                         |
  |------ [file chunk 1] ------------------>|
  |------ [file chunk 2] ------------------>|
  |------ [file chunk N] ------------------>|
  |                                         | - Ghi vào file
  |<----- OK Successful upload -------------|
```

#### 3.3.2. Code thực hiện

**Client side**: `TCP_Client/TCP_Client.c`
- `send_download_command()` (dòng 504-590)
  ```c
  void send_download_command(int sockfd, char *recv_buf)
  {
      // 1. Nhập đường dẫn file cần upload
      char filepath[1024];
      printf("Enter file path to upload: ");
      fgets(filepath, sizeof(filepath), stdin);
      
      // 2. Mở file để đọc
      FILE *fp = fopen(filepath, "rb");
      
      // 3. Lấy kích thước file
      long file_size = get_file_size(fp);
      
      // 4. Lấy tên file (basename)
      char *filename_ptr = basename(filepath);
      strncpy(filename, filename_ptr, sizeof(filename) - 1);
      
      // 5. Gửi command "DNLD <filename> <filesize>\r\n"
      snprintf(command, sizeof(command), "DNLD %s %ld\r\n", 
               filename, file_size);
      send(sockfd, command, strlen(command), 0);
      
      // 6. Chờ response "+OK Please send file"
      recv(sockfd, buffer, BUFF_SIZE, 0);
      
      // 7. Gửi file theo chunks
      while((bytes_read = fread(buffer, 1, BUFF_SIZE, fp)) > 0) {
          send(sockfd, buffer, bytes_read, 0);
          total_sent += bytes_read;
      }
      
      // 8. Đóng file
      fclose(fp);
      
      // 9. Nhận kết quả cuối cùng
      recv(sockfd, buffer, BUFF_SIZE, 0);
      printf("Result: %s\n", buffer);
  }
  ```

**Server side**: `utils/service.c`
- `handle_download_file()` (dòng 345-453)
  ```c
  void handle_download_file(int conn_sock, const char *client_addr_str,
                            char *command_value, const char *request_log)
  {
      // 1. Kiểm tra is_logged_in
      if (is_logged_in == 0) {
          send "221: You have NOT logged in";
          return;
      }
      
      // 2. Parse command_value = "<filename> <filesize>"
      char *last_space = strrchr(command_value, ' ');
      if (last_space == NULL) {
          send "ERR Invalid command";
          return;
      }
      
      // 3. Tách filename và filesize
      *last_space = '\0';
      char *size_str = last_space + 1;
      
      // 4. Convert filesize string → unsigned long
      unsigned long fs = strtoul(size_str, &endptr, 10);
      if (endptr == size_str || fs == 0) {
          send "ERR Invalid filesize";
          return;
      }
      
      file_size = fs;
      strncpy(filename, command_value, sizeof(filename) - 1);
      
      // 5. Tạo đường dẫn đầy đủ
      snprintf(filepath, sizeof(filepath), "%s/%s",
               current_root_dir, filename);
      
      // 6. Tạo thư mục nếu chưa tồn tại
      mkdir(current_root_dir, 0755);
      
      // 7. Mở file để ghi
      FILE *fp = fopen(filepath, "wb");
      if (fp == NULL) {
          send "ERR Cannot create file";
          return;
      }
      
      // 8. Gửi OK
      send_all(conn_sock, "+OK Please send file\r\n", ...);
      
      // 9. Nhận file theo chunks (16KB)
      while (total_received < file_size) {
          bytes_in_chunk = recv(conn_sock, file_buffer, 16384, 0);
          fwrite(file_buffer, 1, bytes_in_chunk, fp);
          total_received += bytes_in_chunk;
      }
      
      // 10. Đóng file
      fclose(fp);
      
      // 11. Gửi kết quả
      if (total_received == file_size) {
          send "OK Successful upload";
      }
      else {
          send "ERR Upload failed";
          remove(filepath); // Xóa file không hoàn chỉnh
      }
      
      // 12. Log
      log_message(client_addr_str, request_log, response_log);
  }
  ```

---

### 3.4. LIST FILES (Liệt kê file/thư mục)

#### 3.4.1. Luồng hoạt động
```
Client                                    Server
  |                                         |
  |------ LIST\r\n ------------------------>| list_files()
  |                                         | - Kiểm tra is_logged_in
  |                                         | - opendir(current_root_dir)
  |<----- 150: Listing directory... --------|
  |<----- [DIR] folder1 --------------------|
  |<----- [FILE] file1.txt (1024 bytes) ----|
  |<----- [FILE] file2.txt (2048 bytes) ----|
  |<----- END -------------------------------|
  |                                         |
  | (Hiển thị danh sách)                    |
```

#### 3.4.2. Code thực hiện

**Client side**: `TCP_Client/TCP_Client.c`
- `send_list_command()` (dòng 453-502)
  ```c
  void send_list_command(int sockfd, char *recv_buf)
  {
      // 1. Gửi command "LIST\r\n"
      send_all(sockfd, "LIST\r\n", 6);
      
      // 2. Nhận header response
      recv_line(sockfd, recv_buf, BUFF_SIZE);
      printf("%s\n", recv_buf); // In "150: Listing directory..."
      
      // 3. Nhận từng dòng cho đến khi gặp "END"
      while (1) {
          recv_line(sockfd, recv_buf, BUFF_SIZE);
          
          if (strcmp(recv_buf, "END") == 0) {
              break; // Kết thúc
          }
          
          printf("%s\n", recv_buf); // In mỗi dòng
      }
  }
  ```

**Server side**: `utils/service.c`
- `list_files()` (dòng 287-343)
  ```c
  void list_files(int client_socket)
  {
      // 1. Kiểm tra is_logged_in
      if (is_logged_in == 0) {
          send "221: You have NOT logged in";
          return;
      }
      
      // 2. Gửi header
      snprintf(return_msg, sizeof(return_msg),
               "150: Listing directory: %s\r\n", current_root_dir);
      send_all(client_socket, return_msg, strlen(return_msg));
      usleep(10);
      
      // 3. Mở thư mục
      DIR *dir = opendir(current_root_dir);
      if (dir == NULL) {
          send "ERR: Cannot open directory";
          return;
      }
      
      // 4. Duyệt từng entry
      struct dirent *entry;
      while ((entry = readdir(dir)) != NULL) {
          // Bỏ qua "." và ".."
          if (strcmp(entry->d_name, ".") == 0 ||
              strcmp(entry->d_name, "..") == 0) {
              continue;
          }
          
          // Tạo đường dẫn đầy đủ
          snprintf(fullpath, sizeof(fullpath), "%s/%s",
                   current_root_dir, entry->d_name);
          
          // Lấy thông tin file
          struct stat file_stat;
          if (stat(fullpath, &file_stat) == -1) {
              continue;
          }
          
          // Kiểm tra loại
          if (S_ISDIR(file_stat.st_mode)) {
              // Là thư mục
              snprintf(return_msg, sizeof(return_msg),
                       "[DIR] %s\r\n", entry->d_name);
          }
          else {
              // Là file
              snprintf(return_msg, sizeof(return_msg),
                       "[FILE] %s (%ld bytes)\r\n",
                       entry->d_name, file_stat.st_size);
          }
          
          // Gửi dòng này
          send_all(client_socket, return_msg, strlen(return_msg));
          usleep(10);
      }
      
      // 5. Đóng thư mục
      closedir(dir);
      
      // 6. Gửi marker kết thúc
      send_all(client_socket, "END\r\n", 5);
      usleep(10);
  }
  ```

---

### 3.5. CHANGE DIRECTORY (Thay đổi thư mục làm việc)

#### 3.5.1. Luồng hoạt động
```
Client                                    Server
  |                                         |
  |------ GETDIR\r\n --------------------->| get_current_directory()
  |<----- 150: Current directory: /path ----|
  |                                         |
  | (User quyết định có đổi hay không)      |
  |                                         |
  |------ CHDIR <new_path>\r\n ----------->| change_directory()
  |                                         | - Cập nhật current_root_dir
  |                                         | - Cập nhật users[i].root_dir
  |                                         | - Ghi lại account.txt
  |<----- 140: Directory changed ----------|
```

#### 3.5.2. Code thực hiện

**Client side**: `TCP_Client/TCP_Client.c`
- `send_chdir_command()` (dòng 383-451)
  ```c
  void send_chdir_command(int sockfd, char *recv_buf)
  {
      // 1. Gửi GETDIR để lấy thư mục hiện tại
      send_all(sockfd, "GETDIR\r\n", 8);
      recv_line(sockfd, recv_buf, BUFF_SIZE);
      printf("Thư mục làm việc hiện tại: %s\n", recv_buf);
      
      // 2. Hỏi user có muốn đổi không
      char choice;
      printf("Bạn có muốn đổi thư mục làm việc không? y/n: ");
      scanf(" %c", &choice);
      
      if (choice == 'y' || choice == 'Y') {
          // 3. Nhập thư mục mới
          char new_dir[512];
          printf("Vui lòng nhập thư mục làm việc mới: ");
          fgets(new_dir, sizeof(new_dir), stdin);
          new_dir[strcspn(new_dir, "\r\n")] = 0;
          
          // 4. Gửi CHDIR command
          char command[600];
          snprintf(command, sizeof(command), "CHDIR %s\r\n", new_dir);
          send_all(sockfd, command, strlen(command));
          
          // 5. Nhận response
          recv_line(sockfd, recv_buf, BUFF_SIZE);
          printf("Server response: %s\n", recv_buf);
      }
  }
  ```

**Server side**: `utils/service.c`

1. **get_current_directory()** (dòng 245-260)
   ```c
   void get_current_directory(int client_socket)
   {
       // Kiểm tra login
       if (is_logged_in == 0) {
           send "221: You have NOT logged in";
           return;
       }
       
       // Gửi current_root_dir
       snprintf(return_msg, sizeof(return_msg),
                "150: Current directory: %s\r\n", current_root_dir);
       send_all(client_socket, return_msg, strlen(return_msg));
   }
   ```

2. **change_directory()** (dòng 262-285)
   ```c
   void change_directory(char *new_dir, int client_socket)
   {
       // 1. Kiểm tra login
       if (is_logged_in == 0) {
           send "221: You have NOT logged in";
           return;
       }
       
       // 2. Cập nhật current_root_dir
       strcpy(current_root_dir, new_dir);
       
       // 3. Tìm và cập nhật users[i].root_dir
       for (int i = 0; i < count; i++) {
           if (strcmp(users[i].username, current_username) == 0) {
               strcpy(users[i].root_dir, new_dir);
               break;
           }
       }
       
       // 4. Ghi lại toàn bộ mảng users[] vào account.txt
       FILE *fp = fopen("account.txt", "w");
       if (fp == NULL) {
           send "240: Failed to update account file";
           return;
       }
       
       for (int i = 0; i < count; i++) {
           fprintf(fp, "%s %s %s %d\n",
                   users[i].username,
                   users[i].password,
                   users[i].root_dir,
                   users[i].status);
       }
       fclose(fp);
       
       // 5. Gửi success
       send "140: Directory changed successfully";
   }
   ```

---

### 3.6. LOGOUT (Đăng xuất)

#### 3.6.1. Luồng hoạt động
```
Client                                    Server
  |                                         |
  |------ BYE\r\n ------------------------>| (trong handle_client)
  |                                         | - Set is_logged_in = 0
  |                                         | - Xóa current_username
  |                                         | - Xóa current_root_dir
  |<----- 130: Logout successfully ---------|
```

#### 3.6.2. Code thực hiện

**Client side**: `TCP_Client/TCP_Client.c`
- `send_bye_command()` (dòng 593-601)
  ```c
  void send_bye_command(int sockfd, char *recv_buf)
  {
      // Gửi BYE
      send_all(sockfd, "BYE\r\n", 5);
      
      // Nhận response
      recv_line(sockfd, recv_buf, BUFF_SIZE);
      printf("%s\n", recv_buf);
  }
  ```

**Server side**: `TCP_Server/TCP_Server.c`
- Trong `handle_client()` (dòng 220-230)
  ```c
  else if (strcmp(command, "BYE") == 0)
  {
      if (is_logged_in == 1) {
          // Reset trạng thái
          is_logged_in = 0;
          memset(current_username, 0, sizeof(current_username));
          memset(current_root_dir, 0, sizeof(current_root_dir));
          
          // Gửi response
          send_all(conn_sock, "130: Logout successfully\r\n", 26);
          response_log = "130: Logout successfully";
      }
      else {
          send_all(conn_sock, "221: You have NOT logged in\r\n", 29);
          response_log = "221: Not logged in";
      }
      
      log_message(client_addr_str, request_log, response_log);
      break; // Thoát vòng lặp, đóng connection
  }
  ```

---

## 4. LOGGING SYSTEM

### 4.1. Cơ chế hoạt động
**File**: `utils/logging.c`

**Hàm chính**: `log_message()` (dòng 8-60)
```c
void log_message(const char *client_addr, const char *request, 
                 const char *response)
{
    // 1. Lấy thời gian hiện tại
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    // 2. Tạo tên file log: log_YYYYMMDD.txt
    char log_filename[256];
    snprintf(log_filename, sizeof(log_filename),
             "logs/log_%04d%02d%02d.txt",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    
    // 3. Mở file để append
    FILE *log_file = fopen(log_filename, "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        return;
    }
    
    // 4. Ghi log với format:
    // [YYYY-MM-DD HH:MM:SS] <IP:Port> | Request: <cmd> | Response: <res>
    fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] %s | Request: %s | Response: %s\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec,
            client_addr, request, response);
    
    // 5. Đóng file
    fclose(log_file);
}
```

### 4.2. Ví dụ log
```
[2025-12-09 14:30:15] 127.0.0.1:52314 | Request: USER admin | Response: 111: Need password
[2025-12-09 14:30:20] 127.0.0.1:52314 | Request: PASS ****** | Response: 110: Login successful
[2025-12-09 14:30:25] 127.0.0.1:52314 | Request: LIST | Response: 150: Listing directory
[2025-12-09 14:30:35] 127.0.0.1:52314 | Request: UPLD test.txt | Response: +OK File sent successfully
[2025-12-09 14:30:50] 127.0.0.1:52314 | Request: DNLD data.txt 2048 | Response: +OK Successful upload
[2025-12-09 14:31:00] 127.0.0.1:52314 | Request: BYE | Response: 130: Logout successfully
```

---

## 5. PROTOCOL VÀ RESPONSE CODES

### 5.1. Commands
| Command | Format | Mô tả |
|---------|--------|-------|
| USER | `USER <username>\r\n` | Gửi username để login |
| PASS | `PASS <password>\r\n` | Gửi password để xác thực |
| UPLD | `UPLD <filename>\r\n` | Tải file từ server về client |
| DNLD | `DNLD <filename> <size>\r\n` | Gửi file từ client lên server |
| LIST | `LIST\r\n` | Liệt kê file trong thư mục |
| GETDIR | `GETDIR\r\n` | Lấy thư mục làm việc hiện tại |
| CHDIR | `CHDIR <new_dir>\r\n` | Thay đổi thư mục làm việc |
| BYE | `BYE\r\n` | Đăng xuất |

### 5.2. Response Codes
**1xx: Thành công**
- `110`: Login successful
- `111`: Username OK, need password
- `130`: Logout successfully
- `140`: Directory changed successfully
- `150`: Listing directory / Current directory

**2xx: Lỗi user**
- `211`: Account IS banned
- `212`: Account does NOT exist
- `213`: You have already logged in
- `214`: Incorrect password
- `220`: Please send username first
- `221`: You have NOT logged in
- `240`: Failed to update account file

**3xx: Lỗi command**
- `300`: Unknown command

**+OK/ERR: File transfer**
- `+OK <filesize>`: Server gửi file (UPLD)
- `+OK Please send file`: Server sẵn sàng nhận (DNLD)
- `OK Successful upload`: Upload hoàn tất
- `ERR Invalid command`: Command không hợp lệ
- `ERR Invalid filesize`: Filesize không hợp lệ
- `ERR Cannot create file`: Không tạo được file
- `ERR Upload failed`: Upload thất bại
- `ERR: File not found`: File không tồn tại
- `ERR: Cannot open directory`: Không mở được thư mục

**Marker**
- `END`: Kết thúc danh sách file

---

## 6. BUILD SYSTEM (Makefile)

### 6.1. Cấu trúc
**File**: `Makefile`

```makefile
# Compiler
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -D_POSIX_C_SOURCE=200112L -Iutils

# Directories
BUILD_DIR = build
TARGET_CLIENT = $(BUILD_DIR)/client
TARGET_SERVER = $(BUILD_DIR)/server

# Source files
UTILS_SRC = $(wildcard utils/*.c)
CLIENT_SRC = TCP_Client/TCP_Client.c
SERVER_SRC = TCP_Server/TCP_Server.c

# Object files
UTILS_OBJS = $(patsubst utils/%.c, $(BUILD_DIR)/%.o, $(UTILS_SRC))
CLIENT_OBJ = $(patsubst TCP_Client/%.c, $(BUILD_DIR)/%.o, $(CLIENT_SRC))
SERVER_OBJ = $(patsubst TCP_Server/%.c, $(BUILD_DIR)/%.o, $(SERVER_SRC))
```

### 6.2. Build process
```bash
make clean && make
```

**Bước 1**: Xóa thư mục build
**Bước 2**: Biên dịch từng file .c thành .o
- `utils/account.c` → `build/account.o`
- `utils/logging.c` → `build/logging.o`
- `utils/service.c` → `build/service.o`
- `utils/utils.c` → `build/utils.o`
- `TCP_Client/TCP_Client.c` → `build/TCP_Client.o`
- `TCP_Server/TCP_Server.c` → `build/TCP_Server.o`

**Bước 3**: Liên kết thành executable
- `build/client`: TCP_Client.o + tất cả utils objs
- `build/server`: TCP_Server.o + tất cả utils objs

**Bước 4**: Tạo thư mục
- `build/storage/`
- `logs/`

---

## 7. SESSION MANAGEMENT

### 7.1. Global state
**File**: `utils/account.c`

```c
// Mảng tài khoản đọc từ account.txt
Account users[MAX_USER];
int count = 0;

// Trạng thái session hiện tại
int is_logged_in = 0;
char current_username[100] = {0};
char current_root_dir[1000] = {0};
char pending_username[100] = {0};
```

### 7.2. Vòng đời session
```
1. Client kết nối → Server chấp nhận
   is_logged_in = 0
   
2. Client gửi USER → Server lưu pending_username
   is_logged_in = 0
   
3. Client gửi PASS → Server xác thực thành công
   is_logged_in = 1
   current_username = "admin"
   current_root_dir = "/path/to/admin"
   
4. Client thực hiện các thao tác (UPLD, DNLD, LIST...)
   Mọi operation đều check is_logged_in trước
   
5. Client gửi BYE hoặc disconnect
   is_logged_in = 0
   current_username = ""
   current_root_dir = ""
   pending_username = ""
```

### 7.3. Security considerations
- Mỗi connection chỉ có 1 session
- Server chạy iterative (1 client tại 1 thời điểm)
- Không có cơ chế timeout
- Password không được mã hóa trong account.txt và network
- Không có rate limiting

---

## 8. ERROR HANDLING

### 8.1. Network errors
```c
// Send error
if (send(...) < 0) {
    perror("send() error");
    return;
}

// Recv error
if (recv(...) <= 0) {
    printf("Connection closed or error\n");
    return;
}
```

### 8.2. File errors
```c
// File not found
FILE *fp = fopen(path, "rb");
if (fp == NULL) {
    send "ERR: File not found";
    return;
}

// Cannot create file
FILE *fp = fopen(path, "wb");
if (fp == NULL) {
    send "ERR Cannot create file";
    return;
}
```

### 8.3. Authentication errors
```c
// Not logged in
if (is_logged_in == 0) {
    send "221: You have NOT logged in";
    return;
}

// Already logged in
if (is_logged_in == 1) {
    send "213: You have already logged in";
    return;
}
```

---

## 9. KẾT LUẬN

### 9.1. Điểm mạnh
1. **Kiến trúc rõ ràng**: Tách biệt client, server, utilities
2. **Protocol đơn giản**: Dễ debug, dễ mở rộng
3. **Logging đầy đủ**: Theo dõi được mọi hoạt động
4. **Build system tự động**: Makefile quản lý dependencies

### 9.2. Hạn chế
1. **Iterative server**: Chỉ xử lý 1 client tại 1 thời điểm
2. **No encryption**: Password và data không được mã hóa
3. **No authentication token**: Session dựa vào biến global
4. **Limited error recovery**: Không có retry mechanism
5. **Buffer overflow risks**: Một số hàm chưa validate đầy đủ

### 9.3. Cải tiến có thể làm
1. Chuyển sang **concurrent server** (fork/thread/epoll)
2. Thêm **SSL/TLS** cho encryption
3. Implement **session token** thay vì global state
4. Thêm **timeout** cho idle connections
5. Validate và sanitize input nghiêm ngặt hơn
6. Thêm **progress indicator** cho file transfer
7. Support **resume/pause** file transfer
8. Implement **access control** chi tiết hơn
