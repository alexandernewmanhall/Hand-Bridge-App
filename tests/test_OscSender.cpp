#include "gtest/gtest.h"
#include "pipeline/04_OscSender.hpp"
#include "osc/OscMessage.hpp"
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// Simple UDP receiver for test
class UdpReceiver {
public:
    UdpReceiver(int port) : port_(port), running_(false) {}
    ~UdpReceiver() { stop(); }
    void start() {
        running_ = true;
        thread_ = std::thread([this]() { run(); });
    }
    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }
    // Wait for a packet, returns true if received
    bool waitForPacket(std::string& out, int timeoutMs = 500) {
        using namespace std::chrono;
        auto start = steady_clock::now();
        while (duration_cast<milliseconds>(steady_clock::now() - start).count() < timeoutMs) {
            if (!buffer_.empty()) {
                out = buffer_;
                buffer_.clear();
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return false;
    }
private:
    void run() {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
#endif
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(port_);
        bind(sock, (sockaddr*)&addr, sizeof(addr));
        char buf[1024];
        // Set socket to non-blocking mode
#ifdef _WIN32
        u_long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
#else
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
        while (running_) {
#ifdef _WIN32
            int len = recv(sock, buf, sizeof(buf), 0);
#else
            int len = recv(sock, buf, sizeof(buf), 0);
#endif
            if (len > 0) buffer_.assign(buf, len);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
    }
    int port_;
    std::atomic<bool> running_;
    std::thread thread_;
    std::string buffer_;
};

TEST(OscSenderTest, SendMessage) {
    int testPort = 9000;
    UdpReceiver receiver(testPort);
    receiver.start();
    OscSender sender("127.0.0.1", testPort);
    ASSERT_TRUE(sender.isInitialized());
    sender.sendMessage("/test/address", 42.0f);
    std::string packet;
    ASSERT_TRUE(receiver.waitForPacket(packet));
    // Check for OSC address in packet (not full decode)
    ASSERT_NE(packet.find("/test/address"), std::string::npos);
    receiver.stop();
}

TEST(OscSenderTest, SetHostAndSend) {
    int port1 = 9001, port2 = 9002;
    UdpReceiver recv1(port1), recv2(port2);
    recv1.start(); recv2.start();
    OscSender sender("127.0.0.1", port1);
    sender.sendMessage("/a", 1.0f);
    std::string pkt1, pkt2;
    ASSERT_TRUE(recv1.waitForPacket(pkt1));
    sender.setHost("127.0.0.1", port2);
    sender.sendMessage("/b", 2.0f);
    ASSERT_TRUE(recv2.waitForPacket(pkt2));
    ASSERT_NE(pkt1.find("/a"), std::string::npos);
    ASSERT_NE(pkt2.find("/b"), std::string::npos);
    recv1.stop(); recv2.stop();
}

TEST(OscSenderTest, SendMessages) {
    int testPort = 9003;
    UdpReceiver receiver(testPort);
    receiver.start();
    OscSender sender("127.0.0.1", testPort);
    std::vector<OscMessage> msgs = {
        {"/msg/1", {1.0f}},
        {"/msg/2", {2.0f}}
    };
    sender.sendMessages(msgs);
    std::string pkt;
    ASSERT_TRUE(receiver.waitForPacket(pkt));
    ASSERT_TRUE(pkt.find("/msg/1") != std::string::npos || pkt.find("/msg/2") != std::string::npos);
    receiver.stop();
}

TEST(OscSenderTest, SendBundle) {
    int testPort = 9004;
    UdpReceiver receiver(testPort);
    receiver.start();
    OscSender sender("127.0.0.1", testPort);
    std::map<std::string, float> bundle = {
        {"/b1", 11.0f},
        {"/b2", 22.0f}
    };
    sender.sendBundle(bundle);
    std::string pkt;
    ASSERT_TRUE(receiver.waitForPacket(pkt));
    ASSERT_TRUE(pkt.find("/b1") != std::string::npos || pkt.find("/b2") != std::string::npos);
    receiver.stop();
}
