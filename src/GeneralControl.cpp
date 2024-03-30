
#include <ctype.h>
#include <boost/bind/bind.hpp>
#include <boost/bind/placeholders.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "GeneralControl.h"
#include "AstroDeviceDef.h"
#include "GLog.h"

#define MSGQUE_NAME "msgque_gtoaes"

using namespace boost;
using namespace boost::placeholders;
using namespace boost::posix_time;

GeneralControl::GeneralControl(Parameter *param)
	: param_(param) {
}

GeneralControl::~GeneralControl() {
}

// 启动服务
bool GeneralControl::Start() {
	if (!MessageQueue::Start(MSGQUE_NAME)) return false;
	if (!start_tcp_server()) return false;
	thrdCycleUpdClient_ = Thread(boost::bind(&GeneralControl::cycle_upload_client, this));
	thrdDumpObss_ = Thread(boost::bind(&GeneralControl::cycle_dump_obss, this));

	return true;
}

// 停止服务
void GeneralControl::Stop() {
	// 终止: 消息和线程
	MessageQueue::Stop();
	interrupt_thread(thrdCycleUpdClient_);
	interrupt_thread(thrdDumpObss_);
	// 终止: 观测系统
	for (auto it = obssVec_.begin(); it != obssVec_.end(); ++it) (*it)->Stop();
	obssVec_.clear();
	// 终止: 网络服务
	tcpSvrClient_.reset();
	tcpSvrMountGWAC_.reset();
	tcpSvrCameraGWAC_.reset();
	tcpSvrFocus_.reset();
	tcpSvrMountGFT_.reset();
	tcpSvrCameraGFT_.reset();
	// 终止: 网络连接
	tcpCliClient_.Reset();
	tcpCliDevice_.Reset();
}

// 注册消息响应函数
void GeneralControl::register_messages() {
	const CBSlot& slot1 = boost::bind(&GeneralControl::on_tcp_close,   this, _1, _2);
	const CBSlot& slot2 = boost::bind(&GeneralControl::on_tcp_receive, this, _1, _2);

	RegisterMessage(MSG_TCP_CLOSE,   slot1);
	RegisterMessage(MSG_TCP_RECEIVE, slot2);
}

// 关闭TCP连接
void GeneralControl::on_tcp_close(const long connptr, const long peer_type) {
	TcpCPtr sp = peer_type == PEER_CLIENT ?
		tcpCliClient_.Pop ((TcpClient*) connptr) : tcpCliDevice_.Pop ((TcpClient*) connptr);
	// GWAC系统
	if (peer_type == PEER_MOUNT_GWAC) {// 转台
		MtxLck lck(mtxObss_);
		for (auto it = obssVec_.begin(); it != obssVec_.end(); ++it) {
			(*it)->DecoupleMount(sp);
		}
	}
	else if (peer_type == PEER_FOCUS) {// 调焦
		MtxLck lck(mtxObss_);
		for (auto it = obssVec_.begin(); it != obssVec_.end(); ++it) {
			(*it)->DecoupleFocus(sp);
		}
	}
}

// 接收到TCP信息
void GeneralControl::on_tcp_receive(const long connptr, const long peer_type) {
	const char term[] = "\n"; // 结束符
	const int len = strlen(term); // 结束符长度
	char buff[TCP_PACK_SIZE];
	int pos, to_read;
	TcpClient* ptrTcp = (TcpClient*) connptr;

	while (ptrTcp->IsOpen() && (pos = ptrTcp->Lookup(term, len)) >= 0) {
		if ((to_read = pos + len) > TCP_PACK_SIZE) {// 信息长度超过预设最大值
			_gLog.Write(LOG_FAULT, "protocol length from %s is over than threshold",
				peer_type == PEER_CLIENT ? "client" :
					((peer_type == PEER_CAMERA_GWAC || peer_type == PEER_CAMERA_GFT)? "camera" :
						(peer_type == PEER_FOCUS ? "focus" : "mount")));
			ptrTcp->Close();
		}
		else {
			// 读取一条通信协议
			ptrTcp->Read(buff, pos + len);
			buff[pos] = '\0';
			// 解析-->
			if (peer_type == PEER_MOUNT_GWAC || peer_type == PEER_FOCUS) {// GWAC: 转台/调焦
				NonKVBasePtr proto = nonkvproto_.Resolve(buff);
				if (proto.unique()) {
					if (peer_type == PEER_MOUNT_GWAC) process_protocol_mount_gwac(ptrTcp, proto);
					else process_protocol_focus(ptrTcp, proto);
				}
			}
			else {// 远程: 客户端或后随望远镜/相机
				KVBasePtr proto = kvproto_.Resolve(buff);
				if (!proto.unique()) {
					_gLog.Write(LOG_FAULT, "undefined protocol from %s: <%s>",
						peer_type == PEER_CLIENT ? "client" :
							(peer_type == PEER_CAMERA_GWAC || peer_type == PEER_CAMERA_GFT) ? "camera" : "mount",
							buff);
					ptrTcp->Close();
				}
				else if (peer_type == PEER_CLIENT)    process_protocol_client(proto);
				else if (peer_type == PEER_MOUNT_GFT) process_protocol_mount_gft(ptrTcp, proto);
				else process_protocol_camera(ptrTcp, proto, peer_type);
			}
		}
	}
}

/**
 * @brief 创建TCP服务
 * @param server 对象智能指针
 * @param port   服务端口
 * @param peer   终端类型
 * @return 服务创建结果
 */
bool GeneralControl::create_tcp_server(TcpSPtr& server, int port, int peer_type) {
	const TcpServer::CBSlot& slot = boost::bind(&GeneralControl::tcp_accept, this, _1, _2, peer_type);
	server = TcpServer::Create();
	server->RegisterAccept(slot);
	return server->Start(port);
}

/**
 * @brief 启动所有TCP服务
 * @return 服务启动结果
 */
bool GeneralControl::start_tcp_server() {
	return create_tcp_server(tcpSvrClient_,     param_->portClient,     PEER_CLIENT)
		&& create_tcp_server(tcpSvrMountGWAC_,  param_->portMountGWAC,  PEER_MOUNT_GWAC)
		&& create_tcp_server(tcpSvrCameraGWAC_, param_->portCameraGWAC, PEER_CAMERA_GWAC)
		&& create_tcp_server(tcpSvrFocus_,      param_->portFocusGWAC,  PEER_FOCUS)
		&& create_tcp_server(tcpSvrMountGFT_,   param_->portMountGFT,   PEER_MOUNT_GFT)
		&& create_tcp_server(tcpSvrCameraGFT_,  param_->portCameraGFT,  PEER_CAMERA_GFT);
}

// 网络;服务;接收: 处理收到的连接请求
void GeneralControl::tcp_accept(TcpClient* cliptr, TcpServer* svrptr, int peer_type) {
	TcpCPtr client(cliptr);
	const TcpClient::CBSlot& slot = boost::bind(&GeneralControl::tcp_receive, this, _1, _2, peer_type);
	client->RegisterRead(slot);
	if (peer_type == PEER_CLIENT) tcpCliClient_.Push(client);
	else tcpCliDevice_.Push(client);
}

// 网络;接收;回调: 触发消息
void GeneralControl::tcp_receive(TcpClient* cliptr, boost::system::error_code ec, int peer_type) {
	PostMessage(!ec ? MSG_TCP_RECEIVE : MSG_TCP_CLOSE, (const long) cliptr, peer_type);
}

// 网络;响应;客户端: 分类处理
void GeneralControl::process_protocol_client(KVBasePtr proto) {
	string gid = proto->gid;
	string uid = proto->uid;
	char first = tolower(proto->type[0]); // 协议;指令字首字符,小写: 加速

	MtxLck lck(mtxObss_);
	for (auto it = obssVec_.begin(); it != obssVec_.end(); ++it) {
		if (!(*it)->IsMatched(gid, uid)) continue;

		if (first == 'a') {
			if (iequals(proto->type, KVTYPE_APPGWAC)  // 观测计划: GWAC
				|| iequals(proto->type, KVTYPE_APPPLAN)) {// 观测计划: GFT
				(*it)->NotifyPlan(proto);
			}
			else if (iequals(proto->type, KVTYPE_ABORT)) {// 中断当前操作和过程
				(*it)->Abort();
			}
		}
		else if (first == 'c') {
			if (iequals(proto->type, KVTYPE_CHKPLAN)) {// 检查计划状态
				string plan_sn = (boost::static_pointer_cast<KVCheckPlan>(proto))->plan_sn;
				KVPlanPtr plan = (*it)->CheckPlan(plan_sn);
				if (plan.use_count()) {
					string msg = plan->ToString();
					tcpCliClient_.Write(msg.c_str(), msg.size());
					break;
				}
			}
			else if (iequals(proto->type, KVTYPE_CAMSET)) {// 检查/修改相机参数
			}
		}
		else if (first == 'd') {
			if (iequals(proto->type, KVTYPE_DEROT)) {// 单独设备指令: 消旋器
			}
			else if (iequals(proto->type, KVTYPE_DOME)) {// 单独设备指令: 圆顶
			}
		}
		else if (first == 'f') {
			if (iequals(proto->type, KVTYPE_FOCUS)) {// 调焦
				(*it)->Focus(boost::static_pointer_cast<KVFocus>(proto));
			}
			else if (iequals(proto->type, KVTYPE_FOCUS_SYNC)) {// 调焦清零
				(*it)->FocusSync(boost::static_pointer_cast<KVFocusSync>(proto));
			}
			else if (iequals(proto->type, KVTYPE_FWHM)) {// 自动调焦
				(*it)->NotifyFWHM(boost::static_pointer_cast<KVFWHM>(proto));
			}
			else if (iequals(proto->type, KVTYPE_FILTER)) {// 变更滤光片
			}
		}
		else if (first == 'g') {
			if (iequals(proto->type, KVTYPE_GUIDE)) {// 导星
				(*it)->Guide(boost::static_pointer_cast<KVGuide>(proto));
			}
			else if (iequals(proto->type, KVTYPE_GEOSITE)) {// 设置或修改测站位置?
			}
		}
		else if (first == 's') {
			if (iequals(proto->type, KVTYPE_SLEWTO)) {// 转台指向
				(*it)->Slewto(boost::static_pointer_cast<KVSlewto>(proto));
			}
			else if (iequals(proto->type, KVTYPE_SYNC)) {// 转台同步零点
				(*it)->HomeSync(boost::static_pointer_cast<KVSync>(proto));
			}
		}
		else if (first == 't') {
			if (iequals(proto->type, KVTYPE_TRACK)) {// 转台转入跟踪模式
				(*it)->Track();
			}
			else if (iequals(proto->type, KVTYPE_TRACKVEL)) {// 设置转台跟踪速度, GWAC
				(*it)->TrackVel(boost::static_pointer_cast<KVTrackVel>(proto));
			}
			else if (iequals(proto->type, KVTYPE_TKIMG)) {// 手动曝光
				(*it)->TakeImage(boost::static_pointer_cast<KVTakeImage>(proto));
			}
		}
		else if (iequals(proto->type, KVTYPE_RMVPLAN)) {// 中断观测并删除观测计划
			string plan_sn = (boost::static_pointer_cast<KVRemovePlan>(proto))->plan_sn;
			if ((*it)->RemovePlan(plan_sn)) break;
		}
		else if (iequals(proto->type, KVTYPE_PARK)) {// 转台复位
			(*it)->Park();
		}
		else if (iequals(proto->type, KVTYPE_HOME)) {// 转台搜索零点
			(*it)->FindHome();
		}
		else if (iequals(proto->type, KVTYPE_MCOVER)) {// 控制镜盖
		}
	}
}

// 处理通信协议: 转台
void GeneralControl::process_protocol_mount_gwac(TcpClient* cliptr, NonKVBasePtr proto) {
	TcpCPtr sp = tcpCliDevice_.Find(cliptr);
	string gid = proto->gid;
	int imin = boost::iequals(gid, "001") ? 1 : 5;
	int imax = imin == 0 ? 4 : 10;

	if (iequals(proto->type, NONKVTYPE_STATE)) {
		boost::shared_ptr<NonKVStatus> status = boost::static_pointer_cast<NonKVStatus>(proto);
		boost::format fmt("%03d");
		for (int i = imin; i < status->n && i <= imax; ++i) {
			ObssPtr obss = find_obss(gid, (fmt % i).str());
			if (obss.use_count()) {
				obss->CoupleMount(sp);
				obss->NotifyMountState(status->state[i]);
			}
		}
	}
	else if (iequals(proto->type, NONKVTYPE_POS)) {
		boost::shared_ptr<NonKVPosition> pos = boost::static_pointer_cast<NonKVPosition>(proto);
		ObssPtr obss = find_obss(gid, pos->uid);
		if (obss.use_count()) obss->NotifyMountPosition(*pos);
	}
	else if (iequals(proto->type, NONKVTYPE_RESPONSE)) {
		boost::shared_ptr<NonKVResponse> rsp = boost::static_pointer_cast<NonKVResponse>(proto);
		ObssPtr obss = find_obss(gid, proto->uid);
		if (obss.use_count()) obss->NotifyResponse(*rsp);
	}
}

// 处理通信协议: GFT转台
void GeneralControl::process_protocol_mount_gft(TcpClient* cliptr, KVBasePtr proto) {
	if (iequals(proto->type, KVTYPE_MOUNT)) {
		ObssPtr obss = find_obss(proto->gid, proto->uid, 1);
		if (obss.use_count()) {
			TcpCPtr ptrTcp = tcpCliDevice_.Pop(cliptr);
			obss->CoupleMount(ptrTcp);
		}
	}
}

// 处理通信协议: 相机
void GeneralControl::process_protocol_camera(TcpClient* cliptr, KVBasePtr proto, int peer_type) {
	if (iequals(proto->type, KVTYPE_CAMERA)) {
		ObssPtr obss = find_obss(proto->gid, proto->uid, peer_type == PEER_CAMERA_GWAC ? 0 : 1);
		if (obss.use_count()) {
			TcpCPtr ptrTcp = tcpCliDevice_.Pop(cliptr);
			obss->CoupleCamera(ptrTcp, proto->cid);
		}
	}
}

// 处理通信协议: 转台
void GeneralControl::process_protocol_focus(TcpClient* cliptr, NonKVBasePtr proto) {
	TcpCPtr sp = tcpCliDevice_.Find(cliptr);
	string gid = proto->gid;
	ObssPtr obss = find_obss(gid, proto->uid);

	if (obss.use_count()) {
		if (iequals(proto->type, NONKVTYPE_FOCUS)) {
			boost::shared_ptr<NonKVFocus> focus = boost::static_pointer_cast<NonKVFocus>(proto);
			boost::format fmt("%03d");
			int uid = std::stoi(proto->uid) * 10 + 1;
			obss->CoupleFocus(sp);
			for (int i = 0; i < 5; ++i) {
				obss->NotifyFocus((fmt % (uid + i)).str(), focus->pos[i]);
			}
		}
		else if (iequals(proto->type, NONKVTYPE_RESPONSE)) {
			boost::shared_ptr<NonKVResponse> rsp = boost::static_pointer_cast<NonKVResponse>(proto);
			ObssPtr obss = find_obss(gid, proto->uid);
			obss->NotifyResponse(*rsp);
		}
	}
}

/*!
 * @brief 查找与gid和uid匹配的观测系统
 * @param gid 组标志
 * @param uid 单元标志
 * @param type 终端类型. 0==GWAC; 1==GFT
 * @return
 * 匹配的观测系统访问接口
 * @note
 * 若观测系统不存在, 则先创建该系统
 */
ObssPtr GeneralControl::find_obss(const string& gid, const string& uid, int type) {
	MtxLck lck(mtxObss_);
	ObssPtr obss;
	for (auto it = obssVec_.begin(); it != obssVec_.end(); ++it) {
		if ((*it)->IsMatched(gid, uid)) {
			obss = *it;
			break;
		}
	}
	if (!obss.use_count()) {
		obss = ObservationSystem::Create(gid, uid);
		if (!obss->Start(type)) obss.reset();
		else {
			const ObservationSystem::PlanCBSlot& slot = boost::bind(&GeneralControl::plan_state, this, _1);
			obss->RegisterPlanCallback(slot);
			obssVec_.push_back(obss);
		}
	}
	return obss;
}

// 定时;客户端;上传: 设备状态
void GeneralControl::cycle_upload_client() {
	boost::chrono::seconds period(2); // 周期2秒

	while (1) {
		boost::this_thread::sleep_for(period);
		if (!tcpCliClient_.Size()) continue;

		MtxLck lck(mtxObss_);
		for (auto it = obssVec_.begin(); it != obssVec_.end(); ++it) {// 遍历观测系统
			// 状态: 转台
			string mount = (*it)->GetInfoMount().ToString();
			tcpCliClient_.Write(mount.c_str(), mount.size());
			// 状态: 相机/调焦/消旋
			KVFocus focus;
			KVDerot derot;

			focus.gid = (*it)->GetInfoMount().gid;
			focus.uid = (*it)->GetInfoMount().uid;
			focus.opType = 0;

			derot.gid = (*it)->GetInfoMount().gid;
			derot.uid = (*it)->GetInfoMount().uid;
			derot.opType = 0;

			const ObservationSystem::CameraInfoVector& camera = (*it)->GetInfoCamera();
			for (auto it1 = camera.begin(); it1 != camera.end(); ++it1) {// 遍历相机
				// 相机
				string info = (*it1).info.ToString();
				tcpCliClient_.Write(info.c_str(), info.size());
				// 调焦
				if ((*it1).focPos != INT_MAX) {
					focus.utc    = it1->focUtc;
					focus.cid    = it1->info.cid;
					focus.state  = it1->focState;
					focus.pos    = it1->focPos;
					focus.posTar = it1->focTar;
					info = focus.ToString();
					tcpCliClient_.Write(info.c_str(), info.size());
				}
				// 消旋
				if ((*it1).derotEnabled) {
					derot.utc   = it1->derotUtc;
					derot.state = it1->derotState;
					derot.pos   = it1->derotPos;
					derot.posTar= it1->derotTar;
					info = derot.ToString();
					tcpCliClient_.Write(info.c_str(), info.size());
				}
			}
		}
	}
}

// 定时;客户端;上传: 计划状态
void GeneralControl::plan_state(KVPlanPtr plan) {
	if (tcpCliClient_.Size()) {
		string msg = plan->ToString();
		tcpCliClient_.Write(msg.c_str(), msg.size());
	}
}

// 定时;管理: 销毁观测系统
void GeneralControl::cycle_dump_obss() {
	boost::chrono::minutes period(1);
	int limit(300); // 300秒

	while (1) {
		boost::this_thread::sleep_for(period);

		MtxLck lck(mtxObss_);
		ptime now = second_clock::universal_time();
		for (auto it = obssVec_.begin(); it != obssVec_.end(); ) {
			if ((*it)->LastClosed(now) > limit) {
				(*it)->Stop();
				it = obssVec_.erase(it);
			}
			else ++it;
		}
	}
}
