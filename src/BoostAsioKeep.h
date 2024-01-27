/**
 * @file AsioIOServiceKeep.h 封装boost::asio::io_service, 维持run()在生命周期内的有效性
 * @date 2017-01-27
 * @version 0.1
 * @author Xiaomeng Lu
 * @note
 * @li boost::asio::io_service::run()在响应所注册的异步调用后自动退出. 为了避免退出run()函数,
 * 建立ioservice_keep维护其长期有效性
 * @date  2021-11-21
 * @note io_service ==> io_context
 */

#ifndef SRC_ASIOIOSERVICEKEEP_H_
#define SRC_ASIOIOSERVICEKEEP_H_

#include <boost/asio/io_service.hpp>
#include <boost/thread/thread.hpp>

using boost::asio::io_service;//	BoostAsioService;

class BoostAsioKeep {
protected:
	/* 成员变量 */
	io_service ios_;
	io_service::work work_;
	boost::thread thrdKeep_;

public:
	BoostAsioKeep();
	virtual ~BoostAsioKeep();
	io_service& GetIOService();

public:
	/**
	 * @brief 检查io_service是否仍在运行
	 * @return io_service运行状态
	 */
	bool IsKeeping();
	/**
	 * @brief 停止服务
	 */
	void Stop();
	/**
	 * @brief 重启服务
	 */
	void Reset();
};

#endif /* SRC_ASIOIOSERVICEKEEP_H_ */
