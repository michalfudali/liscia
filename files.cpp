#include <fstream>
#include <iostream>

#include <string>
#include <vector>

std::vector<uint8_t> ReadBytesFromFile(std::string filename, int n) {

	static std::ifstream read_stream; //TODO: Close read stream
	if (!read_stream.is_open()) {
		read_stream.open(filename, std::ios_base::binary);
	}

	std::streampos starting_read_position = read_stream.tellg();

	char* char_bytes = new char[n];

	if (__CHAR_BIT__ / 8 == sizeof(uint8_t)) {
		read_stream.readsome(char_bytes, n);
	}
	else {
		std::cout << "ERROR: Incompatible machine.";
	}

	std::vector<uint8_t> bytes(char_bytes, char_bytes + read_stream.tellg() - starting_read_position);
	delete(char_bytes);

	if (bytes.size() < 512) {
		read_stream.close();
	}

	return bytes;
}
void WriteBytesToFile(std::string filename, std::vector<uint8_t> bytes) {
	static std::ofstream write_stream;
	if (!write_stream.is_open()) {
		write_stream.open(filename, std::ios_base::ate | std::ios_base::trunc | std::ios_base::binary);
	}

	write_stream.write(reinterpret_cast<char*>(bytes.data()), bytes.size());

	if (bytes.size() < 512) {
		write_stream.close();
	}
}

void WriteNetASCIIToFile(std::string filename, std::vector<uint8_t> bytes) {
	static std::ofstream write_stream;
	if (!write_stream.is_open()) {
		write_stream.open(filename, std::ios_base::ate | std::ios_base::trunc);
	}

	for (int i = 0; i < bytes.size(); i++) {
		if (bytes[i] == 13) {
			bytes.erase(bytes.begin() + i);
		}
	}

	write_stream.write(reinterpret_cast<char*>(bytes.data()), bytes.size());

	if (bytes.size() < 512) {
		write_stream.close();
	}
}
std::vector<uint8_t> ReadNetASCIIFromFile(std::string filename, int n) {

	static std::ifstream read_stream;
	if (!read_stream.is_open()) {
		read_stream.open(filename);
	}

	std::streampos starting_read_position = read_stream.tellg();

	char* char_bytes = new char[n];

	if (__CHAR_BIT__ / 8 == sizeof(uint8_t)) {
		read_stream.readsome(char_bytes, n);
	}
	else {
		std::cout << "ERROR: Incompatible machine.";
	}

	std::vector<uint8_t> bytes(char_bytes, char_bytes + read_stream.tellg() - starting_read_position);
	delete(char_bytes);

	if (bytes.size() < 512) {
		read_stream.close();
	}

	for (int i = 0; i < bytes.size(); i++) {
		if (bytes[i] == 10) {
			if (i == 0 || bytes[i - 1] != 13) {
				bytes.insert(bytes.begin() + i, 13);
			}
		}
		else if (bytes[i] == 13) {
			if (i == bytes.size() - 1 || bytes[i + 1] != 10) {
				bytes.insert(bytes.begin() + i + 1, 0);
			}
		}
	}

	return bytes;
}