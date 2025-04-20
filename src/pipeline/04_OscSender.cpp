#include "04_OscSender.hpp"
#include <osc/OscOutboundPacketStream.h>
#include <ip/UdpSocket.h>
#include <ip/IpEndpointName.h>
#include <iostream>
#include <map>
#include <memory>

OscSender::OscSender(const std::string& host, int port)
    : host_(host), port_(port), socket_(nullptr), buffer_(OUTPUT_BUFFER_SIZE)
{
    std::cout << "[OscSender] Constructing with host: " << host << ", port: " << port << std::endl;
    initializeSocket();
}

OscSender::OscSender(const char* ip, int port)
    : OscSender(std::string(ip), port) {}

OscSender::~OscSender() {
    // No explicit delete needed, unique_ptr handles it.
    // if (socket_) {
    //     delete socket_;
    //     socket_ = nullptr;
    // }
}

// ITransportSink interface implementation
bool OscSender::send(const void* data, size_t size) {
    // Not implemented: use sendMessage/sendBundle for typed OSC
    std::cerr << "[OscSender] Raw send() not implemented. Use sendMessage/sendBundle." << std::endl;
    return false;
}

void OscSender::close() {
    socket_.reset(); // Use reset() for unique_ptr
    std::cout << "[OscSender] Socket closed." << std::endl;
}

void OscSender::initializeSocket() {
    std::cout << "[OscSender] Initializing socket for host: " << host_ << ", port: " << port_ << std::endl;
    try {
        IpEndpointName destination(host_.c_str(), port_);
        socket_ = std::make_unique<UdpTransmitSocket>(destination);
        std::cout << "[OscSender] Socket initialized successfully." << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "[OscSender] ERROR: Failed to initialize socket: " << e.what() << std::endl;
        socket_.reset(); // Ensure socket is null on failure
    }
}

void OscSender::setHost(const std::string& host, int port) {
    std::cout << "[OscSender] setHost called with host: " << host << ", port: " << port << std::endl;
    host_ = host;
    port_ = port;
    initializeSocket();
}

bool OscSender::isInitialized() const {
    // Check unique_ptr for validity
    return socket_ != nullptr;
}

void OscSender::sendBundle(const std::map<std::string, float>& messages, const std::string& baseAddress) {
    // Check unique_ptr
    if (!socket_) {
        std::cerr << "[OscSender] ERROR: Socket not initialized. Cannot send OSC bundle." << std::endl;
        return;
    }
    std::cout << "[OscSender] [STUB] Would send OSC bundle with base address: " << baseAddress << ", message count: " << messages.size() << std::endl;
    // [STUB] OSC bundle sending is disabled until OSC library is available.
}

void OscSender::sendMessage(const std::string& address, float value) {
    if (!socket_) {
        std::cerr << "[OscSender] ERROR: Socket not initialized. Cannot send message to " << address << std::endl;
        return;
    }
    // std::cout << "[OscSender] [STUB] Would send OSC message to " << address << " with value " << value << std::endl;
    
    try {
        osc::OutboundPacketStream p(buffer_.data(), buffer_.size());
        p << osc::BeginMessage(address.c_str()) << value << osc::EndMessage;
        socket_->Send(p.Data(), p.Size());
        // Optional: Log success
        // std::cout << "[OscSender] Sent: " << address << " = " << value << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "[OscSender] ERROR sending message to " << address << ": " << e.what() << std::endl;
        // Consider re-initializing socket? Or maybe just log.
    }
}

void OscSender::sendMessages(const std::vector<OscMessage>& messages) {
    std::cout << "[OscSender] Sending batch of " << messages.size() << " OSC messages." << std::endl;
    for (const auto& msg : messages) {
        if (!msg.address.empty() && !msg.values.empty()) {
            for (float value : msg.values) {
                std::cout << "  [OscSender] Batch message: " << msg.address << " = " << value << std::endl;
                sendMessage(msg.address, value);
            }
        }
    }
}

// Implement ITransportSink methods
void OscSender::sendOscMessage(const OscMessage& message)
{
    sendMessages({message}); // Reuse existing method
}

void OscSender::updateTarget(const std::string& target, int port)
{
    setHost(target, port); // Reuse existing method
}
