#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include<unistd.h>

#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>

#include <cassert>
#include <cstddef>
#include<cmath>
#include <array>

using namespace std; //REMOVEME

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

const string kmodes[3] = { "netascii", "octet", "mail" };

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
	Opcode opcode_;
	string filename_;
	Mode mode_;

	RRQWRQ(uint8_t packet[]) {
		opcode_ = static_cast<Opcode>(ConvertIntegerTypes(&packet[0]));

		for (int i = 2; packet[i] != '\0'; i++) {
			filename_ += packet[i];
		}

		string mode_name;

		for (int i = 3 + filename_.size(); packet[i] != '\0'; i++) {
			mode_name += packet[i];
		}

		mode_ = static_cast<Mode>(distance(kmodes, find(kmodes, kmodes + sizeof(kmodes)/sizeof(string), mode_name)));
	};
};

vector<uint8_t> ReadNBytesFromFile(string filename, int n) {

	static ifstream read_stream;
	if (!read_stream.is_open()) {
		read_stream.open(filename);
	}

	streampos starting_read_position = read_stream.tellg();

	char* char_bytes = new char[n];

	if (__CHAR_BIT__ / 8 == sizeof(uint8_t)) {
		read_stream.readsome(char_bytes, n);
	} else {
		cout << "ERROR: Incompatible machine.";
	}

	vector<uint8_t> bytes(char_bytes, char_bytes + read_stream.tellg() - starting_read_position);
	delete(char_bytes);

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

int HandleTransfer(int socket) {

	uint8_t buffer[512]; //TODO: Precise me
	sockaddr received_addr;
	socklen_t addr_size = sizeof(sockaddr);

	assert(recvfrom(socket, &buffer, sizeof(buffer), 0, &received_addr, &addr_size) != -1);

	const sockaddr kclient_addr = received_addr;

	RRQWRQ message(buffer);
	vector<uint8_t> file_bytes;

	uint16_t block_number = 1;

	if (message.opcode_ == Opcode::kWRQ) {
		cout << "Client is sending a file " << message.filename_ << " to us.\n";
		AcknowledgePacket(socket, kclient_addr, 0);
	}
	else if (message.opcode_ == Opcode::kRRQ) {
		cout << "Client asks for a file " << message.filename_ << ".\n";

		do {
			file_bytes = ReadNBytesFromFile(message.filename_, 512);
			SendData(socket, kclient_addr, block_number, file_bytes);
			block_number++;

			sleep(1);
		} while (file_bytes.size() >= 512);
	}

	shutdown(socket, SHUT_RDWR);
}

int main(int const argc, char const *argv[]) {
	
	sockaddr_in addr_in = { AF_INET, htons(69), INADDR_ANY };
	sockaddr *paddr = reinterpret_cast<sockaddr*>(&addr_in);

	pollfd fds[1];
	fds[0].fd = socket(AF_INET, SOCK_DGRAM, 0);
	assert(fds[0].fd != -1);

	fds[0].events = POLLIN;
	assert(bind(fds[0].fd, paddr, sizeof(*paddr)) != -1);
	
	//while(1)
	{
		cout << "Waiting for connection...\n";
		poll(fds, sizeof(fds) / sizeof(fds[0]), -1);
		cout << "Connected.\n";

		HandleTransfer(fds[0].fd);
	}

	return 0;
}