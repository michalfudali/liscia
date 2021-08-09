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

void WriteCharactersToFile(string filename, vector<uint8_t> bytes) {
	static ofstream write_stream;
	if (!write_stream.is_open()) {
		write_stream.open(filename, std::ios_base::ate | ios_base::trunc);
	}

	write_stream.write(reinterpret_cast<char*>(bytes.data()), bytes.size());

	if (bytes.size() < 512) {
		write_stream.close();
	}
}
vector<uint8_t> ReadCharactersFromFile(string filename, int n) {

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

	return bytes;
}