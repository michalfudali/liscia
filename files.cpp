#include <fstream>
#include <iostream>

#include "main.hpp"

using namespace std;

vector<uint8_t> ReadBytesFromFile(string filename, int n) {

	static ifstream read_stream; //TODO: Close read stream
	if (!read_stream.is_open()) {
		read_stream.open(filename, std::ios_base::binary);
	}

	streampos starting_read_position = read_stream.tellg();

	char* char_bytes = new char[n];

	if (__CHAR_BIT__ / 8 == sizeof(uint8_t)) {
		read_stream.readsome(char_bytes, n);
	}
	else {
		cout << "ERROR: Incompatible machine.";
	}

	vector<uint8_t> bytes(char_bytes, char_bytes + read_stream.tellg() - starting_read_position);
	delete(char_bytes);

	if (bytes.size() < 512) {
		read_stream.close();
	}

	return bytes;
}
void WriteBytesToFile(string filename, vector<uint8_t> bytes) {
	static ofstream write_stream;
	if (!write_stream.is_open()) {
		write_stream.open(filename, std::ios_base::ate | ios_base::trunc | ios_base::binary);
	}

	write_stream.write(reinterpret_cast<char*>(bytes.data()), bytes.size());

	if (bytes.size() < 512) {
		write_stream.close();
	}
}

void WriteNetASCIIToFile(string filename, vector<uint8_t> bytes) {
	static ofstream write_stream;
	if (!write_stream.is_open()) {
		write_stream.open(filename, std::ios_base::ate | ios_base::trunc);
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
vector<uint8_t> ReadNetASCIIFromFile(string filename, int n) {

	static ifstream read_stream;
	if (!read_stream.is_open()) {
		read_stream.open(filename);
	}

	streampos starting_read_position = read_stream.tellg();

	char* char_bytes = new char[n];

	if (__CHAR_BIT__ / 8 == sizeof(uint8_t)) {
		read_stream.readsome(char_bytes, n);
	}
	else {
		cout << "ERROR: Incompatible machine.";
	}

	vector<uint8_t> bytes(char_bytes, char_bytes + read_stream.tellg() - starting_read_position);
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