#pragma once

#include "communication.hpp"

#include <string>
#include <vector>

class Message {
public:
	const Opcode opcode_;
	Message(Opcode opcode);
};

class ERROR : public Message {
public:
	ErrorCode error_code_;
	std::string err_msg_;

	ERROR(std::vector<uint8_t> bytes);

	ERROR(ErrorCode error_code, std::string err_msg);

	void ReturnErrorToClient(int socket, sockaddr client_addr);
	std::vector<uint8_t> GetBinaryRepresentation();
};

class RRQWRQ : public Message {
public:
	std::string filename_;
	Mode mode_;

	RRQWRQ(uint8_t packet[]);
};