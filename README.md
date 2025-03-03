# ABANS Stock Client

This project is a stock market data client application that connects to a mock exchange server, fetches real-time stock ticker data, and ensures no missing sequences in the final JSON output.

## Prerequisites

Ensure you have the following installed on your system:

1. **Node.js** (for running `main.js` if required by the exchange server)
2. **C++ Compiler (g++)**
   - Install MinGW-w64 (Windows)
   - Ensure `winsock2` library is available
3. **Mock Exchange Server** running on `127.0.0.1:3000`

## Installation Steps

### **1. Start the Exchange Server**

If your project requires running `main.js` to simulate the stock exchange server, start it using:

```sh
node main.js
```

Ensure the server is running before starting the client.

### **2. Compile the C++ Client**

Run the following command to compile `client.cpp`:

```sh
g++ client.cpp -o client.exe -mconsole -lws2_32
```

This will generate the `client.exe` executable.

### **3. Run the Client**

Execute the compiled client:

```sh
./client.exe
```

### **4. Output JSON File**

Once the program executes successfully, it will generate a file named `output.json` that contains stock market data in JSON format.

## Troubleshooting

### **Compilation Issues**
- Ensure `g++` is installed and available in your system's PATH.
- If using MinGW, ensure `winsock2` is included (`ws2_32.lib`).

### **Server Connection Issues**
- Ensure the exchange server is running on `127.0.0.1:3000`.
- Check firewall settings if the client cannot establish a connection.

### **Permission Errors**
- Run the command prompt as an administrator if permission issues arise.


