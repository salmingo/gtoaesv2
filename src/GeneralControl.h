#ifndef GENERALCONTROL_H
#define GENERALCONTROL_H

#include <vector>
#include "MessageQueue.h"
#include "Parameter.h"
#include "AsioTCP.h"
#include "KVProtocol.h"
#include "NonKVProtocol.h"
#include "ObservationSystem.h"

class GeneralControl : public MessageQueue
{
public:
    GeneralControl(Parameter* param);
    ~GeneralControl();

// 数据类型
private:
	struct TcpCVec {
		boost::mutex mtx;				///< 互斥锁
		std::vector<TcpCPtr> connBuff;	///< 连接池

	public:
		size_t Size() { return connBuff.size(); }
		void Reset() {
			MtxLck lck(mtx);
			connBuff.clear();
		}

		void Push(TcpCPtr conn) {
			MtxLck lck(mtx);
			connBuff.push_back(conn);
		}

		TcpCPtr Pop(TcpClient* client) {
			TcpCPtr ptr;
			MtxLck lck(mtx);
			for (auto it = connBuff.begin(); it != connBuff.end(); ++it) {
				if ((*it).get() == client) {
					ptr = *it;
					connBuff.erase(it);
					break;
				}
			}
			return ptr;
		}

		TcpCPtr Find(TcpClient* client) {
			MtxLck lck(mtx);
			TcpCPtr ptr;
			for (auto it = connBuff.begin(); it != connBuff.end(); ++it) {
				if ((*it).get() == client) {
					ptr = *it;
					break;
				}
			}
			return ptr;
		}

		void Write(const char* data, int len) {
			MtxLck lck(mtx);
			for (auto it = connBuff.begin(); it != connBuff.end(); ++it) {
				(*it)->Write(data, len);
			}
		}
	};

	typedef std::vector<ObssPtr> ObssVec;

// 成员变量
private:
	Parameter* param_;		///< 配置参数
	TcpSPtr tcpSvrClient_;		///< TCP服务: 客户端
	TcpSPtr tcpSvrMountGWAC_;	///< TCP服务: 转台, GWAC
	TcpSPtr tcpSvrCameraGWAC_;	///< TCP服务: 相机, GWAC
	TcpSPtr tcpSvrFocus_;		///< TCP服务: 调焦, GWAC
	TcpSPtr tcpSvrMountGFT_;	///< TCP服务: 转台, GFT
	TcpSPtr tcpSvrCameraGFT_;	///< TCP服务: 相机, GFT

	TcpCVec tcpCliClient_;	///< TCP客户: 客户端
	TcpCVec tcpCliDevice_;	///< TCP客户: 设备

	KVProtocol kvproto_;		///< 解析通信协议: 指令+键值对
	NonKVProtocol nonkvproto_;	///< 解析通信协议: 转台

	boost::mutex mtxObss_;	///< 互斥锁: 观测系统集合
	ObssVec obssVec_;	///< 观测系统集合

	Thread thrdCycleUpdClient_;	///< 定时向客户端上传系统工作状态
	Thread thrdDumpObss_;	///< 线程: 定时检查观测系统有效性

public:
	// 启动服务
	bool Start();
	// 停止服务
	void Stop();

// 功能: 消息响应函数
private:
	enum {
		MSG_TCP_CLOSE = MSG_USER,
		MSG_TCP_RECEIVE,
		MSG_MAX
	};
	// 注册消息响应函数
	void register_messages();
	// 关闭TCP连接
	void on_tcp_close(const long connptr, const long peer_type);
	// 接收到TCP信息
	void on_tcp_receive(const long connptr, const long peer_type);

// 功能: 网络通信
private:
	/**
	 * @brief 创建TCP服务
	 * @param server      对象智能指针
	 * @param port        服务端口
	 * @param peer_type   终端类型
	 * @return 服务创建结果
	 */
	bool create_tcp_server(TcpSPtr& server, int port, int peer_type);
	/**
	 * @brief 启动所有TCP服务
	 * @return 服务启动结果
	 */
	bool start_tcp_server();
	// 收到连接请求
	void tcp_accept(TcpClient* cliptr, TcpServer* svrptr, int peer_type);
	// 收到网络信息
	void tcp_receive(TcpClient* cliptr, boost::system::error_code ec, int peer_type);

	// 处理通信协议: 客户端
	void process_protocol_client(KVBasePtr proto);
	// 处理通信协议: 转台, GWAC
	void process_protocol_mount_gwac(TcpClient* cliptr, NonKVBasePtr proto);
	// 处理通信协议: 转台, GFT
	void process_protocol_mount_gft(TcpClient* cliptr, KVBasePtr proto);
	// 处理通信协议: 相机
	void process_protocol_camera(TcpClient* cliptr, KVBasePtr proto, int peer_type);
	// 处理通信协议: 调焦
	void process_protocol_focus(TcpClient* cliptr, NonKVBasePtr proto);

private:
	/*!
	 * @brief 查找与gid和uid匹配的观测系统
	 * @param gid  组标志
	 * @param uid  单元标志
	 * @param type 终端类型. 0==GWAC; 1==GFT
	 * @return
	 * 匹配的观测系统访问接口
	 * @note
	 * 若观测系统不存在, 则先创建该系统
	 */
	ObssPtr find_obss(const string& gid, const string& uid, int type = 0);

private:
	/**
	 * @brief 回调函数: 向客户端发送观测计划状态
	 * @param plan  观测计划状态
	 */
	void plan_state(KVPlanPtr plan);
	/**
	 * @brief 线程: 定时向客户端上传系统工作状态
	 */
	void cycle_upload_client();
	/**
	 * @brief 线程: 定时清理无效观测系统
	 */
	void cycle_dump_obss();
};

#endif
