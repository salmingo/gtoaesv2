/**
 * @file AsioIOServiceKeep.cpp 封装boost::asio::io_service, 维持run()在生命周期内的有效性
 * @date 2017-01-27
 * @version 0.1
 * @author Xiaomeng Lu
 */
#include <boost/bind/bind.hpp>
#include "BoostAsioKeep.h"

using namespace boost::asio;

BoostAsioKeep::BoostAsioKeep()
	: work_(ios_) {
	thrdKeep_ = boost::thread(boost::bind(&io_service::run, boost::ref(ios_)));
}

BoostAsioKeep::~BoostAsioKeep() {
	ios_.stop();
	thrdKeep_.join();
}

io_service& BoostAsioKeep::GetIOService() {
	return ios_;
}

bool BoostAsioKeep::IsKeeping() {
	return thrdKeep_.joinable();
}

void BoostAsioKeep::Stop() {
	ios_.stop();
	thrdKeep_.join();
}

void BoostAsioKeep::Reset() {
	if (!thrdKeep_.joinable()) thrdKeep_ = boost::thread(boost::bind(&io_service::run, boost::ref(ios_)));
}
