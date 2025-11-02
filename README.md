# FTP Client-Server Application

## Mô tả

Dự án này là một ứng dụng client-server đơn giản với các chức năng:
- **Đăng nhập (LOGIN)**: Xác thực người dùng với tài khoản trong file `account.txt`
- **Đăng xuất (LOGOUT)**: Đăng xuất khỏi hệ thống
- **Đăng bài (POST)**: Đăng một tin nhắn lên server (yêu cầu đã đăng nhập)
- **Upload file (UPLD)**: Upload file lên server (lưu vào thư mục storage)

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
2. Post message
3. Upload file
4. Logout
5. Exit
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

**2. Post message**
- Nhập nội dung tin nhắn để đăng
- Yêu cầu đã đăng nhập
- Phản hồi:
  - `120: Post successfully. With content: <message>` - Đăng bài thành công
  - `221: You have NOT logged in` - Chưa đăng nhập

**3. Upload file**
- Nhập đường dẫn file cần upload
- File sẽ được lưu vào thư mục storage của server
- Phản hồi:
  - `+OK Please send file` - Server sẵn sàng nhận file
  - `OK Successful upload` - Upload thành công
  - `ERR Upload failed` - Upload thất bại

**4. Logout**
- Đăng xuất khỏi hệ thống
- Phản hồi:
  - `130: Logout successfully` - Đăng xuất thành công
  - `221: You have NOT logged in` - Chưa đăng nhập

**5. Exit**
- Thoát chương trình client

## Protocol

### Format lệnh

Tất cả các lệnh gửi lên server đều kết thúc bằng `\r\n`

1. **USER command**: `USER <username>\r\n`
2. **POST command**: `POST <message>\r\n`
3. **UPLD command**: `UPLD <filename> <filesize>\r\n`
4. **BYE command**: `BYE\r\n`

### Response codes

- `1xx`: Thành công
- `2xx`: Lỗi liên quan đến user
- `3xx`: Lỗi command không hợp lệ
- `+OK`: Thành công (dùng cho upload file)
- `ERR`: Lỗi (dùng cho upload file)

## Logging

Server ghi log tất cả các hoạt động vào file trong thư mục `logs/`. Log bao gồm:
- Thời gian
- Địa chỉ IP:Port của client
- Lệnh được gửi
- Kết quả xử lý

Format log: `[YYYY-MM-DD HH:MM:SS] <IP:Port> | Request: <command> | Response: <result>`

## Lưu ý

1. Server chạy ở chế độ iterative (xử lý từng client một lần)
2. File `account.txt` phải tồn tại trước khi chạy server
3. Thư mục storage sẽ được tự động tạo nếu chưa tồn tại
4. Client sử dụng `recv_line()` để đọc response từ server (kết thúc bằng `\r\n`)
5. File upload không yêu cầu đăng nhập (có thể thay đổi logic nếu cần)

## Tác giả

Project được phát triển theo yêu cầu của khóa học Network Programming.
