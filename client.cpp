// Include the necessary headers for networking, I/O, and data handling
#include <iostream>
#include <winsock2.h>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <fstream>

#pragma comment(lib, "ws2_32.lib") // Linking Winsock library

#define SERVER_IP "127.0.0.1" // Server running locally
#define SERVER_PORT 3000      // Port server listening on 3000
#define TIMEOUT_SEC 500       // Timeout in micro-seconds

// packets for stock market data
struct StockPacket
{
    std::string symbol; // Symbol of Stock
    char buySell;   // Buy or Sell
    int quantity;   // Quantity of stock
    int price;  // price of stock
    int sequence;
};

// Parsing the raw buffer received into StockPacket struct
StockPacket parsePacket(const char *buffer)
{
    StockPacket packet;
    packet.symbol = std::string(buffer, 4);                               // First 4 chars for symbol
    packet.buySell = buffer[4];                                           // Buy or sell flag
    packet.quantity = ntohl(*reinterpret_cast<const int *>(buffer + 5));  // Quantity as int
    packet.price = ntohl(*reinterpret_cast<const int *>(buffer + 9));     // Price as int
    packet.sequence = ntohl(*reinterpret_cast<const int *>(buffer + 13)); // Sequence number as int
    return packet;
}

// Convert packets into JSON format
std::string generateJSON(const std::vector<StockPacket> &packets)
{
    std::ostringstream json;
    json << "[\n";
    for (size_t i = 0; i < packets.size(); ++i)
    {
        json << "  {\n";
        json << "    \"symbol\": \"" << packets[i].symbol << "\",\n";
        json << "    \"buySell\": \"" << packets[i].buySell << "\",\n";
        json << "    \"quantity\": " << packets[i].quantity << ",\n";
        json << "    \"price\": " << packets[i].price << ",\n";
        json << "    \"sequence\": " << packets[i].sequence << "\n";
        json << "  }" << (i < packets.size() - 1 ? "," : "") << "\n";
    }
    json << "]";
    return json.str();
}

// Save the JSON output into a file
void saveToJsonFile(const std::string &jsonOutput, const std::string &filename)
{
    std::ofstream outFile(filename);
    if (outFile.is_open())
    {
        outFile << jsonOutput;
        outFile.close();
        std::cout << "Output saved in " << filename << std::endl;
    }
    else
    {
        std::cerr << "Could not write file " << filename << std::endl;
    }
}

// Establishing TCP connection to the server
SOCKET connectToServer()
{
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;

    // Initializing Winsock API
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return INVALID_SOCKET;
    }

    // Creating socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed with Sever\n";
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Setting up server address details
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(SERVER_PORT);

    // Attempt to connect
    if (connect(sock, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Connection Failed with Server\n";
        closesocket(sock);
        WSACleanup();
        return INVALID_SOCKET;
    }
    return sock;
}

// Fetching packets from server, either callType = 1 or callType = 2
std::vector<StockPacket> fetchPackets(SOCKET sock, char callType, int resendSeq = 0)
{
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode); // Make socket non-blocking
    fd_set readfds;
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT_SEC;
    std::vector<StockPacket> packets;
    char request[2] = {callType, static_cast<char>(resendSeq)};

    // Sending request to server
    if (send(sock, request, sizeof(request), 0) == SOCKET_ERROR)
    {
        std::cerr << "Send failed\n";
        return packets;
    }

    char buffer[17]; // Buffer for received packets, buffer size = data packet size

    while (true)
    {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        int result = select(0, &readfds, nullptr, nullptr, &timeout);
        if (result == 0)
        {
            std::cout << "Timeout, (No more data).\n";
            break;
        }
        else if (result < 0)
        {
            std::cerr << "SELECT Error\n";
            break;
        }

        // result > 0, means server has sent some data concering the request from client
        int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0)
        {
            packets.push_back(parsePacket(buffer));
        }
        else if (bytesReceived == 0)
        {
            std::cout << "Connection hass been closed by the Server.\n";
            break;
        }
        else
        {
            std::cerr << "Error in Receiving Packets \n";
            break;
        }
    }
    return packets;
}

int main()
{
    // establish connection to the server
    SOCKET sock = connectToServer();
    if (sock == INVALID_SOCKET)
    {
        return 1;
    }

    // Fetch all packets from server first, for callType = 1
    std::vector<StockPacket> packets = fetchPackets(sock, 1);

    // Store received packets in a map to track sequences
    // Sequence number as key and Packet-Struct as value
    std::map<int, StockPacket> packetMap;

    // Wrote minSeq as 1 and maxSeq as 14 because server gives error for seq no. more than 14 or less than 1
    // Otherwise could have written logic to detect maximum sequence number from server
    int minSeq = 0, maxSeq = 14;
    for (const auto &p : packets)
    {
        packetMap[p.sequence] = p;
    }

    // Finding missing sequence nu7mbers
    std::vector<int> missingSeqNumber;
    for (int i = minSeq; i <= maxSeq; ++i)
    {
        if (packetMap.find(i) == packetMap.end())
        {
            missingSeqNumber.push_back(i);
        }
    }

    // disconnecting server after receiving all thge stream packets for callType 1
    closesocket(sock);
    sock = connectToServer();
    if (sock == INVALID_SOCKET)
    {
        return 1;
    }

    // Request missing packets, for callType = 2
    for (int seq : missingSeqNumber)
    {
        std::vector<StockPacket> missingPacket = fetchPackets(sock, 2, seq);
        if (!missingPacket.empty())
        {
            packetMap[seq] = missingPacket[0];
        }
        else
        {
            std::cerr << "Warning: Sequence " << seq << " not retrieved.\n";
        }
    }

    // Close the connection
    closesocket(sock);
    WSACleanup();

    // sequence number stored in sorted order, because 'packetMap' map stores key in sorted order
    std::vector<StockPacket> sortedPackets;
    for (auto it : packetMap)
    {
        sortedPackets.push_back(packetMap[it.first]);
    }

    std::string jsonOutput = generateJSON(sortedPackets);
    saveToJsonFile(jsonOutput, "output.json");
    return 0;
}