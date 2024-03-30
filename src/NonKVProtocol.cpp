/*
 * @file mountproto.cpp 封装与转台相关的通信协议
 * @author         卢晓猛
 * @version        2.0
 * @date           2017年2月20日
 */

#include <string.h>
#include <ctype.h>
#include <typeinfo>
#include <boost/utility/string_ref.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "NonKVProtocol.h"
#include "GLog.h"

using namespace boost;
using namespace boost::posix_time;

NonKVProtocol::NonKVProtocol() {
	gid_ = uid_ = "";
}

NonKVProtocol::NonKVProtocol(const string& gid, const string& uid) {
	gid_ = gid;
	uid_ = uid;
}

NonKVProtocol::~NonKVProtocol() {
}

// 解析焦点位置
int NonKVProtocol::resolve_focus(const char* id) {
	if (iequals(id, "es")) return 0;
	if (iequals(id, "ws")) return 1;
	if (iequals(id, "wn")) return 2;
	if (iequals(id, "en")) return 3;
	if (iequals(id, "mid")) return 4;
	return -1;
}

int NonKVProtocol::increase_serno() {
	int serno = serno_;
	if (++serno_ == 100000) serno_ = 1;
	return serno;
}

NonKVBasePtr NonKVProtocol::Resolve(const char* rcvd) {
	NonKVBasePtr proto;
	string_ref sref(rcvd);
	string prefix("g#");				// 定义引导符
	string suffix("%");					// 定义结束符: 同时还是中间符!!!
	string type_state(NONKVTYPE_STATE);	// 定义协议: status
	string type_pos(NONKVTYPE_POS);		// 定义协议: currentpos
	string type_focus(NONKVTYPE_FOCUS);	// 定义协议: focus

	char sep = '%';			// 数据间分隔符
	int group_len = 3;		// 约定: 组标志长度为3字节
	int unit_len = 3;		// 约定: 单元标志长度为3字节
	int rsp_len = 3;		// 约定: 回馈指令特征字"Rec"长度为3字节
	int n(sref.length() - suffix.length()), pos, i(0), j;
	char ch, buff[20], buff1[20];

	if (!sref.starts_with(prefix) || !sref.ends_with(suffix)) {
		_gLog.Write(LOG_FAULT, "%s:%s, illegal protocol[%s]",
			typeid(this).name(), __FUNCTION__, rcvd);
	}
	else if ((pos = sref.find("Rec")) > 0) {// 指令回馈
	// g#001001trackRec%YYYY-MM-DD%hh:mm:ss%xxxxxx%
	// g#002008fwhm0810024T101756000Rec%2024-03-28%10:17:56%%00001%
		boost::shared_ptr<NonKVResponse> body(new NonKVResponse);
		for (i = prefix.length(), j = 0; j < group_len; ++i, ++j) body->gid += sref.at(i);
		for (j = 0; j < unit_len; ++i, ++j) body->uid += sref.at(i);
		i = pos + rsp_len;
		if (sref.at(i) != sep) --i; // 容错:g#001001trackRecYYYY-MM-DD%hh:mm:ss%xxxxxx%
		proto = boost::static_pointer_cast<NonKVBase>(body);
	}
	else if ((pos = sref.find("finished")) > 0) {// 指令回馈: 调焦到位
	}
	else if ((pos = sref.find(type_state)) > 0) {// state
	// g#002status0000555755%2024-03-29%13:07:26%32846%
		boost::shared_ptr<NonKVStatus> body(new NonKVStatus);
		for (i = prefix.length(); i < pos; ++i) body->gid += sref.at(i);
		for (i = pos + type_state.length(), j = 0; i < n && sref.at(i) != sep; ++i, ++j, ++body->n) body->state[j] = sref.at(i) - '0';
		proto = boost::static_pointer_cast<NonKVBase>(body);
	}
	else if ((pos = sref.find(type_pos)) > 0) {// currentpos
		boost::shared_ptr<NonKVPosition> body(new NonKVPosition);
		int j1(0), j2(0);
		pos -= unit_len;
		for (i = prefix.length(); i < pos; ++i) body->gid += sref.at(i);
		pos += unit_len;
		for (; i < pos; ++i) body->uid += sref.at(i);
		for (i = pos + type_pos.length(), j1 = 0; i < n && sref.at(i) != sep; ++i, ++j1) buff[j1] = sref.at(i);
		buff[j1] = '\0';
		body->ra = atoi(buff) * 1E-4;
		for (++i, j2 = 0; i < n && sref.at(i) != sep; ++i, ++j2) buff[j2] = sref.at(i);
		buff[j2] = '\0';
		body->dec = atoi(buff) * 1E-4;
		if (j1 && j2) proto = boost::static_pointer_cast<NonKVBase>(body);
	}
	else if ((pos = sref.find(type_focus)) > 0) {// focus
		// sample: g#002006focuses+0010en-0030ws+0020wn-0025mid+0015%
		boost::shared_ptr<NonKVFocus> body(new NonKVFocus);
		int j1(0), j2(0), index(-1);
		pos -= unit_len;
		for (i = prefix.length(); i < pos; ++i) body->gid += sref.at(i);
		pos += unit_len;
		for (; i < pos; ++i) body->uid += sref.at(i);
		for (i = pos + type_focus.length(); i < n; ++i) {
			if ((ch = sref.at(i)) == sep) {
				if (j1 && j2) {
					buff[j1]  = '\0';
					buff1[j2] = '\0';
					index = resolve_focus(buff);
					if (index >= 0) body->pos[index] = atoi(buff1);
				}
				break;
			}
			if (isalpha(ch)) {
				if (j1 && j2) {
					buff[j1]  = '\0';
					buff1[j2] = '\0';
					j1 = j2 = 0;
					if ((index = resolve_focus(buff)) < 0) break;
					body->pos[index] = atoi(buff1);
				}
				buff[j1++] = ch;
			}
			else buff1[j2++] = ch;
		}

		if (index >= 0) proto = boost::static_pointer_cast<NonKVBase>(body);
	}

	if (proto.unique()) {
		if (proto->gid.size() != group_len || (proto->uid.size() && proto->uid.size() != unit_len))
			proto.reset();
		else {
			for (++i; i < n && sref.at(i) != sep; ++i) proto->utc += sref.at(i);
			proto->utc += "T";
			for (++i; i < n && sref.at(i) != sep; ++i) proto->utc += sref.at(i);
			for (++i, j = 0; i < n && sref.at(i) != sep; ++i, ++j) buff[j] = sref.at(i);
			buff[j] = '\0';

			if (j > 0) proto->sn = atoi(buff);
			else proto.reset();
		}
	}

	return proto;
}

string NonKVProtocol::FindHome(int& serno, bool ra, bool dec) {
	MtxLck lck(mtxSerno_);
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf (buff, 100, "g#%s%shomera%ddec%d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		ra, dec,
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		serno_);
	serno = increase_serno();
	return string(buff);
}

string NonKVProtocol::HomeSync(int& serno, double ra, double dec) {
	MtxLck lck(mtxSerno_);
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%ssync%07d%%%+07d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		int(ra * 10000), int(dec * 10000),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		serno_);
	serno = increase_serno();
	return string(buff);
}

string NonKVProtocol::Slew(int& serno, double ra, double dec) {
	MtxLck lck(mtxSerno_);
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%sslew%07d%%%+07d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		int(ra * 10000), int(dec * 10000),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		serno_);
	serno = increase_serno();
	return string(buff);
}

string NonKVProtocol::SlewHD(int& serno, double ha, double dec) {
	MtxLck lck(mtxSerno_);
	char buff[100];
	ptime utc = second_clock::universal_time();
	if (ha < 0.0) ha += 360.0;
	snprintf(buff, 100, "g#%s%sHA%07d%%%+07d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		int(ha * 10000), int(dec * 10000),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		serno_);
	serno = increase_serno();
	return string(buff);
}

string NonKVProtocol::Guide(int& serno, int ra, int dec) {
	MtxLck lck(mtxSerno_);
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%sguide%+06d%%%+06d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		ra, dec,
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		serno_);
	serno = increase_serno();
	return string(buff);
}

string NonKVProtocol::Park(int& serno) {
	MtxLck lck(mtxSerno_);
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%spark%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		serno_);
	serno = increase_serno();
	return string(buff);
}

string NonKVProtocol::AbortSlew(int& serno) {
	MtxLck lck(mtxSerno_);
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%sabortslew%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		serno_);
	serno = increase_serno();
	return string(buff);
}

/**
 * @brief 组装构建track指令
 */
string NonKVProtocol::Track(int& serno) {
	MtxLck lck(mtxSerno_);
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%strack%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		serno_);
	serno = increase_serno();
	return string(buff);
}

/**
 * @brief 组装构建trackVel指令
 * @param ra  赤经速度, 量纲: 角秒@秒
 * @param dec 赤纬速度, 量纲: 角秒@秒
 * @note 设置转台自定义跟踪速度
 */
string NonKVProtocol::TrackVelocity(int& serno, double ra, double dec) {
	MtxLck lck(mtxSerno_);
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%strackvel%+8d%%%+8d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		int(ra * 1000 + 0.5), int(dec * 1000 + 0.5),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		serno_);
	serno = increase_serno();
	return string(buff);
}

string NonKVProtocol::FWHM(int& serno, const string& cid, const string& tmobs, double fwhm) {
	/* 组装调焦指令 */
	MtxLck lck(mtxSerno_);
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%sfwhm%s%04dT%s%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(), cid.c_str(),
		int(fwhm * 1000 + 0.5), tmobs.c_str(),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		serno_);
	serno = increase_serno();
	return string(buff);
}

string NonKVProtocol::Focus(int& serno, const std::string& cid, int pos) {
	/* 组装调焦指令 */
	MtxLck lck(mtxSerno_);
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%sfocus%s%+05d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(), cid.c_str(),
		pos,
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		serno_);
	serno = increase_serno();
	return string(buff);
}

/*!
 * @brief 由focus组装构建focus_sync指令
 * @param cid    相机标志
 * @return
 * 组装后协议
 */
string NonKVProtocol::FocusSync(int& serno, const std::string& cid) {
	/* 组装焦点重置指令 */
	MtxLck lck(mtxSerno_);
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%sfocus%sclear%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(), cid.c_str(),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		serno_);
	serno = increase_serno();
	return string(buff);
}
