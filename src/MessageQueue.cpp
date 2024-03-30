/*!
 * @file MessageQueue.h 声明文件, 基于boost::interprocess::ipc::message_queue封装消息队列
 * @version 0.2
 * @date 2017-10-02
 * - 优化消息队列实现方式
 * @date 2020-10-01
 * - 优化
 */

#include "MessageQueue.h"
#include "GLog.h"

using namespace boost::interprocess;

MessageQueue::MessageQueue()
	: funcs_count_(1024) {
	funcs_.reset(new CBF[funcs_count_]);
}

MessageQueue::~MessageQueue() {
	Stop();
}

bool MessageQueue::Start(const char *name) {
	if (!thrdMsgLoop_.unique()) {
		try {// 启动消息队列
			mqName_ = name;
			message_queue::remove(name);
			mqPtr_.reset(new MsgQue(open_or_create, name, 1024, sizeof(Message)));
			register_messages();
			thrdMsgLoop_.reset(new boost::thread(boost::bind(&MessageQueue::message_loop, this)));
		}
		catch(interprocess_exception &ex) {
			_gLog.Write(LOG_FAULT, "[%s : %s] %s", __FILE__, __FUNCTION__, ex.what());
			return false;
		}
	}
	return thrdMsgLoop_.unique();
}

void MessageQueue::Stop() {
	if (thrdMsgLoop_.unique()) {
		SendMessage(MSG_QUIT);
		thrdMsgLoop_->join();
		thrdMsgLoop_.reset();
		message_queue::remove(mqName_.c_str());
	}
}

bool MessageQueue::RegisterMessage(const long id, const CBSlot& slot) {
	long pos(id - MSG_USER);
	bool rslt = pos >= 0 && pos < funcs_count_;
	if (rslt) funcs_[pos].connect(slot);
	return rslt;
}

void MessageQueue::PostMessage(const long id, const long par1, const long par2) {
	if (mqPtr_.unique()) {
		Message msg(id, par1, par2);
		mqPtr_->send(&msg, sizeof(Message), 1);
	}
}

void MessageQueue::SendMessage(const long id, const long par1, const long par2) {
	if (mqPtr_.unique()) {
		Message msg(id, par1, par2);
		mqPtr_->send(&msg, sizeof(Message), 10);
	}
}

void MessageQueue::message_loop() {
	Message msg;
	MsgQue::size_type szRcv, szBuf(sizeof(Message));
	uint32_t priority, pos;

	do {
		mqPtr_->receive(&msg, szBuf, szRcv, priority);
		if ((pos = msg.id - MSG_USER) >= 0 && pos < funcs_count_) {
			(funcs_[pos])(msg.par1, msg.par2);
		}
	} while(msg.id != MSG_QUIT);
}
