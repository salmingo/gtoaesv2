
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
	interrupt_thread(thrdCycleUpdClient_);
	interrupt_thread(thrdDumpObss_);
	for (auto it = obssVec_.begin(); it != obssVec_.end(); ++it) (*it)->Stop();
	obssVec_.clear();

	tcpSvrClient_.reset();
	tcpSvrMountGWAC_.reset();
	tcpSvrCamera_.reset();
	tcpSvrFocusGWAC_.reset();
	tcpSvrMountGFT_.reset();

	tcpCliClient_.Reset();
	tcpCliMountGWAC_.Reset();
	tcpCliCamera_.Reset();
	tcpCliFocusGWAC_.Reset();
	tcpCliMountGFT_.Reset();

	MessageQueue::Stop();
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
	if (peer_type == PEER_CLIENT)     tcpCliClient_.Pop((TcpClient*) connptr);
	else if (peer_type == PEER_CAMERA)tcpCliCamera_.Pop((TcpClient*) connptr);
	else if (peer_type == PEER_MOUNT_GWAC) {
		TcpCPtr sp = tcpCliMountGWAC_.Pop ((TcpClient*) connptr);
		MtxLck lck(mtxObss_);
		for (auto it = obssVec_.begin(); it != obssVec_.end(); ++it) {
			(*it)->DecoupleMount(sp);
		}
	}
	else if (peer_type == PEER_FOCUS) {
		TcpCPtr sp = tcpCliFocus_.Pop ((TcpClient*) connptr);
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

	while ((pos = ptrTcp->Lookup(term, len)) >= 0) {
		if ((to_read = pos + len) > TCP_PACK_SIZE) {// 信息长度超过预设最大值
			_gLog.Write(LOG_FAULT, "protocol length from %s is over than threshold",
				peer_type == PEER_CLIENT ? "client" : (peer_type == PEER_MOUNT ? "mount" : "camera"));
			ptrTcp->Close();
		}
		else {
			ptrTcp->Read(buff, pos + len);
			buff[pos] = '\0';

			if (peer_type == PEER_MOUNT || peer_type == PEER_FOCUS) {// 远程: 转台或调焦
				NonKVBasePtr proto = nonkvproto_.Resolve(buff);
				if (proto.unique()) {
					if (peer_type == PEER_MOUNT) process_protocol_mount(ptrTcp, proto);
					else process_protocol_focus(ptrTcp, proto);
				}
				// else _gLog.Write(LOG_FAULT, "peer = %d, resolve failed: %s", peer_type, buff);
			}
			else {// 远程: 客户端或相机
				KVBasePtr proto = kvproto_.Resolve(buff);
				if (proto.unique()) {
					if (peer_type == PEER_CLIENT) process_protocol_client(proto);
					else process_protocol_camera(ptrTcp, proto);
				}
				else {
					_gLog.Write(LOG_FAULT, "undefined protocol from %s",
						peer_type == PEER_CLIENT ? "client" : "cameras");
					ptrTcp->Close();
				}
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
	return create_tcp_server(tcpSvrClient_, param_->portClient,         PEER_CLIENT)
		&& create_tcp_server(tcpSvrMountGWAC_,  param_->portMountGWAC,  PEER_MOUNT)
		&& create_tcp_server(tcpSvrCamera_, param_->portCamera,         PEER_CAMERA)
		&& create_tcp_server(tcpSvrFocusGWAC_,  param_->portFocusGWAC,  PEER_FOCUS)
		&& create_tcp_server(tcpSvrMountGFT_,  param_->portMountGFT,    PEER_MOUNT);
}

// 收到连接请求
void GeneralControl::tcp_accept(TcpClient* cliptr, TcpServer* svrptr, int peer_type) {
	TcpCPtr client(cliptr);
	const TcpClient::CBSlot& slot = boost::bind(&GeneralControl::tcp_receive, this, _1, _2, peer_type);
	client->RegisterRead(slot);

	if (peer_type == PEER_CLIENT)     tcpCliClient_.Push(client);
	else if (peer_type == PEER_MOUNT) tcpCliMount_.Push(client);
	else if (peer_type == PEER_CAMERA)tcpCliCamera_.Push(client);
	else if (peer_type == PEER_FOCUS) tcpCliFocus_.Push(client);
}

// 收到网络信息
void GeneralControl::tcp_receive(TcpClient* cliptr, boost::system::error_code ec, int peer_type) {
	PostMessage(!ec ? MSG_TCP_RECEIVE : MSG_TCP_CLOSE, (const long) cliptr, peer_type);
}

// 处理图像协议: 客户端
void GeneralControl::process_protocol_client(KVBasePtr proto) {
	string gid = proto->gid;
	string uid = proto->uid;

	MtxLck lck(mtxObss_);
	for (auto it = obssVec_.begin(); it != obssVec_.end(); ++it) {
		if (!(*it)->IsMatched(gid, uid)) continue;
		// 观测计划
		if (iequals(proto->type, KVTYPE_APPGWAC) || iequals(proto->type, KVTYPE_APPPLAN)) {
			(*it)->NotifyPlan(proto);
		}
		else if (iequals(proto->type, KVTYPE_CHKPLAN)) {
			string plan_sn = (boost::static_pointer_cast<KVCheckPlan>(proto))->plan_sn;
			KVPlanPtr plan = (*it)->CheckPlan(plan_sn);
			if (plan.use_count()) {
				string msg = plan->ToString();
				tcpCliClient_.Write(msg.c_str(), msg.size());
				break;
			}
		}
		else if (iequals(proto->type, KVTYPE_RMVPLAN)) {
			string plan_sn = (boost::static_pointer_cast<KVRemovePlan>(proto))->plan_sn;
			if ((*it)->RemovePlan(plan_sn)) break;
		}
		else if (iequals(proto->type, KVTYPE_ABORT)) {
			(*it)->Abort();
		}
		// 单独指令: 转台
		else if (iequals(proto->type, KVTYPE_SLEWTO)) {
			(*it)->Slewto(boost::static_pointer_cast<KVSlewto>(proto));
		}
		else if (iequals(proto->type, KVTYPE_PARK)) {
			(*it)->Park();
		}
		else if (iequals(proto->type, KVTYPE_GUIDE)) {
			(*it)->Guide(boost::static_pointer_cast<KVGuide>(proto));
		}
		else if (iequals(proto->type, KVTYPE_HOME)) {
			(*it)->FindHome();
		}
		else if (iequals(proto->type, KVTYPE_SYNC)) {
			(*it)->HomeSync(boost::static_pointer_cast<KVSync>(proto));
		}
		// 单独设备指令: 相机
		else if (iequals(proto->type, KVTYPE_TKIMG)) {
			(*it)->TakeImage(boost::static_pointer_cast<KVTakeImage>(proto));
		}
		else if (iequals(proto->type, KVTYPE_CAMSET)) {
		}
		// 单独设备指令: 调焦
		else if (iequals(proto->type, KVTYPE_FOCUS)) {
			(*it)->Focus(boost::static_pointer_cast<KVFocus>(proto));
		}
		// 单独设备指令: 消旋器
		else if (iequals(proto->type, KVTYPE_DEROT)) {

		}
		// 单独设备指令: 圆顶
		else if (iequals(proto->type, KVTYPE_DOME)) {

		}
		// 单独设备指令: 镜盖
		else if (iequals(proto->type, KVTYPE_MCOVER)) {

		}
		// 单独设备指令: 滤光片
		else if (iequals(proto->type, KVTYPE_FILTER)) {

		}
		// 单独设备指令: 测站位置
		else if (iequals(proto->type, KVTYPE_GEOSITE)) {

		}
	}
}

// 处理图像协议: 转台
void GeneralControl::process_protocol_mount(TcpClient* cliptr, NonKVBasePtr proto) {
	TcpCPtr sp = tcpCliMount_.Find(cliptr);
	string gid = proto->gid;
	int imin = boost::iequals(gid, "001") ? 0 : 4;
	int imax = imin == 0 ? 3 : 9;

	if (iequals(proto->type, NONKVTYPE_STATE)) {
		boost::shared_ptr<NonKVStatus> status = boost::static_pointer_cast<NonKVStatus>(proto);
		boost::format fmt("%03d");
		for (int i = imin; i < status->n && i <= imax; ++i) {
			ObssPtr obss = find_obss(gid, (fmt % (i + 1)).str());
			obss->CoupleMount(sp);
			obss->NotifyMountState(status->state[i]);
		}
	}
	else if (iequals(proto->type, NONKVTYPE_POS)) {
		boost::shared_ptr<NonKVPosition> pos = boost::static_pointer_cast<NonKVPosition>(proto);
		ObssPtr obss = find_obss(gid, pos->uid);
		obss->NotifyMountPosition(*pos);
	}
}

// 处理图像协议: 相机
void GeneralControl::process_protocol_camera(TcpClient* cliptr, KVBasePtr proto) {
	if (iequals(proto->type, KVTYPE_CAMERA)) {
		ObssPtr obss = find_obss(proto->gid, proto->uid);
		TcpCPtr ptrTcp = tcpCliCamera_.Pop(cliptr);
		obss->CoupleCamera(ptrTcp, proto->cid);
	}
}

// 处理图像协议: 转台
void GeneralControl::process_protocol_focus(TcpClient* cliptr, NonKVBasePtr proto) {
	TcpCPtr sp = tcpCliFocus_.Find(cliptr);
	string gid = proto->gid;

	if (iequals(proto->type, NONKVTYPE_FOCUS)) {
		boost::shared_ptr<NonKVFocus> focus = boost::static_pointer_cast<NonKVFocus>(proto);
		boost::format fmt("%03d");
		ObssPtr obss = find_obss(gid, proto->uid);
		obss->CoupleFocus(sp);
		for (int i = 0; i < 5; ++i) {
			obss->NotifyFocus((fmt % (i + 1)).str(), focus->pos[i]);
		}
	}
}

/*!
 * @brief 查找与gid和uid匹配的观测系统
 * @param gid 组标志
 * @param uid 单元标志
 * @return
 * 匹配的观测系统访问接口
 * @note
 * 若观测系统不存在, 则先创建该系统
 */
ObssPtr GeneralControl::find_obss(const string& gid, const string& uid) {
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
		if (!obss->Start()) obss.reset();
		else {
			_gLog.Write("Observation System <%s:%s> goes running", gid.c_str(), uid.c_str());
			const ObservationSystem::PlanCBSlot& slot = boost::bind(&GeneralControl::plan_state, this, _1);
			obss->RegisterPlanCallback(slot);
			obssVec_.push_back(obss);
		}
	}
	return obss;
}

/**
 * @brief 线程: 定时向客户端上传系统工作状态
 */
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
			// 状态: 相机
			KVFocus focus;
			focus.UpdateUTC();
			focus.gid = (*it)->GetInfoMount().gid;
			focus.uid = (*it)->GetInfoMount().uid;
			focus.opType = 0;

			const ObservationSystem::CameraInfoVector& camera = (*it)->GetInfoCamera();
			for (auto it1 = camera.begin(); it1 != camera.end(); ++it1) {// 遍历相机
				string info = (*it1).info.ToString();
				tcpCliClient_.Write(info.c_str(), info.size());

				if ((*it1).focPos != INT_MAX) {// 焦点位置
					focus.cid    = it1->info.cid;
					focus.state  = it1->focState;
					focus.pos    = it1->focPos;
					focus.posTar = it1->focTar;
					info = focus.ToString();
					tcpCliClient_.Write(info.c_str(), info.size());
				}
			}
		}
	}
}

/**
 * @brief 回调函数: 向客户端发送观测计划状态
 * @param plan  观测计划状态
 */
void GeneralControl::plan_state(KVPlanPtr plan) {
	if (tcpCliClient_.Size()) {
		string msg = plan->ToString();
		tcpCliClient_.Write(msg.c_str(), msg.size());
	}
}

/**
 * @brief 线程: 定时清理无效观测系统
 */
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
