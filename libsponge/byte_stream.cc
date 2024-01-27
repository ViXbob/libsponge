#include "byte_stream.hh"
#include<algorithm>
#include <cstddef>
#include<iostream>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`
// const unsigned int st=0;
template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) :
    _stream_capcity(capacity),
    _byte_stream(), 
    _byte_read_num(0),
    _byte_write_num(0),
    _byte_read_end(false),
    _error(false) {
}
size_t ByteStream::write(const string &data) {
    // cout<<"here!!!!!!!!!!!"<<endl;
    size_t write_num=0 , datalen=data.length();
    for(size_t i=0;i<datalen;i++){
        if(_byte_stream.size()==_stream_capcity) break;
        _byte_write_num++;
        _byte_stream.push_front(data[i]);
        write_num++;
    }
    return write_num;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // cout<<"here!!!!!!!!!!!"<<endl;
    if(_byte_stream.empty()) return "";
    size_t sz=_byte_stream.size();
    string subs="";
    size_t tag=min(sz , len);
    for(size_t i=tag-1;i!=SIZE_MAX;i--){
        subs.push_back(_byte_stream[i]);
    }
    return subs;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    // cout<<"here!!!!!!!!!!!"<<endl;
    size_t sz=_byte_stream.size();
    for(size_t i=1;i<=min(sz , len);i++){
        _byte_stream.pop_back();
        _byte_read_num++;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    // cout<<"here!!!!!!!!!!!"<<endl;

    size_t sz=_byte_stream.size();
    string subs="";
    for(size_t i=1;i<=min(sz , len);i++){
        _byte_read_num++;
        subs.push_back(_byte_stream.back());
        _byte_stream.pop_back();
    }
    // cout<<_byte_read_num<<"-----------"<<endl;
    return subs;
}

void ByteStream::end_input() {_byte_read_end=true;}

bool ByteStream::input_ended() const {return _byte_read_end; }

size_t ByteStream::buffer_size() const {return _byte_stream.size(); }

bool ByteStream::buffer_empty() const {return _byte_stream.empty(); }

bool ByteStream::eof() const {
    if(_byte_stream.empty()&&_byte_read_end) return true;
    return false; 
}

size_t ByteStream::bytes_written() const {return _byte_write_num; }

size_t ByteStream::bytes_read() const {return _byte_read_num; }

size_t ByteStream::remaining_capacity() const {return _stream_capcity-_byte_stream.size();}
