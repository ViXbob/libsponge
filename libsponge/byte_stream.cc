#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t cap) : s(""), capacity(cap), readBytes(0), writeBytes(0), input_end(false) { DUMMY_CODE(capacity); }

size_t ByteStream::write(const string &data) {
    DUMMY_CODE(data);
	int len = 0;
	if (remaining_capacity() >= data.size()) {
		len = data.size();
		s.insert(s.size(), data);
	} else {
		len = remaining_capacity();
		s.insert(s.size(), data.substr(0, len));
	}
	writeBytes += len;
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    DUMMY_CODE(len);
	if (s.size() <= len) {
		return s;
	}
    return s.substr(0, len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
	DUMMY_CODE(len);
	if (s.size() <= len) {
		readBytes += s.size();
		s = "";
	} else {
		readBytes += len;
		s.replace(0, s.size() - len, s.substr(len, s.size() - len));
		s.erase(s.size() - len, len);
	}
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
	string ret = peek_output(len);
	pop_output(len);
    return ret;
}

void ByteStream::end_input() { input_end = true; }

bool ByteStream::input_ended() const { return input_end; }

size_t ByteStream::buffer_size() const { return s.size(); }

bool ByteStream::buffer_empty() const { return s.size() == 0; }

// when the size of s is 0 and there is no more data can be input, then we enter the state of EOF
bool ByteStream::eof() const { return input_end && s.size() == 0; }

size_t ByteStream::bytes_written() const { return writeBytes; }

size_t ByteStream::bytes_read() const { return readBytes; }

size_t ByteStream::remaining_capacity() const { return capacity - s.size(); }
