#include "communication.hpp"
#include "packets.hpp"

#include "files.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>

#include <iostream>
#include <algorithm>
#include <fstream>

#include "cstring"
#include <cassert>
#include <cstddef>
#include <cmath>

using namespace std; //REMOVEME

const size_t kNumberOfDataRetries = 6;
const size_t kNumberOfAckRetries = 6;

int HandleTransfer(int socket) {
	
	uint8_t connection_init_buffer[132]; //TODO: Precise me, e.g. 4 bytes + 2 * 64 * sizeof(string)
	sockaddr received_addr;
	socklen_t received_addr_len = sizeof(received_addr);

	assert(recvfrom(socket, &connection_init_buffer, sizeof(connection_init_buffer), 0, &received_addr, &received_addr_len) != -1);
	perror("RECV");
	const sockaddr kClientAddr = received_addr;

	RRQWRQ message(connection_init_buffer);
	std::optional<std::vector<uint8_t>> bytes = std::vector<uint8_t>();

	uint16_t block_number = 1;

	message.mode_ == Mode::koctet ? cout << "Using octet mode.\n" : cout << "Using netascii mode.\n";

	if (message.opcode_ == Opcode::kWRQ) {
		cout << "Client is sending a file " << message.filename_ << " to us.\n";

		AcknowledgePacket(socket, kClientAddr, 0);
		bytes.value().reserve(kMaxDataPacketSize);
		do {
			int i = 0;
			int t = 1;
			do {
				bytes = WaitAndExtractData(socket, kClientAddr, block_number);

				if (!bytes) {
					AcknowledgePacket(socket, kClientAddr, block_number - 1);
					sleep(t);
					t *= 2;
					i++;
				}
				else {
					if (bytes.value().size() > 4) {
						if (ConvertIntegerTypes(bytes.value().data()) == Opcode::kERROR) {
							return -2;
						}
					}
					AcknowledgePacket(socket, kClientAddr, block_number);
				}
			} while (!bytes && i < kNumberOfAckRetries);

			if (i == kNumberOfAckRetries) {
				cout << "Timeout expired while waiting for data.\n";
				return -1;
			}

			assert(bytes);

			cout << "Received Data block no. " << block_number << ".\n";
			if (message.mode_ == Mode::koctet) {
				WriteBytesToFile(message.filename_, bytes.value());
			}
			else {
				WriteNetASCIIToFile(message.filename_, bytes.value());
			}

			block_number++;
		} while (bytes.value().size() == kMaxDataPacketSize - 4);
	
	}
	else if (message.opcode_ == Opcode::kRRQ) {
		cout << "Client asks for a file " << message.filename_ << ".\n";
		std::optional<std::vector<uint8_t>> file_bytes = std::vector<uint8_t>();
		do {
			if (message.mode_ == Mode::koctet) {
				file_bytes = ReadBytesFromFile(message.filename_, kMaxDataPacketSize - 4);
			}
			else {
				file_bytes = ReadNetASCIIFromFile(message.filename_, kMaxDataPacketSize - 4);
			}

			int i = 0;
			int t = 1;
			do {
				SendData(socket, kClientAddr, block_number, file_bytes.value());

				do {
					bytes = SafelyReceivePacket(socket, kClientAddr, kMaxAckPacketSize);

					if (bytes && bytes.value().size() >= 2) {
						uint16_t opcode = ConvertIntegerTypes(bytes.value().data());
						if (opcode == Opcode::kERROR) {
							return -2; //TODO: Replace with error code const
						}
						else if (opcode == Opcode::kACK && bytes.value().size() == 4) {
							if (ConvertIntegerTypes(bytes.value().data() + 2) == block_number) {
								break;
							}
						}
					}

				} while (bytes);

				if (!bytes) {
					sleep(t);
					t *= 2;
					i++;
				}
				else {
					break;
				}
			} while (i < kNumberOfDataRetries + 1);

			if (i >= kNumberOfAckRetries) {
				cout << "Timeout expired while waiting for acknowledgement.\n";
				return -1; //TODO: Replace with timeout code const
			}

			cout << "Received Acknowledgment for block no. " << block_number << ".\n";

			block_number++;
		} while (file_bytes.value().size() == (kMaxDataPacketSize - 4));
	}

	shutdown(socket, SHUT_RDWR);
}

int main(int const argc, char const *argv[]) {
	
	chdir("/tftp/");

	sockaddr_in addr_in = { AF_INET, htons(69), INADDR_ANY };
	sockaddr *paddr = reinterpret_cast<sockaddr*>(&addr_in);

	pollfd fds[1];
	fds[0].fd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(fds[0].fd != -1);

	timeval timeout;
	timeout.tv_usec = 5;
	assert(setsockopt(fds[0].fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) >= 0);

	fds[0].events = POLLIN;
	assert(bind(fds[0].fd, paddr, sizeof(*paddr)) != -1);

	{
		cout << "Waiting for connection...\n";
		poll(fds, sizeof(fds) / sizeof(fds[0]), -1);
		cout << "Connected.\n";

		return HandleTransfer(fds[0].fd);
	}
}

