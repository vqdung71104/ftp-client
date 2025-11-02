#!/bin/bash

# Script to demonstrate the FTP Client-Server application

echo "=========================================="
echo "FTP Client-Server Application Demo"
echo "=========================================="
echo ""

# Check if executables exist
if [ ! -f "build/server" ] || [ ! -f "build/client" ]; then
    echo "Error: Executables not found. Please run 'make' first."
    exit 1
fi

# Check if account.txt exists
if [ ! -f "account.txt" ]; then
    echo "Error: account.txt not found. Creating a sample file..."
    cat > account.txt << EOF
tungbt 1
admin 1
user1 1
user2 0
testuser 1
EOF
    echo "Sample account.txt created."
fi

echo "Available test accounts:"
echo "  - tungbt (active)"
echo "  - admin (active)"
echo "  - user1 (active)"
echo "  - user2 (banned)"
echo "  - testuser (active)"
echo ""

echo "To test the application:"
echo ""
echo "1. Start the server in one terminal:"
echo "   ./build/server 8080 build/storage"
echo ""
echo "2. Start the client in another terminal:"
echo "   ./build/client 127.0.0.1 8080"
echo ""
echo "3. Try the following operations:"
echo "   - Login with 'tungbt'"
echo "   - Post a message"
echo "   - Upload a file"
echo "   - Logout"
echo ""
echo "4. Check logs in the logs/ directory"
echo ""
echo "=========================================="
