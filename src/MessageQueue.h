/**
 * @file MessageQueue.h 声明文件, 基于boost多进程封装消息队列
 * @version 0.2
 * @date 2017-10-02
 * - 优化消息队列实现方式
 * @date 2020-10-01
 * - 优化
 * - 面向gtoaes, 将GeneralControl和ObservationSystem的共同特征迁移至此处
 */

#ifndef SRC_MESSAGEQUEUE_H_
#define SRC_MESSAGEQUEUE_H_

#include <string>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/signals2/signal.hpp>
#include "BoostInclude.h"

class MessageQueue {
protected:
	/*--------------- 数据类型 ---------------*/
	struct Message {
		long id;			///< 消息编号
		long par1, par2;	///< 参数

	public:
		Message() {
			id = par1 = par2 = 0;
		}

		Message(long _id, long _par1 = 0, long _par2 = 0) {
			id   = _id;
			par1 = _par1;
			par2 = _par2;
		}
	};

	//////////////////////////////////////////////////////////////////////////////
	typedef boost::signals2::signal<void (const long, const long)>  CBF;	///< 消息回调函数
	typedef CBF::slot_type CBSlot;	///< 回调函数插槽
	typedef boost::shared_array<CBF> CBArray;	///< 回调函数数组
	typedef boost::interprocess::message_queue MsgQue;	///< boost消息队列
	typedef boost::shared_ptr<MsgQue> MsgQuePtr;	///< boost消息队列指针

protected:
	/* 成员变量 */
	//////////////////////////////////////////////////////////////////////////////
	enum {
		MSG_QUIT = 0,	///< 结束消息队列
		MSG_USER		///< 用户自定义消息起始编号
	};

	//////////////////////////////////////////////////////////////////////////////
	/* 消息队列 */
	std::string mqName_;		///< 消息队列名称. 用于删除错误创建的消息队列
	MsgQuePtr mqPtr_;			///< 消息队列
	const long funcs_count_;	///< 自定义回调函数数组长度
	CBArray funcs_;				///< 回调函数数组

	/* 多线程 */
	ThrdPtr thrdMsgLoop_;	///< 消息响应线程

public:
	MessageQueue();
	virtual ~MessageQueue();
	/*!
	 * @brief 创建消息队列并启动监测/响应服务
	 * @param name 消息队列名称
	 * @return
	 * 操作结果. false代表失败
	 */
	virtual bool Start(const char *name);
	/**
	 * @brief 停止服务
	 */
	virtual void Stop();
	/*!
	 * @brief 注册消息及其响应函数
	 * @param id   消息代码
	 * @param slot 回调函数插槽
	 * @return
	 * 消息注册结果. 若失败返回false
	 */
	bool RegisterMessage(const long id, const CBSlot& slot);
	/*!
	 * @brief 投递低优先级消息
	 * @param id   消息代码
	 * @param par1 参数1
	 * @param par2 参数2
	 */
	void PostMessage(const long id, const long par1 = 0, const long par2 = 0);
	/*!
	 * @brief 投递高优先级消息
	 * @param id   消息代码
	 * @param par1 参数1
	 * @param par2 参数2
	 */
	void SendMessage(const long id, const long par1 = 0, const long par2 = 0);

protected:
	/* 消息响应函数 */
	/*!
	 * @brief 注册消息响应函数
	 */
	virtual void register_messages() = 0;

protected:
	/*!
	 * @brief 线程, 监测/响应消息
	 */
	void message_loop();
};

#endif /* SRC_MESSAGEQUEUE_H_ */
