#pragma once
// Renamed: 04_OscSender
#include <string>
#include <vector>
#include <map>
#include "transport/osc/OscMessage.hpp"
#include "core/interfaces/ITransportSink.hpp"
#include <memory>

// Forward-declare the oscpack socket type
class UdpTransmitSocket;

class OscSender : public ITransportSink {
public:
    OscSender(const std::string& host, int port);
    OscSender(const char* ip, int port);
    ~OscSender();

    void setHost(const std::string& host, int port);
    bool isInitialized() const;
    void sendMessage(const std::string& address, float value);
    void sendMessages(const std::vector<OscMessage>& messages);
    void sendBundle(const std::map<std::string, float>& messages, const std::string& baseAddress = "");

    // ITransportSink interface
    bool send(const void* data, size_t size) override; // Send raw bytes (stub for now)
    void sendOscMessage(const OscMessage& message) override;
    void updateTarget(const std::string& target, int port) override;
    void close() override;

public:
    std::string getHost() const { return host_; }
    int getPort() const { return port_; }
private:
    void initializeSocket();
    std::string host_;
    int port_;
    // Use unique_ptr for the socket
    std::unique_ptr<UdpTransmitSocket> socket_;
    static constexpr size_t OUTPUT_BUFFER_SIZE = 4096;
    std::vector<char> buffer_;
};
