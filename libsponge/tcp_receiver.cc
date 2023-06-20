#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
	TCPHeader header = seg.header();
	Buffer payload = seg.payload();
	std::string data = std::string(payload.str());
	if (header.syn) {
		isn = header.seqno;
		ack = isn;
		flag = true;
	}  else {
		//invalid segment, we directly throw it.
		if (isn >= seg.header().seqno) {
			return;
		}
	}
	// if ack equals header.seqno, then seg is going to be written.
	if (flag) {
		bool eof = false;
		if (header.fin) { eof = true; finSeqno = header.seqno; }
		// There is a subtle problem in checkpoint.
		uint64_t checkpoint = static_cast<uint64_t>(_reassembler.stream_out().bytes_written()) + unassembled_bytes();
		// if header.seq equal isn, then fin and syn come together.
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
		if (ack == header.seqno) {
			ack = ack + written2 - written1 + (header.syn ? 1 : 0);
		}
		// If fin has been set and fin segment has been written.
		WrappingInt32 zero(0);
		if (finSeqno != zero && ack >= finSeqno) {
			ack = ack + 1;
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
