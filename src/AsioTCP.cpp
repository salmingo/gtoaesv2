/**
 * @file AsioTCP.cpp 定义文件, 基于boost::asio实现TCP通信接口
 */

#include <boost/system/system_error.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/placeholders.hpp>
#include <string.h>
#include "AsioTCP.h"
#include "GLog.h"

using std::string;
using namespace boost::system;
using namespace boost::placeholders;
using namespace boost::asio;

/////////////////////////////////////////////////////////////////////
/*--------------------- 客户端 ---------------------*/
TcpClient::TcpClient()
	: sock_(keep_.GetIOService()) {
	pckRead_.reset(new char[TCP_PACK_SIZE]);
	crcbuf_read_.set_capacity (TCP_PACK_SIZE * 50);
	crcbuf_write_.set_capacity(TCP_PACK_SIZE * 50);
}

TcpClient::~TcpClient() {
	Close();
}

BoostTcpSock& TcpClient::Socket() {
	return sock_;
}

bool TcpClient::Connect(const std::string& host, const uint16_t port, bool async) {
	try {
		BoostTcp::resolver rslv(keep_.GetIOService());
		BoostTcp::resolver::query query(host, boost::lexical_cast<string>(port));
		BoostTcp::resolver::iterator itertor = rslv.resolve(query);

		if (!keep_.IsKeeping()) keep_.Reset();
		if (async) {
			sock_.async_connect(*itertor, boost::bind(&TcpClient::handle_connect, this, placeholders::error));
		}
		else {
			sock_.connect(*itertor);
			sock_.set_option(BoostTcpSock::keep_alive(true));
			start_read();
		}
		return true;
	}
	catch(system_error& ex) {
		_gLog.Write(LOG_WARN, "[%s:%s], %s", __FILE__, __FUNCTION__, ex.what());
		return false;
	}
}

bool TcpClient::ShutDown(int how) {
	BoostTcpSock::shutdown_type type = how == 0 ? BoostTcpSock::shutdown_receive
				: (how == 1 ? BoostTcpSock::shutdown_send : BoostTcpSock::shutdown_both);
	try {
		sock_.shutdown(type);
		return true;
	}
	catch (system_error &ex) {
		// _gLog.Write(LOG_WARN, "[%s:%s], %s", __FILE__, __FUNCTION__, ex.what());
		return false;
	}
}

bool TcpClient::Close() {
	try {
		ShutDown(2);
		sock_.close();
		return true;
	}
	catch (system_error &ex) {
		// _gLog.Write(LOG_WARN, "[%s:%s], %s", __FILE__, __FUNCTION__, ex.what());
		return false;
	}
}

bool TcpClient::IsOpen() {
	return sock_.is_open();
}

int TcpClient::Read(char* data, const int n, const int from) {
	if (!data || n <= 0 || from < 0) return 0;

	MtxLck lck(mtx_read_);
	int end(from + n), to_read;

	to_read = crcbuf_read_.size() > end ? n : crcbuf_read_.size() - from;
	if (to_read > 0) {
		int i(0), j(from);
		for (; i < to_read; ++i, ++j) data[i] = crcbuf_read_[j];
		crcbuf_read_.erase_begin(j);
	}
	return to_read;
}

int TcpClient::Write(const char* data, const int n) {
	if (!data || n <= 0) return 0;

	MtxLck lck(mtx_write_);
	int had_write(n);
	int wait_write(crcbuf_write_.size());
	int capacity(crcbuf_write_.capacity() - wait_write);

	if (had_write > capacity) had_write = capacity;
	for (int i = 0; i < had_write; ++i) crcbuf_write_.push_back(data[i]);
	if (!wait_write) start_write();
	return had_write;
}

int TcpClient::Lookup(char* first) {
	MtxLck lck(mtx_read_);
	int n = crcbuf_read_.size();
	if (first && n) *first = crcbuf_read_[0];
	return n;
}

int TcpClient::Lookup(const char* flag, const int n, const int from) {
	if (!flag || n <= 0 || from < 0) return -1;

	MtxLck lck(mtx_read_);
	int end =crcbuf_read_.size() - n;
	int pos, i(0), j(0);

	for (pos = from; pos <= end; ++pos) {
		for (i = 0, j = pos; i < n && flag[i] == crcbuf_read_[j]; ++i, ++j);
		if (i == n) break;
	}
	return (i == n ? pos : -1);
}

int TcpClient::Lookup(const char chBegin, const char chEnd, int &posBegin, int &posEnd)
{
	MtxLck lck(mtx_read_);

	int end = crcbuf_read_.size();
	int pos, n1(0), n2(0);

	posBegin = 1;
	posEnd   = 0;

	for (pos = 0; pos < end && (!n2 || n2 != n1); ++pos) {
		if (crcbuf_read_[pos] == chBegin) {
			if (++n1 == 1) posBegin = pos;
		}
		else if (crcbuf_read_[pos] == chEnd) {
			if (++n2 == n1) posEnd = pos;
		}
	}
	return posEnd > posBegin ? (posEnd - posBegin + 1) : 0;
}

void TcpClient::Start()
{
	sock_.set_option(BoostTcpSock::keep_alive(true));
	start_read();
}

void TcpClient::RegisterConnect(const CBSlot& slot) {
	cbconn_.disconnect_all_slots();
	cbconn_.connect(slot);
}

void TcpClient::RegisterRead(const CBSlot& slot) {
	cbread_.disconnect_all_slots();
	cbread_.connect(slot);
}

void TcpClient::RegisterWrite(const CBSlot& slot) {
	cbwrite_.disconnect_all_slots();
	cbwrite_.connect(slot);
}

void TcpClient::start_read() {
	sock_.async_read_some(buffer(pckRead_.get(), TCP_PACK_SIZE),
			boost::bind(&TcpClient::handle_read, this,
				placeholders::error, placeholders::bytes_transferred));
}

void TcpClient::start_write() {
	int towrite(crcbuf_write_.size());
	if (towrite) {
		sock_.async_write_some(buffer(crcbuf_write_.linearize(), towrite),
				boost::bind(&TcpClient::handle_write, this,
					placeholders::error, placeholders::bytes_transferred));
	}
}

/* 响应async_函数的回调函数 */
void TcpClient::handle_connect(const error_code& ec) {
	cbconn_(this, ec);
	if (!ec) {
		sock_.set_option(BoostTcpSock::keep_alive(true));
		start_read();
	}
}

void TcpClient::handle_read(const error_code& ec, int n) {
	if (!ec) {
		MtxLck lck(mtx_read_);
		for (int i = 0; i < n; ++i) crcbuf_read_.push_back(pckRead_[i]);
	}
	cbread_(this, ec);
	if (!ec) start_read();
}

void TcpClient::handle_write(const error_code& ec, int n) {
	if (!ec) {
		MtxLck lck(mtx_write_);
		crcbuf_write_.erase_begin(n);
		start_write();
	}
	cbwrite_(this, ec);
}

/////////////////////////////////////////////////////////////////////
/*--------------------- 服务器 ---------------------*/
TcpServer::TcpServer()
	: accept_(keep_.GetIOService()) {
}

TcpServer::~TcpServer() {
	if (accept_.is_open()) {
		error_code ec;
		accept_.close(ec);
	}
}

void TcpServer::RegisterAccept(const CBSlot &slot) {
	cbfunc_.connect(slot);
}

bool TcpServer::Start(uint16_t port, bool v6) {
	try {
		BoostTcp::endpoint endpt(v6 ? BoostTcp::v6() : BoostTcp::v4(), port);
		accept_.open(endpt.protocol());
		accept_.bind(endpt);
		accept_.listen(BoostTcpSock::max_listen_connections);
		start_accept();
		return true;
	}
	catch (system_error& ex) {
		_gLog.Write(LOG_WARN, "[%s:%s], %s", __FILE__, __FUNCTION__, ex.what());
		return false;
	}
}

void TcpServer::start_accept() {
	if (accept_.is_open()) {
		TcpClient* client = new TcpClient;
		accept_.async_accept(client->Socket(),
				boost::bind(&TcpServer::handle_accept, this, client, placeholders::error));
	}
}

void TcpServer::handle_accept(TcpClient* client, const error_code& ec) {
	if (!ec) {
		cbfunc_(client, this);
		client->Start();
	}
	start_accept();
}
