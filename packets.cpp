#include "packets.hpp"

#include <algorithm>
#include <cstdint>

Message::Message(Opcode opcode) : opcode_(opcode) {};

ERROR::ERROR(std::vector<uint8_t> bytes) : Message(Opcode::kERROR) {
	error_code_ = static_cast<ErrorCode>(ConvertIntegerTypes(bytes.data() + 2));
	err_msg_ = reinterpret_cast<char*>(bytes.data() + 4);
};

ERROR::ERROR(ErrorCode error_code, std::string err_msg) :
	Message(Opcode::kERROR),
	error_code_(error_code),
	err_msg_(err_msg) {};

std::vector<uint8_t> ERROR::GetBinaryRepresentation() {
	uint8_t* opcode_bytes = ConvertIntegerTypes(opcode_);
	std::vector<uint8_t> bytes(opcode_bytes, opcode_bytes + 2);
	uint8_t* error_code_bytes = ConvertIntegerTypes(error_code_);
	bytes.insert(bytes.end(), error_code_bytes, error_code_bytes + 2);
	const char* err_msg_bytes = err_msg_.c_str();
	bytes.insert(bytes.end(), err_msg_bytes, err_msg_bytes + err_msg_.size() + 1);
	return bytes;
}

void ERROR::ReturnErrorToClient(int socket, sockaddr client_addr) {
	std::vector<uint8_t> buffer = GetBinaryRepresentation();
	sendto(socket, buffer.data(), buffer.size(), 0, &client_addr, sizeof(client_addr));
}

RRQWRQ::RRQWRQ(uint8_t packet[]) : Message(static_cast<Opcode>(ConvertIntegerTypes(&packet[0]))) {
	for (int i = 2; packet[i] != '\0'; i++) {
		filename_ += packet[i];
	}

	std::string mode_name;

	for (int i = 3 + filename_.size(); packet[i] != '\0'; i++) {
		mode_name += packet[i];
	}

	static const std::string kmodes[3] = { "netascii", "octet", "mail" };

	mode_ = static_cast<Mode>(distance(kmodes, find(kmodes, kmodes + sizeof(kmodes) / sizeof(std::string), mode_name)));
};