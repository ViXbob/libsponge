#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const { 
	return _sender.bytes_in_flight();
 }

size_t TCPConnection::unassembled_bytes() const { 
	return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
	DUMMY_CODE(seg);
	if (!active()) { return ;}
	_time_since_last_segment_received = 0;
	if (seg.header().rst) {
		//false indicates we receive a rst segment.
		unclean_shutdown(false);
		return;
	}
	if (seg.header().ack) {
		_sender.ack_received(seg.header().ackno, seg.header().win);
	}
	// If we receive a segment with fin, then we end the receiver.
	if (seg.header().fin) {
		_receiver.stream_out().end_input();
		// if the sender is already in the eof state, then it is a active close, instead it is a passive close.
		if (!_sender.stream_in().eof()) {
			_linger_after_streams_finish = false;
		}
	}
	_receiver.segment_received(seg);
	// if incoming segment occupied any sequence number, even tcp_sender has no segments out, we have to send a segment to update ackno and window size.
	if (seg.length_in_sequence_space() >= 1) {
		_sender.fill_window();
		if (!_sender.segments_out().empty()) {
			write("");
			return;
		}
		auto ackOptional = _receiver.ackno();
		size_t window_size = _receiver.window_size();
		TCPSegment segment;
		if (ackOptional.has_value()) {
			segment.header().ackno = ackOptional.value();
			segment.header().ack = true;
		}
		segment.header().win = window_size;
		_segments_out.push(segment);
	} 
} 
bool TCPConnection::active() const { 
	//uncleanly shutdown.
	if (_receive_or_send_rst) {
		return false;
	}
	if (_clean_shutdown) {
		return false;
	}
	return true;
}

size_t TCPConnection::write(const string &data) {
    DUMMY_CODE(data);
	ByteStream b = _sender.stream_in();
	size_t ret = b.write(data);
	_sender.stream_in() = b;
	_sender.fill_window();
	send_segment();
    return ret;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
	DUMMY_CODE(ms_since_last_tick); 
	if (!active()) { return ; }
	_time_since_last_segment_received += ms_since_last_tick;
	_sender.tick(ms_since_last_tick);
    //if transmissions is larger than MAX_RETX_ATTEMPTS, then send a reset segment to the peer.
	if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
		unclean_shutdown(true);
    }
	//here we maybe write a segment into _sender.segments_out, we need to push it into the tcpconnection's segments queue
	send_segment();
	// end the connection cleanly if necessary.
	clean_shutdown();
}

//shutdown the outbound byte stream.
void TCPConnection::end_input_stream() {
	// if the inbound stream ends before the connection has reached out EOF on its inbound stream, then set _stream_finish to false.
	_sender.stream_in().end_input();
	// send a segment with fin.
	_sender.fill_window();
	send_segment();
}

void TCPConnection::connect() {
	_sender.fill_window();
	if (_sender.segments_out().size() != 1) {
		throw "segments size must equal 1";
	}
	send_segment();
}

void TCPConnection::clean_shutdown() {
	// prerequest 1
	if (!_receiver.stream_out().input_ended() || unassembled_bytes()) 		return;
	// prerequest 2
	if (!_sender.stream_in().eof() || !_sender.fin_sent()) {
		return;
	}
	// prerequest 3
	if (_sender.bytes_in_flight()) {
		return;
	}
	// passive close
	if (!_linger_after_streams_finish) {
		_clean_shutdown = true;
	}
	// active close.
	if (time_since_last_segment_received() >= (_cfg.rt_timeout * 10)) {
		_clean_shutdown = true;
	}
}

void TCPConnection::unclean_shutdown(bool send) {
		if (send) {
			_sender.send_empty_segment();
			TCPSegment empty_segment = _sender.segments_out().front();
			_sender.segments_out().pop();
			empty_segment.header().rst = true;
			_receive_or_send_rst = true;
			_segments_out.push(empty_segment);
		}
		_receive_or_send_rst = true;
		_sender.stream_in().set_error();
		_receiver.stream_out().set_error();
}
void TCPConnection::send_segment() {
	if (!active()) { return ; }
	while (!_sender.segments_out().empty()) {
		TCPSegment segment = _sender.segments_out().front();
		_sender.segments_out().pop();
		size_t window_size = _receiver.window_size();
		segment.header().win = window_size;
		auto ackOptional = _receiver.ackno();
		if (ackOptional.has_value()) {
			segment.header().ackno = ackOptional.value();
			segment.header().ack = true;
		}
		_segments_out.push(segment);
	}
	clean_shutdown();
}
TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
			unclean_shutdown(true);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
