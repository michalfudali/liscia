#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>

#include "main.hpp"

#include <iostream>
#include <algorithm>
#include <fstream>

#include <cassert>
#include <cstddef>
#include <cmath>

using namespace std; //REMOVEME

static socklen_t ksockaddr_size = sizeof(sockaddr);

enum Opcode {
	kRRQ = 1,
	kWRQ = 2,
	kDATA = 3,
	kACK = 4,
	kERROR = 5
};

enum class Mode {
	knetascii,
	koctet,
	kmail
};

inline uint16_t ConvertIntegerTypes(uint8_t bytes[2]) {
	return (bytes[0] << 8) | bytes[1];
}

inline uint8_t* ConvertIntegerTypes(uint16_t opcode) {
	uint8_t* bytes = new uint8_t[2];
	bytes[0] = opcode >> 8;
	bytes[1] = opcode & 0xff;
	return bytes;
}

class RRQWRQ {
public:
	const Opcode opcode_;
	string filename_;
	Mode mode_;

	RRQWRQ(uint8_t packet[]) : opcode_(static_cast<Opcode>(ConvertIntegerTypes(&packet[0]))) {
		for (int i = 2; packet[i] != '\0'; i++) {
			filename_ += packet[i];
		}

		string mode_name;

		for (int i = 3 + filename_.size(); packet[i] != '\0'; i++) {
			mode_name += packet[i];
		}

		static const string kmodes[3] = { "netascii", "octet", "mail" };

		mode_ = static_cast<Mode>(distance(kmodes, find(kmodes, kmodes + sizeof(kmodes)/sizeof(string), mode_name)));
	};
};

//class Data {
//	static const Opcode opcode_ = Opcode::kDATA;
//};

vector<uint8_t> SafelyReceivePacket(const int socket, const sockaddr expected_addr, size_t max_packet_size) {
	vector<uint8_t> bytes(max_packet_size);
	sockaddr received_addr;
	ssize_t n = recvfrom(socket, bytes.data(), max_packet_size, 0, &received_addr, &ksockaddr_size);
	assert(n != -1);
	bytes.resize(n);
	//assert(received_addr.sa_data == expected_addr.sa_data); TODO: IMPLEMENT SENDER CHECKING
	return bytes;
}


void AcknowledgePacket(int socket, sockaddr client_addr, uint16_t block_number) {
	uint8_t buffer[4] = { 0 };
	buffer[1] = kACK;
	uint8_t* block_number_array = ConvertIntegerTypes(block_number);
	copy(block_number_array, block_number_array + 2, buffer + 2);
	sendto(socket, &buffer, sizeof(buffer), 0, &client_addr, sizeof(client_addr));
}

void SendData(int socket, sockaddr client_addr, uint16_t block_number, vector<uint8_t> data) {
	vector<uint8_t> bytes;
	bytes.reserve(4 + data.size());
	bytes.resize(4, 0);

	uint8_t* block_number_bytes = ConvertIntegerTypes(block_number);
	bytes[1] = Opcode::kDATA;
	bytes[2] = *(block_number_bytes);
	bytes[3] = *(block_number_bytes + 1);
	bytes.insert(bytes.end(), data.begin(), data.end());

	assert(sendto(socket, bytes.data(), bytes.size(), 0, &client_addr, sizeof(client_addr)) != -1);
	cout << "The data has been sent.";
}

void WaitForAcknowledgment(const int socket, const sockaddr expected_addr, const uint16_t expected_block_number) {
	vector<uint8_t> buffer = SafelyReceivePacket(socket, expected_addr, 4);

	if (ConvertIntegerTypes(buffer.data()) == Opcode::kACK) {
		if (ConvertIntegerTypes(&buffer[2]) == expected_block_number) {
			cout << "Received Acknowledgment for block no. " << expected_block_number << ".\n";
		}
	}
}

vector<uint8_t> WaitForData(const int socket, const sockaddr expected_addr, const uint16_t expected_block_number) {
	vector<uint8_t> bytes = SafelyReceivePacket(socket, expected_addr, 516);

	if (ConvertIntegerTypes(bytes.data()) == Opcode::kDATA) {
		if (ConvertIntegerTypes(bytes.data() + 2) == expected_block_number) {
			cout << "Received Data block no. " << expected_block_number << ".\n";
			return vector<uint8_t>(bytes.data() + 4, bytes.data() + bytes.size());
		}
	}
}

int HandleTransfer(int socket) {

	uint8_t connection_init_buffer[132]; //TODO: Precise me, e.g. 4 bytes + 2 * 64 * sizeof(string)
	sockaddr received_addr;

	assert(recvfrom(socket, &connection_init_buffer, sizeof(connection_init_buffer), 0, &received_addr, &ksockaddr_size) != -1);

	const sockaddr kclient_addr = received_addr;

	RRQWRQ message(connection_init_buffer);
	vector<uint8_t> file_bytes;

	uint16_t block_number = 1;

	if (message.mode_ == Mode::koctet) {
		if (message.opcode_ == Opcode::kWRQ) {
			cout << "Client is sending a file " << message.filename_ << " to us.\n";
			cout << "Using octet mode.\n";

			AcknowledgePacket(socket, kclient_addr, 0);
			file_bytes.reserve(516);
			do {
				file_bytes = WaitForData(socket, kclient_addr, block_number);

				WriteBytesToFile(message.filename_, file_bytes);

				AcknowledgePacket(socket, kclient_addr, block_number);

				block_number++;
			} while (file_bytes.size() == 512);

		}
		else if (message.opcode_ == Opcode::kRRQ) {
			cout << "Client asks for a file " << message.filename_ << ".\n";
			do {
				file_bytes = ReadBytesFromFile(message.filename_, 512);
				SendData(socket, kclient_addr, block_number, file_bytes);
				WaitForAcknowledgment(socket, kclient_addr, block_number);

				block_number++;
			} while (file_bytes.size() == 512);
		}
	}
	else if (message.mode_ == Mode::knetascii) {
		if (message.opcode_ == Opcode::kWRQ) {
			cout << "Client is sending a file " << message.filename_ << " to us.\n";
			cout << "Using netascii mode.\n";
			AcknowledgePacket(socket, kclient_addr, 0);
			file_bytes.reserve(516);
			do {
				file_bytes = WaitForData(socket, kclient_addr, block_number);

				WriteCharactersToFile(message.filename_, file_bytes);

				AcknowledgePacket(socket, kclient_addr, block_number);

				block_number++;
			} while (file_bytes.size() == 512);

		}
		else if (message.opcode_ == Opcode::kRRQ) {
			cout << "Client asks for a file " << message.filename_ << ".\n";
			do {
				file_bytes = ReadCharactersFromFile(message.filename_, 512);
				SendData(socket, kclient_addr, block_number, file_bytes);
				WaitForAcknowledgment(socket, kclient_addr, block_number);

				block_number++;
			} while (file_bytes.size() == 512);
		}
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

	fds[0].events = POLLIN;
	assert(bind(fds[0].fd, paddr, sizeof(*paddr)) != -1);
	
	{
		cout << "Waiting for connection...\n";
		poll(fds, sizeof(fds) / sizeof(fds[0]), -1);
		cout << "Connected.\n";

		HandleTransfer(fds[0].fd);
	}

	return 0;
}