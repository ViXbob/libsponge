#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

/* 
	Function: received a TCPSegment, and push it into _reassembler.
	Note:
		1. if segment is invalid, we directly throw it.
		2. fin and syn maybe come together or not.
		3. we may receive 2nd FIN segment.
		4. we may receive a segment out of window.
*/
void TCPReceiver::segment_received(const TCPSegment &seg) {
	TCPHeader header = seg.header();
	Buffer payload = seg.payload();
	std::string data = std::string(payload.str());
	// if we receive a second synSegment.
	if (flag && header.syn) {
		return;
	}
	if (header.syn) {
		isn = header.seqno;
		ack = isn;
		flag = true;
	}  else {
		//invalid segment, we directly throw it.
		//case1: seqno is smaller than isn
		//case2: seqno plus data.size is smaller than ack
		//case3: seqno is out of window.
		//note case1 and case2 is not duplicated.
		// case2 and case3 are the left edge and right edge.
		if (seg.header().seqno - isn <= 0 || seg.header().seqno + data.size() - ack < 0 || static_cast<int32_t>(window_size()) <= (seg.header().seqno - ack)) {
			return;
		}
	}
	if (flag) {
		bool eof = false;
		if (header.fin) { eof = true; finSeqno = header.seqno; }
		uint64_t checkpoint = static_cast<uint64_t>(_reassembler.stream_out().bytes_written()) + unassembled_bytes();
		// if header.seq equal isn, then it is the first segment.
		// else, syn has been sent, so we have to minus 1.
		uint64_t index;
		if (header.seqno == isn) {
			index = 0;
		} else {
			index = unwrap(header.seqno, isn, checkpoint) - 1;
		}
	// the TcpReceiver will deliver the data from TCPSegment into streamReassembler using push_substring method.
		uint64_t written1 = _reassembler.stream_out().bytes_written();
		_reassembler.push_substring(data, index, eof);
		uint64_t written2 = _reassembler.stream_out().bytes_written();
		//If ack equals header.seqno, the segment has been written.
		ack = ack + written2 - written1 + (header.syn ? 1 : 0);

		// we can't let the follwing code execute twice, because we may receive 2nd FIN segment, then the number of ack will be wrong.
		WrappingInt32 zero(0);
		// If fin has been set and fin segment has been written.
		if (!finWritten && finSeqno != zero && ack >= finSeqno) {
			ack = ack + 1;
			finWritten = true;
		}
	}
    DUMMY_CODE(seg);
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
	// the isn hasn't been set yet.
	if (!flag) {
		return nullopt;
	}
	return ack;
}

size_t TCPReceiver::window_size() const { 
	ByteStream stream = _reassembler.stream_out();
	return stream.remaining_capacity();
}
