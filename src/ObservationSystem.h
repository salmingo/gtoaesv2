#ifndef OBSERVATIONSYSTEM_H
#define OBSERVATIONSYSTEM_H

#include "MessageQueue.h"
#include "AsioTCP.h"
#include "KVProtocol.h"
#include "NonKVProtocol.h"

class ObservationSystem : public MessageQueue {
public:
    ObservationSystem(const string& gid, const string& uid);
    ~ObservationSystem();

	typedef boost::shared_ptr<ObservationSystem> Pointer;
	static Pointer Create(const string& gid, const string& uid) {
		return Pointer(new ObservationSystem(gid, uid));
	}

// 数据类型
public:
	struct CameraInfo {
		TcpCPtr ptrTcp;	///< TCP连接
		KVCamera info;	///< 相机实时工作状态
		int focState  = -1;		///< 0: 静止; 1: 定位中; -1: 未知
		int focPos = 999999;	///< 焦点实时位置
		int focTar = 999999;	///< 焦点目标位置
		int repeat = 0;  ///< 焦点位置重复次数
	};
	typedef std::vector<CameraInfo> CameraInfoVector;
	/*!
	 * @brief 声明回调函数及插槽: 观测计划状态
	 * @param 1 观测计划状态
	 */
	typedef boost::signals2::signal<void (KVPlanPtr)> PlanCBF;
	typedef PlanCBF::slot_type PlanCBSlot;

private:
	/**
	 * @brief 系统工作状态汇总
	 */
	struct SystemState {
		boost::mutex mtx; // 互斥锁
		int camonline = 0; 	///< 联机相机数量
		int exposing = 0;	///< 已曝光相机数量
		int waitflat = 0;	///< 等待平场的相机数量

		void CameraOnline(bool online = true) {
			MtxLck lck(mtx);
			if (online) ++camonline;
			else if (camonline) --camonline;
		}

		void EnterExpose() {// 单台相机进入曝光状态
			MtxLck lck(mtx);
			++exposing;
		}

		bool LeaveExpose() {// 单台相机离开曝光状态
			MtxLck lck(mtx);
			if (exposing) --exposing;
			return (!exposing);
		}

		bool AnyExposing() {// 任意相机正在曝光
			return exposing;
		}

		void EnterWaitFlat() {
			MtxLck lck(mtx);
			++waitflat;
		}

		void LeaveWaitFlat() {
			MtxLck lck(mtx);
			if (waitflat) --waitflat;
		}
	};

// 成员变量
private:
	string gid_;	///< 组标志
	string uid_;	///< 单元标志
	KVProtocol kvproto_;		///< 解析通信协议: 指令+键值对
	NonKVProtocol nonkvproto_;	///< 解析通信协议: 转台

	TcpCPtr tcpMount_;	///< TCP连接: 转台
	KVMount mountInfo_;	///< 转台实时工作状态
	int countMountPos_ = 0;	///< 接收到的转台位置计数
	CameraInfoVector camInfoVec_;	///< 相机集成接口
	TcpCPtr tcpFocus_;	///< TCP连接: 调焦

	KVAppGWACPtr plan_;	///< GWAC观测计划
	KVAppGWACPtr plan_wait_;	///< GWAC观测计划: 待执行
	KVPlanPtr plan_state_;	///< 观测计划状态
	PlanCBF cbfPlan_;		///< 回调函数: 观测计划状态

	SystemState sysState_;	///< 系统状态汇总
	int oldDay_ = 0;	///< UTC日期
	int planSN_ = 0;	///< 自定义计划序号

	Thread thrdObsplan_;	///< 线程: 监测观测计划
	boost::condition_variable cvNewPlan_;	///< 事件: 在线程中启动观测计划

	boost::posix_time::ptime lastClosed_;	///< 设备最后断开时间

public:
	/*!
	 * @brief 启动观测系统
	 * @return
	 * 操作结果. false代表失败
	 */
	bool Start();
	/**
	 * @brief 停止观测系统
	 */
	void Stop();
	/*!
	 * @brief 检查观测系统唯一性标志是否与输入条件一致
	 * @param gid 组标志
	 * @param uid  单元标志
	 * @return
	 * 匹配一致则返回true, 否则返回false
	 */
	bool IsMatched(const string& gid, const string& uid);
	// 查看转台状态
	const KVMount& GetInfoMount();
	// 查看相机状态
	const CameraInfoVector& GetInfoCamera();
	// 注册回调函数: 观测计划状态
	void RegisterPlanCallback(const PlanCBSlot& slot);
	/**
	 * @brief 计算当前时间与设备最后关闭时间的差异
	 * @param now  当前UTC时间
	 * @return
	 * 秒数, now - lastClosed
	 */
	int LastClosed(boost::posix_time::ptime &now);

public:
	/**
	 * @brief 关联观测系统和转台TCP连接
	 * @param ptrTcp  TCP连接
	 * @return 关联结果
	 */
	void CoupleMount(TcpCPtr ptrTcp);
	/**
	 * @brief 解除观测系统和转台TCP连接
	 * @param ptrTcp TCP连接
	 */
	void DecoupleMount(TcpCPtr ptrTcp);
	/**
	 * @brief 关联观测系统和相机TCP连接
	 * @param ptrTcp  TCP连接
	 * @param cid     相机标志
	 * @return 关联结果
	 */
	bool CoupleCamera(TcpCPtr ptrTcp, const string& cid);
	/**
	 * @brief 关联观测系统和调焦TCP连接
	 * @param ptrTcp  TCP连接
	 * @return 关联结果
	 */
	void CoupleFocus(TcpCPtr ptrTcp);
	/**
	 * @brief 解除观测系统和调焦TCP连接
	 * @param ptrTcp TCP连接
	 */
	void DecoupleFocus(TcpCPtr ptrTcp);
	/**
	 * @brief 通知转台状态
	 * @param state  转台状态
	 */
	void NotifyMountState(int state);
	/**
	 * @brief 通知转台位置
	 * @param pos  转台位置
	 */
	void NotifyMountPosition(const NonKVPosition& pos);
	/**
	 * @brief 更新焦点位置
	 * @param cid  相机标志
	 * @param pos  焦点位置
	 */
	void NotifyFocus(const string& cid, int pos);

public:
	// 通知: 观测计划
	void NotifyPlan(KVBasePtr proto);
	// 通知: 检查计划状态
	KVPlanPtr CheckPlan(const string& plan_sn);
	// 通知: 删除计划
	bool RemovePlan(const string& plan_sn);
	// 通知: 中断当前计划/指向/曝光
	void Abort();
	// 通知: 指向
	void Slewto(KVSlewPtr proto);
	// 通知: 复位
	void Park();
	// 通知: 导星
	void Guide(KVGuidePtr proto);
	// 通知: 搜索零点
	void FindHome();
	// 通知: 同步零点
	void HomeSync(KVSyncPtr proto);
	// 通知: 曝光
	void TakeImage(KVTkImgPtr proto);
	// 通知: 相机配置参数
	KVCamSetPtr CameraSetting(KVCamSetPtr proto);
	// 通知: 调焦
	void Focus(KVFocusPtr proto);
	// 通知: 消旋器
	void Derot(KVDerotPtr proto);
	// 通知: 圆顶
	void Dome(KVDomePtr proto);
	// 通知: 镜盖

private:
	enum {
		MSG_CAMERA_CLOSE = MSG_USER,
		MSG_CAMERA_RECEIVE,
		MSG_NEW_PLAN,
		MSG_FLAT_RESLEW,
		MSG_MAX
	};
	/*!
	 * @brief 注册消息响应函数
	 */
	void register_messages();
	// 关闭相机TCP连接
	void on_camera_close(const long connptr, const long peer_type);
	// 接收到相机TCP信息
	void on_camera_receive(const long connptr, const long peer_type);
	// 新计划
	void on_new_plan(const long, const long);
	// 平场: 重新指向
	void on_flat_reslew(const long, const long);

private:
	// 收到网络信息: 相机
	void camera_receive(TcpClient* cliptr, boost::system::error_code ec);
	// 处理图像协议: 相机
	void process_protocol_camera(TcpClient* cliptr, KVBasePtr proto);
	// 将指定协议发送给相机
	void write2camera(const char* cmd, int n, const char* cid = NULL);
	// 将指定曝光协议发送给相机
	void expose2camera(int cmd, int frmno = 0, const char* cid = NULL);

private:
	// 线程: 监测观测计划
	void thread_obsplan();
};
typedef ObservationSystem::Pointer ObssPtr;

#endif
