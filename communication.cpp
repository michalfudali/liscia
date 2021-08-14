#include "communication.hpp" 

#include <algorithm>
#include <iostream>

#include <cassert>
#include <cstring>
#include "packets.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const size_t kMaxAckPacketSize = 4;
const size_t kMaxDataPacketSize = 516;

void AcknowledgePacket(int socket, sockaddr client_addr, uint16_t block_number) {
	std::array<uint8_t, 4> buffer = { 0 };
	buffer[1] = kACK;
	uint8_t* block_number_array = ConvertIntegerTypes(block_number);
	std::copy(block_number_array, block_number_array + 2, buffer.data() + 2);

	assert(sendto(socket, buffer.data(), sizeof(buffer), 0, &client_addr, sizeof(client_addr)) != -1);
}

void SendData(int socket, sockaddr client_addr, uint16_t block_number, std::vector<uint8_t> data) {
	std::vector<uint8_t> bytes;
	bytes.reserve(4 + data.size());
	bytes.resize(4, 0);

	uint8_t* block_number_bytes = ConvertIntegerTypes(block_number);
	bytes[1] = Opcode::kDATA;
	bytes[2] = *(block_number_bytes);
	bytes[3] = *(block_number_bytes + 1);
	bytes.insert(bytes.end(), data.begin(), data.end());

	assert(sendto(socket, bytes.data(), bytes.size(), 0, &client_addr, sizeof(client_addr)) != -1);
}

std::optional<std::vector<uint8_t>> SafelyReceivePacket(const int socket, const sockaddr expected_addr, size_t max_packet_size) {	
	sockaddr received_addr;
	socklen_t received_addr_len = sizeof(received_addr);
	std::vector<uint8_t> bytes(max_packet_size);

	static ERROR error(ErrorCode::kUnknownTransferID, "You are not the allowed one.");

	while (true) {
		ssize_t n = recvfrom(socket, bytes.data(), max_packet_size, 0, &received_addr, &received_addr_len);

		if (n == -1) {
			if (errno == 11)
				return std::nullopt;
			else
				assert(false);
		}
		else {
			//Handle packets of another TID
			if (memcmp(received_addr.sa_data, expected_addr.sa_data, 14) != 0) {
				error.ReturnErrorToClient(socket, received_addr);
				continue;
			}

			bytes.resize(n);
			return bytes;
		}
	}
}

std::optional<std::vector<uint8_t>> WaitAndExtractData(const int socket, const sockaddr expected_addr, const uint16_t expected_block_number) {
	std::optional<std::vector<uint8_t>> bytes; 
		
	do {
		bytes = SafelyReceivePacket(socket, expected_addr, kMaxDataPacketSize);

		if (!bytes) break;

		uint16_t opcode = ConvertIntegerTypes(bytes.value().data());

		if (opcode == Opcode::kDATA) {
			if (ConvertIntegerTypes(bytes.value().data() + 2) == expected_block_number) {
				return std::vector<uint8_t>(bytes.value().data() + 4, bytes.value().data() + bytes.value().size());
			}
		} else if (opcode == Opcode::kERROR) {
			return bytes;
		}
	} while (bytes);

	return bytes;
}
