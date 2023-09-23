#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
	, _rto{retx_timeout}
    , _stream(capacity)
	, _window_size(1)
	, _retransmission_counter(0) 
	, _ackno(_isn) {}

uint64_t TCPSender::bytes_in_flight() const { 
	if (_segments_map.empty()) {
		return 0;
	}
	uint64_t bytes = 0;
	for (auto ptr = _segments_map.begin(); ptr != _segments_map.end(); ptr++) {
		bytes += ptr->second.length_in_sequence_space();
	}
	return bytes;
}

void TCPSender::fill_window() {
	//determine the current size of TCPSegment queue;
	// should we need to send TCPSegment with SYN or FIN flag.
	uint64_t remaining_size;
	remaining_size = max(0, _ackno + _window_size - (_isn + _next_seqno));
	// State of stream started but nothing acknowledged
	if (_next_seqno > 0 && _next_seqno == bytes_in_flight()) {
		return;
	}
	// state of stream finished and fully acknowledged
	// we can't add bytes_in_flight() == 0 condition.
	// because at this momement, we can't send any data any more.
	if (stream_in().eof() && _next_seqno == stream_in().bytes_written() + 2) {
		return;
	}
	while (remaining_size > 0) {
		TCPHeader header;
		// we need to change here.
		header.seqno = wrap(_next_seqno, _isn);
		std::string s;
		// first send
		if (_next_seqno == 0) {
			header.syn = true;
			s = "";
		} else {
			if (stream_in().input_ended()) {
				if (_next_seqno == stream_in().bytes_written() + 1) {
					header.fin = true;
					s = "";
					_fin_sent = true;
					//set the remaining size to 1 + string length, so we can end the loop after this.
					remaining_size = 1 + s.size();
				} 
				// if the remaining size can be read all by _stream, then we come to the eof state, and sent the fin segment.
				else if (stream_in().buffer_size() < remaining_size){
					header.fin = true;
					s = _stream.read(std::min(TCPConfig::MAX_PAYLOAD_SIZE, remaining_size));
					remaining_size = 1 + s.size();
				} 
				else {
					s = _stream.read(std::min(TCPConfig::MAX_PAYLOAD_SIZE, remaining_size));
				}
			} else {
				s = _stream.read(std::min(TCPConfig::MAX_PAYLOAD_SIZE, remaining_size));
				if (s.size() == 0) {
					break;
				} 
			}
		}
		Buffer payload = Buffer(std::move(s));
		TCPSegment segment;
		segment.header() = header;
		segment.payload() = payload;
		_segments_out.push(segment);
		_segments_map[segment.header().seqno.raw_value()] = segment;
		remaining_size -= segment.length_in_sequence_space();
		_next_seqno += segment.length_in_sequence_space();
	}
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
	// if we receive a ackno which is smaller than _ackno
	if (_ackno - ackno > 0) {
		return;
	}
	if (_next_seqno && ackno - wrap(_next_seqno, _isn) > 0) {
		return;
	}
	//TCPConfig::default_capacity == 64000, so uint16_t is enough to store the window size.
	DUMMY_CODE(ackno, window_size); 
	_zero_window = (window_size == 0 ? true : false);
	_window_size = (window_size > 0 ? window_size : 1);
	_ackno = ackno;
	// if ackno is bigger than the segment's seqno, remove that segment.
	uint32_t size = _segments_map.size();
	for (auto ptr = _segments_map.begin(); ptr != _segments_map.end();) {
		uint64_t seqno = ptr->first;
		if (ackno - WrappingInt32(seqno + ptr->second.length_in_sequence_space()) >= 0) {
			auto ptr_next = _segments_map.erase(ptr);
			if (ptr_next == _segments_map.end()) {
				break;
			} 
			ptr = ptr_next;
		} else {
			ptr++;
		}
	}

	if (_segments_map.size() < size) {
		_rto = _initial_retransmission_timeout;
		_retransmission_counter = 0;
	}
	if (_next_seqno != 0) {
		fill_window();
	}
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
	DUMMY_CODE(ms_since_last_tick);
	if (_rto <= ms_since_last_tick) {
		if (!_segments_map.empty()) {
			TCPSegment segment = _segments_map.begin()->second;
			_segments_out.push(segment);
			if (_zero_window) {
				_rto = _initial_retransmission_timeout;
				return;
			}
			_retransmission_counter += 1;
			_rto = _initial_retransmission_timeout; for (int i = 0; i < _retransmission_counter; i++) {
				_rto *= 2;
			}
		}
	} else {
		_rto -= ms_since_last_tick;
	}
}

unsigned int TCPSender::consecutive_retransmissions() const {
	return _retransmission_counter;
}

bool TCPSender::fin_sent() {
	return _fin_sent;
}

void TCPSender::send_empty_segment() {
	TCPHeader header;
	header.seqno = wrap(_next_seqno, _isn);
	Buffer payload;
	TCPSegment segment;
	segment.header() = header;
	segment.payload() = payload;
	_segments_out.push(segment);
}
