
#include <boost/bind/bind.hpp>
#include <boost/bind/placeholders.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include "ObservationSystem.h"
#include "GLog.h"

using namespace boost;
using namespace boost::placeholders;
using namespace boost::posix_time;

ObservationSystem::ObservationSystem(const string& gid, const string& uid)
	: nonkvproto_(gid, uid) {
	gid_ = gid;
	uid_ = uid;
	plan_state_.reset(new KVPlan);
	plan_state_->gid = gid;
	plan_state_->uid = uid;

	mountInfo_.gid = gid_;
	mountInfo_.uid = uid_;

	lastClosed_ = second_clock::universal_time();
}

ObservationSystem::~ObservationSystem() {
}

/*!
 * @brief 启动观测系统
 * @return
 * 操作结果. false代表失败
 */
bool ObservationSystem::Start() {
	if (gid_.empty() || uid_.empty()) {
		_gLog.Write(LOG_FAULT, "failed to create observation system<%s:%s>",
			gid_.c_str(), uid_.c_str());
		return false;
	}
	string name = "msgque_obss_";
	name += gid_ + "_";
	name += uid_;

	if (MessageQueue::Start(name.c_str())) {
		thrdObsplan_ = Thread(boost::bind(&ObservationSystem::thread_obsplan, this));
		return true;
	}
	return false;
}

/**
 * @brief 停止观测系统
 */
void ObservationSystem::Stop() {
	interrupt_thread(thrdObsplan_);
	MessageQueue::Stop();
}

/*!
 * @brief 检查观测系统唯一性标志是否与输入条件一致
 * @param gid 组标志
 * @param uid  单元标志
 * @return
 * 匹配一致则返回true, 否则返回false
 */
bool ObservationSystem::IsMatched(const string& gid, const string& uid) {
	return ((gid.empty() && uid.empty())					// 全为空时操作对所有观测系统有效
			|| (uid.empty() && iequals(gid, gid_))			// 单元标志为空时, 组标志必须相同
			|| (iequals(gid, gid_) && iequals(uid, uid_))	// 全不为空时, 二者必须相同
			);												// 不接受单元标志相同, 而组标志相同
}

// 查看转台状态
const KVMount& ObservationSystem::GetInfoMount() {
	return mountInfo_;
}

// 查看相机状态
const ObservationSystem::CameraInfoVector& ObservationSystem::GetInfoCamera() {
	return camInfoVec_;
}

// 注册回调函数: 观测计划状态
void ObservationSystem::RegisterPlanCallback(const PlanCBSlot& slot) {
	cbfPlan_.disconnect_all_slots();
	cbfPlan_.connect(slot);
}

/**
 * @brief 计算当前时间与设备最后关闭时间的差异
 * @param now  当前UTC时间
 * @return
 * 秒数, now - lastClosed
 */
int ObservationSystem::LastClosed(ptime &now) {
	if (tcpMount_.use_count() || tcpFocus_.use_count()) return 0;
	for (auto it = camInfoVec_.begin(); it != camInfoVec_.end(); ++it) {
		if (it->ptrTcp.use_count()) return 0;
	}
	return int((now - lastClosed_).total_seconds());
}

/**
 * @brief 关联观测系统和转台TCP连接
 * @param ptrTcp  TCP连接
 * @return 关联结果
 */
void ObservationSystem::CoupleMount(TcpCPtr ptrTcp) {
	if (tcpMount_ != ptrTcp) {
		_gLog.Write("Mount<%s:%s> is on-line", gid_.c_str(), uid_.c_str());
		tcpMount_ = ptrTcp;
		countMountPos_ = 0;
		mountInfo_.state = MOUNT_MIN;
	}
}

/**
 * @brief 解除观测系统和转台TCP连接
 * @param ptrTcp TCP连接
 */
void ObservationSystem::DecoupleMount(TcpCPtr ptrTcp) {
	if (tcpMount_ == ptrTcp) {
		_gLog.Write("Mount<%s:%s> is off-line", gid_.c_str(), uid_.c_str());
		tcpMount_.reset();
		mountInfo_.state = MOUNT_ERROR;

		lastClosed_ = second_clock::universal_time();
	}
}

/**
 * @brief 关联观测系统和调焦TCP连接
 * @param ptrTcp  TCP连接
 * @return 关联结果
 */
void ObservationSystem::CoupleFocus(TcpCPtr ptrTcp) {
	if (tcpFocus_ != ptrTcp) {
		_gLog.Write("Focus<%s:%s> is on-line", gid_.c_str(), uid_.c_str());
		tcpFocus_ = ptrTcp;
	}
}

/**
 * @brief 解除观测系统和调焦TCP连接
 * @param ptrTcp TCP连接
 */
void ObservationSystem::DecoupleFocus(TcpCPtr ptrTcp) {
	if (tcpFocus_ == ptrTcp) {
		_gLog.Write("Focus<%s:%s> is off-line", gid_.c_str(), uid_.c_str());
		tcpFocus_.reset();
		lastClosed_ = second_clock::universal_time();
	}
}

/**
 * @brief 关联观测系统和相机TCP连接
 * @param ptrTcp  TCP连接
 * @param cid     相机标志
 * @return 关联结果
 */
bool ObservationSystem::CoupleCamera(TcpCPtr ptrTcp, const string& cid) {
	// 检查是否重复关联
	bool found(false);
	for (auto it = camInfoVec_.begin(); it != camInfoVec_.end(); ++it) {
		if (iequals((*it).info.cid, cid)) {
			if ((*it).ptrTcp.use_count()) {
				_gLog.Write(LOG_FAULT, "OBSS<%s:%s> had related camera <%s>",
						gid_.c_str(), uid_.c_str(), cid.c_str());
				return false;
			}
			else {
				found = true;
				(*it).ptrTcp = ptrTcp;
				// 焦点位置
				if (it->focState == 0) {
					KVFocus proto;
					proto.gid = gid_;
					proto.uid = uid_;
					proto.cid = cid;
					proto.pos    = it->focPos;
					proto.posTar = it->focTar;
					string rsp = proto.ToString();
					ptrTcp->Write(rsp.c_str(), rsp.size());
				}
				break;
			}
		}
	}

	_gLog.Write("Camera<%s:%s:%s> is on-line", gid_.c_str(), uid_.c_str(), cid.c_str());
	sysState_.CameraOnline();
	if (!found) {// 建立关联关系
		CameraInfo nfcam;
		nfcam.ptrTcp = ptrTcp;
		nfcam.info.gid = gid_;
		nfcam.info.uid = uid_;
		nfcam.info.cid = cid;
		camInfoVec_.push_back(nfcam);
	}
	// 更新回调函数
	const TcpClient::CBSlot& slot = boost::bind(&ObservationSystem::camera_receive, this, _1, _2);
	ptrTcp->RegisterRead(slot);
	// 在断点恢复计划
	if (found && plan_.unique()) {
		string cmd = plan_->ToString();
		ptrTcp->Write(cmd.c_str(), cmd.size());

		if (mountInfo_.state == MOUNT_TRACKING) {
			int frmno(0);
			for (auto it = camInfoVec_.begin(); it != camInfoVec_.end(); ++it) {// 从最大序号恢复曝光
				if (frmno < (*it).info.frmno) frmno = (*it).info.frmno;
			}
			expose2camera(EXP_START, frmno >= 0 ? frmno : 0, cid.c_str());
		}
	}

	return true;
}

/**
 * @brief 通知转台状态
 * @param state  转台状态
 */
void ObservationSystem::NotifyMountState(int state) {
	if (state == mountInfo_.state) return;
	if (state > MOUNT_MIN && state < MOUNT_MAX) {
		_gLog.Write("Mount<%s:%s> state is %s", gid_.c_str(), uid_.c_str(),
			DESC_STATE_MOUNT[state]);

		// 条件:
		// 1: 转台跟踪
		// 2: 正在执行计划
		if (state == MOUNT_TRACKING && mountInfo_.state != MOUNT_MIN) {
			_gLog.Write("Mount<%s:%s> arrived at <%.4f, %.4f>[degree]",
					gid_.c_str(), uid_.c_str(), mountInfo_.ra, mountInfo_.dec);
			if (plan_.use_count()) {
				if (mountInfo_.state == MOUNT_GUIDING) {
					KVGuide proto;
					string cmd = proto.ToString();
					write2camera(cmd.c_str(), cmd.size());
				}
				else if (mountInfo_.state == MOUNT_SLEWING) expose2camera(EXP_START);
			}
		}
		mountInfo_.state = state;
		mountInfo_.UpdateUTC();
	}
	else {
		_gLog.Write(LOG_WARN, "Mount<%s:%s> received undefined state [%d]",
			gid_.c_str(), uid_.c_str(), state);
	}
}

/**
 * @brief 通知转台位置
 * @param pos  转台位置
 */
void ObservationSystem::NotifyMountPosition(const NonKVPosition& pos) {
	mountInfo_.ra  = pos.ra;
	mountInfo_.dec = pos.dec;
	if (countMountPos_ % 200 == 0) {
		try {
			ptime utc = from_iso_extended_string(pos.utc);
			ptime now = second_clock::universal_time();
			int64_t bias = (utc - now).total_seconds();
			if (fabs(bias) >= 5) {
				_gLog.Write(LOG_WARN, "Mount<%s:%s> clock is %s for %lld seconds",
					gid_.c_str(), uid_.c_str(),
					bias > 0 ? "faster" : "slower",
					bias);
			}
		}
		catch(...) {
		}
	}
	++countMountPos_;
}

/**
 * @brief 更新焦点位置
 * @param cid  相机标志
 * @param pos  焦点位置
 */
void ObservationSystem::NotifyFocus(const string& cid, int pos) {
	for (auto it = camInfoVec_.begin(); it != camInfoVec_.end(); ++it) {
		if (iequals((*it).info.cid, cid)) {
			int state(it->focState);
			if (pos != (*it).focPos) {
				(*it).focPos = pos;
				(*it).repeat = 0;
			}
			else if ((*it).focState && ++(*it).repeat >= 3) {// 响应调焦指令
				if (it->focState > 0 && pos != (*it).focTar) {
					_gLog.Write(LOG_WARN, "Focus<%s:%s:%s> position<%d> differs from target<%d>",
						gid_.c_str(), uid_.c_str(), cid.c_str(), pos, (*it).focTar);
				}
				(*it).focState = 0;
				// 通知相机
				KVFocus proto;
				proto.gid = gid_;
				proto.uid = uid_;
				proto.cid = cid;
				proto.pos    = pos;
				proto.posTar = it->focTar;
				string rsp = proto.ToString();
				if ((*it).ptrTcp.use_count()) (*it).ptrTcp->Write(rsp.c_str(), rsp.size());
			}
			if (state != it->focState) {
				_gLog.Write("Focus<%s:%s:%s> position is %d", gid_.c_str(), uid_.c_str(),
					cid.c_str(), pos);
			}

			break;
		}
	}
}

// 通知: 观测计划
void ObservationSystem::NotifyPlan(KVBasePtr proto) {
	KVAppGWACPtr plan = boost::static_pointer_cast<KVAppGWAC>(proto);
	_gLog.Write("new plan<%s:%s> %s", gid_.c_str(), uid_.c_str(), plan->ToString().c_str());
	// 更新计划状态
	KVPlanPtr plan_state(new KVPlan);
	plan_state->UpdateUTC();
	plan_state->plan_sn = plan->plan_sn;
	plan_state->state = OBSPLAN_CATALOGED;
	cbfPlan_(plan_state);
	// 处理计划
	if (plan_.unique()) {// 停止当前计划
		Abort();
		if (sysState_.AnyExposing()) plan_wait_ = plan;
		else {
			plan_ = plan;
			PostMessage(MSG_NEW_PLAN);
		}
	}
	else {
		plan_ = plan;
		PostMessage(MSG_NEW_PLAN);
	}
}

// 通知: 检查计划状态
KVPlanPtr ObservationSystem::CheckPlan(const string& plan_sn) {
	KVPlanPtr plan;
	if (iequals(plan_state_->plan_sn, plan_sn)) plan = plan_state_;
	return plan;
}

// 通知: 删除计划
bool ObservationSystem::RemovePlan(const string& plan_sn) {
	if (iequals(plan_state_->plan_sn, plan_sn)) {
		// 转台
		// 相机
		return true;
	}
	return false;
}

// 通知: 中断当前计划/指向/曝光
void ObservationSystem::Abort() {
	_gLog.Write("Abort OBSS<%s:%s> current operations", gid_.c_str(), uid_.c_str());
	if (tcpMount_.use_count()) {// 转台
		string cmd = nonkvproto_.AbortSlew();
		tcpMount_->Write(cmd.c_str(), cmd.size());
		// 存储目标位置
		mountInfo_.objra = 1000.0;
		mountInfo_.objdec = 1000.0;
	}
	if (sysState_.AnyExposing()) {
		expose2camera(EXP_STOP);
	}
	if (plan_.unique()) {// 计划
		_gLog.Write("plan<%s> is aborted", plan_->plan_sn.c_str());
		plan_.reset();

		// 更新计划状态
		plan_state_->UpdateUTC();
		plan_state_->tm_stop = plan_state_->utc;
		plan_state_->state = OBSPLAN_INTERRUPTED;
		cbfPlan_(plan_state_);
	}
}

// 通知: 指向
void ObservationSystem::Slewto(KVSlewPtr proto) {
	if (plan_.unique()) {
		_gLog.Write("plan<%s> in OBSS<%s:%s> rejects command slew",
			plan_->plan_sn.c_str(), gid_.c_str(), uid_.c_str());
	}
	else if (tcpMount_.use_count()) {
		_gLog.Write("Mount<%s:%s> points to <%.4f, %.4f>[degree]",
			gid_.c_str(), uid_.c_str(),
			proto->ra, proto->dec);
		string cmd = nonkvproto_.Slew(proto->ra, proto->dec);
		tcpMount_->Write(cmd.c_str(), cmd.size());
		// 存储目标位置
		mountInfo_.objra = proto->ra;
		mountInfo_.objdec = proto->dec;
	}
}

// 通知: 复位
void ObservationSystem::Park() {
	_gLog.Write("Parking Mount<%s:%s>", gid_.c_str(), uid_.c_str());
	// 转台
	if (tcpMount_.use_count()
			&& mountInfo_.state != MOUNT_PARKED
			&& mountInfo_.state != MOUNT_PARKING) {
		string cmd = nonkvproto_.Park();
		tcpMount_->Write(cmd.c_str(), cmd.size());
		// 存储目标位置
		mountInfo_.objra = 1000.0;
		mountInfo_.objdec = 1000.0;
	}
	if (sysState_.AnyExposing()) {// 相机
		_gLog.Write("abort exposing <%s:%s>", gid_.c_str(), uid_.c_str());
		expose2camera(EXP_STOP);
	}
	if (plan_.unique()) {// 计划
		_gLog.Write("plan<%s> is aborted", plan_->plan_sn.c_str());
		plan_.reset();
		// 更新计划状态
		plan_state_->UpdateUTC();
		plan_state_->tm_stop = plan_state_->utc;
		plan_state_->state = OBSPLAN_INTERRUPTED;
		cbfPlan_(plan_state_);
	}
}

// 通知: 导星
void ObservationSystem::Guide(KVGuidePtr proto) {
	_gLog.Write("Guide<%s:%s>: result = %d, op = %d, ra = %d, dec = %d",
		gid_.c_str(), uid_.c_str(),
		proto->result, proto->op,
		int(proto->ra + 0.5), int (proto->dec + 0.5));
	if (!proto->result && tcpMount_.use_count()) {// 导星, 通知转台
		string cmd = nonkvproto_.Guide(proto->ra, proto->dec);
		tcpMount_->Write(cmd.c_str(), cmd.size());
	}
	// 通知相机
	proto->op = proto->result ? 0 : 1;
	string cmd = proto->ToString();
	write2camera(cmd.c_str(), cmd.size());
}

// 通知: 搜索零点
void ObservationSystem::FindHome() {
	if (tcpMount_.use_count()) {
		_gLog.Write("Mount<%s:%s> find home", gid_.c_str(), uid_.c_str());
		string cmd = nonkvproto_.FindHome();
		tcpMount_->Write(cmd.c_str(), cmd.size());
		// 存储目标位置
		mountInfo_.objra = 1000.0;
		mountInfo_.objdec = 1000.0;
	}
}

// 通知: 同步零点
void ObservationSystem::HomeSync(KVSyncPtr proto) {
	if (tcpMount_.use_count()) {
		_gLog.Write("Mount<%s:%s> home sync to <%.4lf %.4lf>",
			gid_.c_str(), uid_.c_str(),
			proto->ra, proto->dec);
	}
}

// 通知: 曝光
void ObservationSystem::TakeImage(KVTkImgPtr proto0) {
	if (plan_.unique()) {
		_gLog.Write("plan<%s> in OBSS<%s:%s> rejects command slew",
			plan_->plan_sn.c_str(), gid_.c_str(), uid_.c_str());
	}
	else {
		KVAppGWACPtr proto(new KVAppGWAC);
		*proto = *proto0;
		// 补全协议缺失项
		if (proto->plan_sn.empty()) {
			ptime utc = second_clock::universal_time();
			ptime::date_type day = utc.date();
			if (day.day() != oldDay_) {
				oldDay_ = day.day();
				planSN_ = 0;
			}
			boost::format fmt("%02d%02d%02d%03d");
			fmt % (day.year() - 2000) % day.month().as_number() % day.day() % ++planSN_;
			proto->plan_sn = gid_ + uid_ + "_" + fmt.str();
		}
		if (proto->exptime < 0) proto->exptime = 0.0;
		if (proto->imgtype.empty()) proto->imgtype = proto->exptime == 0.0 ? "bias" : "object";
		if (proto->objid.empty())   proto->objid = proto->imgtype;
		if (proto->frmcnt <= 0) proto->frmcnt = 1;
		proto->type = KVTYPE_APPGWAC; // 修改指令字
		_gLog.Write("TakeImage<%s:%s>: imgtype = %s, exptime = %.3lf, frmcnt = %d",
			gid_.c_str(), uid_.c_str(),
			proto->imgtype.c_str(), proto->exptime, proto->frmcnt);
		// 相机指令
		string cmd = proto->ToString();
		write2camera(cmd.c_str(), cmd.size(), proto->cid.c_str());
		expose2camera(EXP_START, 0, proto->cid.c_str());
		// 更新协议状态
		plan_ = proto;
		plan_state_->UpdateUTC();
		plan_state_->plan_sn = proto->plan_sn;
		plan_state_->tm_start = plan_state_->utc;
		plan_state_->state = OBSPLAN_RUNNING;
		cbfPlan_(plan_state_);
	}
}

// 通知: 相机配置参数
KVCamSetPtr ObservationSystem::CameraSetting(KVCamSetPtr proto) {

	return proto;
}

// 通知: 调焦
void ObservationSystem::Focus(KVFocusPtr proto) {
	_gLog.Write("%s", proto->ToString().c_str());
	if (!tcpFocus_.use_count()) {
		_gLog.Write(LOG_FAULT, "Focuser<%s:%s> is not on-line", gid_.c_str(), uid_.c_str());
	}
	else if (proto->opType == 1 && proto->relPos) {
		string cid = proto->cid;
		bool found(false);
		int posNow, posTar;
		for (auto it = camInfoVec_.begin(); it != camInfoVec_.end() && !found; ++it) {
			if (iequals((*it).info.cid, cid)) {
				found = true;
				posNow = (*it).focPos;
				(*it).focTar = posTar = posNow + proto->relPos;
				(*it).focState = 1;
				(*it).repeat = 0;
			}
		}
		if (!found) {
			_gLog.Write(LOG_FAULT, "Camera<%s:%s:%s> off-line rejects focus",
				gid_.c_str(), uid_.c_str(), cid.c_str());
		}
		else {
			_gLog.Write("Focus<%s:%s:%s> try to move from<%d> to<%d>",
				gid_.c_str(), uid_.c_str(), cid.c_str(),
				posNow, posTar);

			string cmd = nonkvproto_.Focus(cid, proto->relPos);
			tcpFocus_->Write(cmd.c_str(), cmd.size());
		}
	}
}

// 通知: 消旋器
void ObservationSystem::Derot(KVDerotPtr proto) {

}

// 通知: 圆顶
void ObservationSystem::Dome(KVDomePtr proto) {

}

/*!
 * @brief 注册消息响应函数
 */
void ObservationSystem::register_messages() {
	const CBSlot& slot1 = boost::bind(&ObservationSystem::on_camera_close,   this, _1, _2);
	const CBSlot& slot2 = boost::bind(&ObservationSystem::on_camera_receive, this, _1, _2);
	const CBSlot& slot3 = boost::bind(&ObservationSystem::on_new_plan,    this, _1, _2);
	const CBSlot& slot4 = boost::bind(&ObservationSystem::on_flat_reslew, this, _1, _2);

	RegisterMessage(MSG_CAMERA_CLOSE,   slot1);
	RegisterMessage(MSG_CAMERA_RECEIVE, slot2);
	RegisterMessage(MSG_NEW_PLAN,    slot3);
	RegisterMessage(MSG_FLAT_RESLEW, slot4);
}

void ObservationSystem::camera_receive(TcpClient* cliptr, boost::system::error_code ec) {
	PostMessage(!ec ? MSG_CAMERA_RECEIVE : MSG_CAMERA_CLOSE, (const long) cliptr);
}

// 关闭相机TCP连接
void ObservationSystem::on_camera_close(const long connptr, const long peer_type) {
	TcpClient *ptrTcp = (TcpClient*) connptr;
	for (auto it = camInfoVec_.begin(); it != camInfoVec_.end(); ++it) {
		if ((*it).ptrTcp.get() == ptrTcp) {
			_gLog.Write("Camera<%s:%s:%s> is off-line", gid_.c_str(), uid_.c_str(),
				(*it).info.cid.c_str());
			if ((*it).info.state > CAMCTL_IDLE) sysState_.LeaveExpose();
			sysState_.CameraOnline(false);
			(*it).info.state = CAMCTL_ERROR;
			(*it).info.errcode = 1;
			(*it).ptrTcp.reset();

			lastClosed_ = second_clock::universal_time();
			break;
		}
	}
}

// 接收到相机TCP信息
void ObservationSystem::on_camera_receive(const long connptr, const long peer_type) {
	const char term[] = "\n"; // 结束符
	const int len = strlen(term); // 结束符长度
	char buff[TCP_PACK_SIZE];
	int pos, to_read;
	TcpClient* ptrTcp = (TcpClient*) connptr;

	while ((pos = ptrTcp->Lookup(term, len)) >= 0) {
		if ((to_read = pos + len) > TCP_PACK_SIZE) {// 信息长度超过预设最大值
			_gLog.Write(LOG_FAULT, "protocol length from camera is over than threshold");
			ptrTcp->Close();
		}
		else {
			ptrTcp->Read(buff, pos + len);
			buff[pos] = '\0';

			KVBasePtr proto = kvproto_.Resolve(buff);
			if (proto.unique()) {
				process_protocol_camera(ptrTcp, proto);
			}
			else {
				_gLog.Write(LOG_FAULT, "undefined protocol from camera");
				ptrTcp->Close();
			}
		}
	}
}

void ObservationSystem::on_new_plan(const long, const long) {
	if (!(tcpMount_.use_count() && sysState_.camonline)) {
		cvNewPlan_.notify_one();
	}
	else {
		bool slew_req = !(iequals(plan_->imgtype, "bias") || iequals(plan_->imgtype, "dark"));
		if (slew_req) {// 通知转台指向
			double errra = (plan_->ra - mountInfo_.objra) * 3600.0;
			double errdec = (plan_->dec - mountInfo_.objdec) * 3600.0;
			slew_req = fabs(errra) > 5.0 || fabs(errdec) > 5.0;
		}
		if (slew_req) {
			_gLog.Write("Plan<%s> in OBSS<%s:%s> slewes to <%.4lf %.4lf>",
				plan_->plan_sn.c_str(),
				gid_.c_str(), uid_.c_str(),
				plan_->ra, plan_->dec);

			string cmd = nonkvproto_.Slew(plan_->ra, plan_->dec);
			tcpMount_->Write(cmd.c_str(), cmd.size());
			mountInfo_.objra  = plan_->ra;
			mountInfo_.objdec = plan_->dec;
		}
		// 通知相机
		string cmd = plan_->ToString();
		write2camera(cmd.c_str(), cmd.size());
		if (!slew_req) {// 立即开始曝光
			expose2camera(EXP_START);
		}
		// 更新计划状态
		plan_state_->UpdateUTC();
		plan_state_->plan_sn = plan_->plan_sn;
		plan_state_->tm_start = plan_state_->utc;
		plan_state_->state = OBSPLAN_RUNNING;
		cbfPlan_(plan_state_);
	}
}

// 平场: 重新指向
void ObservationSystem::on_flat_reslew(const long, const long) {

}

// 处理图像协议: 相机
void ObservationSystem::process_protocol_camera(TcpClient* cliptr, KVBasePtr proto) {
	// 解析通信协议
	int state_new(CAMCTL_ERROR), state_old(CAMCTL_ERROR);
	KVCamPtr camera;
	KVCamSetPtr camset;
	if (iequals(proto->type, KVTYPE_CAMERA)) {
		camera = boost::static_pointer_cast<KVCamera>(proto);
		state_new = camera->state;
	}
	else if (iequals(proto->type, KVTYPE_CAMSET)) {
		camset = boost::static_pointer_cast<KVCamSet>(proto);
	}
	// 更新相机状态
	for (auto it = camInfoVec_.begin(); it != camInfoVec_.end(); ++it) {
		if ((*it).ptrTcp.get() == cliptr) {
			if (camera.use_count()) {
				state_old = (*it).info.state;
				(*it).info = *camera;
			}
			else if (camset.use_count()) {
			}

			break;
		}
	}
	// 检查相机工作状态
	if (state_new != state_old) {
		if (state_new <= CAMCTL_IDLE && state_old > CAMCTL_IDLE) {
			if (sysState_.LeaveExpose()) {
				if (plan_.unique()) {// 清理计划
					_gLog.Write("plan<%s> is over", plan_->plan_sn.c_str());
					plan_.reset();
					// 更新计划状态
					plan_state_->UpdateUTC();
					plan_state_->tm_stop = plan_state_->utc;
					plan_state_->state = OBSPLAN_OVER;
					cbfPlan_(plan_state_);
				}
				if (plan_wait_.unique()) {// 执行新计划
					plan_ = plan_wait_;
					plan_wait_.reset();
					PostMessage(MSG_NEW_PLAN);
				}
			}
		}
		else if (state_new > CAMCTL_IDLE && state_old <= CAMCTL_IDLE) sysState_.EnterExpose();
		// 特殊处理: 平场
		else if (state_new == CAMCTL_WAIT_FLAT) {
			sysState_.EnterWaitFlat();
			if (sysState_.exposing == sysState_.waitflat) {// 重新指向
				PostMessage(MSG_FLAT_RESLEW);
			}
		}
		else if (state_old == CAMCTL_WAIT_FLAT) sysState_.LeaveWaitFlat();
	}
}

// 将指定协议发送给相机
void ObservationSystem::write2camera(const char* cmd, int n, const char* cid) {
	bool empty = !cid || iequals(cid, "");
	for (auto it = camInfoVec_.begin(); it != camInfoVec_.end(); ++it) {
		if ((*it).ptrTcp.use_count()
				&& (empty || iequals((*it).info.cid, cid))) {
			(*it).ptrTcp->Write(cmd, n);
			if (!empty) break;
		}
	}
}

// 将指定曝光协议发送给相机
void ObservationSystem::expose2camera(int cmd, int frmno, const char* cid) {
	KVExpose proto;
	proto.command = cmd;
	proto.frmno = frmno;
	string output = proto.ToString();
	write2camera(output.c_str(), output.size(), cid);
}

// 线程: 监测观测计划
void ObservationSystem::thread_obsplan() {
	boost::mutex mtx;
	MtxLck lck(mtx);
	boost::chrono::seconds period(10);

	while (1) {
		cvNewPlan_.wait(lck);
		boost::this_thread::sleep_for(period);
		if (plan_.unique() && plan_state_->state != OBSPLAN_RUNNING)
			PostMessage(MSG_NEW_PLAN);
	}
}
