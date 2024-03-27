#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <thread>
#include <deque>
#include <vector>

using namespace std;

int lineCounter = 0;
vector<thread> threads;

void directoryNavigator(string dirPath);
void fileReader(string filePath);

bool fillDeque(deque<byte> &fourBytes, ifstream &file);
void popDeque(deque<byte>& fourBytes, int times);

int isUTF8(deque<byte> &fourBytes);
bool isCorrectNonFirstByte(byte& b);

int main(int argc, char* argv[])
{
	if (argc == 2) {                //check if it is 2 when launching in command line
		directoryNavigator(argv[1]);
		for (auto& thread : threads) thread.join();
		cout << lineCounter;
	}
	else {
		cout << "Incorrect number of arguments\n";
	}
	return 0;
}

void directoryNavigator(string dirPath) {
    for (auto& entry : filesystem::directory_iterator(filesystem::path(dirPath))) {
		if (entry.is_directory()) {
			directoryNavigator(entry.path().string());
		}
		else {
			threads.push_back(thread(fileReader, entry.path().string()));
		}
    }
}

void fileReader(string filePath)
{
	bool userWarned = false;
	deque<byte> fourBytes;
	int byteCount;

	try {
		ifstream file(filePath, ios::binary);
		lineCounter++;
		while (fillDeque(fourBytes, file) || fourBytes.size() != 0) {
			if (fourBytes[0] == byte{ 0b00001101 } && fourBytes[1] == byte{ 0b00001010 }) {
				fourBytes.pop_front();
				fourBytes.pop_front();
				lineCounter++;
				continue;
			}
			byteCount = 1;

			if (!userWarned) {
				byteCount = isUTF8(fourBytes);
				if (byteCount == 0) {
					string message = "WARNING: file at path " + filePath + " is not encoded in UTF-8\n";
					cout << message;
					byteCount = 1;
					userWarned = true;
				}
			}
			popDeque(fourBytes, byteCount);
		}
	}
	catch (const ifstream::failure& e) { 
		string message = "Could not open file " + filePath + "\n";
		cout << message;
	}
}


int isUTF8(deque<byte>& fourBytes) {
	if (fourBytes.size() > 0 && (fourBytes[0] & byte { 0b10000000 }) == byte{ 0b00000000 }) {
		return 1;
	}
	if (fourBytes.size() > 1 && (fourBytes[0] & byte { 0b11100000 }) == byte{ 0b11000000 }) {
		return isCorrectNonFirstByte(fourBytes[1]) ?
			2 : 0;
	}
	if (fourBytes.size() > 2 && (fourBytes[0] & byte { 0b11110000 }) == byte{ 0b11100000 }) {
		return isCorrectNonFirstByte(fourBytes[1])
			&& isCorrectNonFirstByte(fourBytes[2]) ?
			3 : 0;
	}
	if (fourBytes.size() > 3 && (fourBytes[0] & byte { 0b11111000 }) == byte{ 0b11110000 }) {
		return isCorrectNonFirstByte(fourBytes[1])
			&& isCorrectNonFirstByte(fourBytes[2])
			&& isCorrectNonFirstByte(fourBytes[3]) ?
			4 : 0;
	}
	return 0;
}

bool isCorrectNonFirstByte(byte& b) {
	return (b & byte { 0b11000000 }) == byte{ 0b10000000 };
}

bool fillDeque(deque<byte> &fourBytes, ifstream &file) {
	byte byteBuffer;
	for (int i = fourBytes.size(); i < 4; i++) {
		if (!file.read(reinterpret_cast<char*>(&byteBuffer), sizeof byteBuffer)) {
			return false;
		}
		fourBytes.push_back(byteBuffer);
	}
	return true;
}

void popDeque(deque<byte>& fourBytes, int times) {
	for (int i = 0; i < times; i++) {
		fourBytes.pop_front();
	}
}
