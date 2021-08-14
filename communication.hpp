#pragma once

#include <optional>
#include <vector>

#include <cstdint>

#include <sys/socket.h>

inline uint16_t ConvertIntegerTypes(uint8_t bytes[2]) {
	return (bytes[0] << 8) | bytes[1];
}

inline uint8_t* ConvertIntegerTypes(uint16_t value) {
	uint8_t* bytes = new uint8_t[2];
	bytes[0] = value >> 8;
	bytes[1] = value & 0xff;
	return bytes;
}

extern const size_t kMaxAckPacketSize;
extern const size_t kMaxDataPacketSize;
extern socklen_t kSockaddrSize;

enum Opcode : uint16_t {
	kRRQ = 1,
	kWRQ = 2,
	kDATA = 3,
	kACK = 4,
	kERROR = 5
};

enum ErrorCode : uint16_t {
	kNotDefined,
	kFileNotFound,
	kAccessViolation,
	kDiskFull = 3,
	kAllocationExceeded = 3,
	kIllegalOperation,
	kUnknownTransferID,
	kFileAlreadyExists,
	kNoSuchUser
};

enum class Mode {
	knetascii,
	koctet,
	kmail
};

void AcknowledgePacket(int socket, sockaddr client_addr, uint16_t block_number);
void SendData(int socket, sockaddr client_addr, uint16_t block_number, std::vector<uint8_t> data);

std::optional<std::vector<uint8_t>> SafelyReceivePacket(const int socket, const sockaddr expected_addr, size_t max_packet_size);
std::optional<std::vector<uint8_t>> WaitAndExtractData(const int socket, const sockaddr expected_addr, const uint16_t expected_block_number);