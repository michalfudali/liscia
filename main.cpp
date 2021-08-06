#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#include <iostream>
#include <algorithm>

#include <cassert>
#include <cstddef>
#include<cmath>

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

void AcknowledgePacket(int socket, sockaddr client_addr, uint16_t block_number) {
	uint8_t buffer[4] = { 0 };
	buffer[1] = kACK;
	uint8_t *block_number_array = ConvertIntegerTypes(block_number);
	copy(block_number_array, block_number_array + 2, buffer + 2);
	sendto(socket, &buffer, sizeof(buffer), 0, &client_addr, sizeof(client_addr));
}

int HandleTransfer(int socket) {

	uint8_t buffer[32];
	sockaddr client_addr;
	socklen_t client_addr_size = sizeof(client_addr);

	assert(recvfrom(socket, &buffer, sizeof(buffer), 0, &client_addr, &client_addr_size) != -1);
	RRQWRQ message(buffer);

	if (message.opcode_ == Opcode::kWRQ) {
		cout << "Client is sending a file " << message.filename_ << " to us.\n";
		AcknowledgePacket(socket, client_addr, 0);
	}
	else if (message.opcode_ == Opcode::kRRQ) {
		cout << "Client asks for a file " << message.filename_ << ".\n";
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