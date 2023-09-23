#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), _map(), _index(0), end_index(-1) {}


//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    DUMMY_CODE(data, index, eof);
	if (eof) {
		end_index = index + data.size();
	}
	// we will ignore the substring which cause streamReassembler to exceed its capacity.
	string subdata = data;
	if (index + data.size() - _output.bytes_read() > _capacity) {
		subdata = data.substr(0, _output.bytes_read() + _capacity - index);
	}
	if (_index >= index && data.size() + index >_index) {
		size_t len;
		len = _output.write(subdata.substr(_index - index, subdata.size() - _index + index ));
		_index += len;
		while(true) {
			if (_map.empty()) {
				break;
			}
			auto ptr = _map.begin();
			if (ptr->first <= _index) {
				size_t bytes;
				//if the size of data is smaller then the difference between _index and ptr->first, then the data isn't need to write.
				if (ptr->second.size() > _index - ptr->first) {
					bytes =	_output.write(ptr->second.substr(_index - ptr->first, ptr->second.size() - _index + ptr->first));
					_index += bytes;
				}
				_map.erase(ptr);
			} else {
				break;
			}
		}
	} else {
		// we should add the data into map or not.
		if (subdata.size() > 0) {
			if (_map.empty()) {
				_map[index] = subdata;
			} else {
				bool put = true;
				// In _map, we don't want the subset relationship exists.
				for (auto ptr = _map.begin(); ptr != _map.end();) {
					// if the data is the subset of ptr->second, we don't include it
					if (ptr->first <= index && index + subdata.size() <= ptr->first + ptr->second.size()) {
						put = false;
					}
					// if the ptr->second is the subset, we erase it.
					if (index <= ptr->first && ptr->first + ptr->second.size() <= index + subdata.size()) {
						auto nextPtr = _map.erase(ptr);
						if (nextPtr == _map.end()) {
							break;
						}
						//We can't let nextPtr--, because ptr maybe _map.begin() and nextPtr-- means nothing. So we had to behave strangly like this.
						ptr = nextPtr;
					} else {
						ptr++;
					}
				}
				if (put) {
					_map[index] = subdata;
				}
			}
		}
	}
	if (_index == static_cast<size_t>(end_index)) {
		_output.end_input();
	}
}

size_t StreamReassembler::unassembled_bytes() const { 
	size_t len = 0;
	for (auto ptr = _map.begin(); ptr != _map.end(); ptr++) {

		len += ptr->second.size();
	}
	// calculating the overlap bytes. And there have no subset relationship.
	size_t overlapBytes = 0;
	if (_map.size() == 1) {
		return len;
	}
	for (auto ptr = _map.begin(); ptr != _map.end();) {
		for (auto itr = ++ptr; itr != _map.end(); itr++) {
			if(ptr->first < itr->first && ptr->first + ptr->second.size() > itr->first && ptr->first + ptr->second.size() < itr->first + itr->second.size())  {
				overlapBytes += ptr->first + ptr->second.size() - itr->first;
			} else if (ptr->first < itr->first && ptr->first + ptr->second.size() > itr->first + itr->second.size()) {
				overlapBytes += itr->second.size();
			} else if (ptr->first > itr->first && itr->first + itr->second.size() > ptr->first) {
				overlapBytes += itr->first + itr->second.size() - ptr->first;
			}
		}
	}
	return len - overlapBytes;
}

bool StreamReassembler::empty() const { return _map.empty(); }
