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
#include <boost/make_shared.hpp>
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

NonKVBasePtr NonKVProtocol::Resolve(const char* rcvd) {
	NonKVBasePtr proto;
	string_ref sref(rcvd);
	string prefix("g#");				// 定义引导符
	string suffix("%");					// 定义结束符: 同时还是中间符!!!
	string type_ready(NONKVTYPE_READY);	// 定义协议: ready
	string type_state(NONKVTYPE_STATE);	// 定义协议: status
	string type_pos(NONKVTYPE_POS);		// 定义协议: currentpos
	string type_focus(NONKVTYPE_FOCUS);	// 定义协议: focus
	// string type_mcover("mirr");		// 定义协议: mirr

	// string op_home("home");		// 指令: 搜索零点
	// string op_sync("sync");		// 指令: 同步零点
	// string op_slew("slew");		// 指令: 指向/赤经赤纬
	// string op_slewhd("HA");		// 指令: 指向/时角赤纬
	// string op_guide("guide");	// 指令: 导星
	// string op_park("park");		// 指令: 复位
	// string op_abort("abortslew");	// 指令: 终止指向

	char sep = '%';			// 数据间分隔符
	int unit_len = 3;		// 约定: 单元标志长度为3字节
	// int camera_len = 3;		// 约定: 相机标志长度为3字节
	// int focus_len  = 4;		// 约定: 焦点位置长度为4字节
	// int mc_len = 2;			// 约定: 镜盖状态长度为2字节
	int n(sref.length() - suffix.length()), pos, i, j;
	char ch, buff[20], buff1[20];

	if (!sref.starts_with(prefix) || !sref.ends_with(suffix)) {
		_gLog.Write(LOG_FAULT, "%s:%s, illegal protocol[%s]",
			typeid(this).name(), __FUNCTION__, rcvd);
	}
	else if ((pos = sref.find("Rec")) > 0) {// 指令回馈, 抛弃
	}
	else if ((pos = sref.find(type_ready)) > 0) {// ready
		boost::shared_ptr<NonKVReady> body(new NonKVReady);
		body->type = type_ready;
		for (i = prefix.length(); i < pos; ++i) body->gid += sref.at(i);
		for (i = pos + type_ready.length(), j = 0; i < n && sref.at(i) != sep; ++i, ++j, ++body->n) body->ready[j] = sref.at(i) - '0';
		proto = boost::static_pointer_cast<NonKVBase>(body);
	}
	else if ((pos = sref.find(type_state)) > 0) {// state
		boost::shared_ptr<NonKVStatus> body(new NonKVStatus);
		body->type = type_state;
		for (i = prefix.length(); i < pos; ++i) body->gid += sref.at(i);
		for (i = pos + type_state.length(), j = 0; i < n && sref.at(i) != sep; ++i, ++j, ++body->n) body->state[j] = sref.at(i) - '0';
		proto = boost::static_pointer_cast<NonKVBase>(body);
	}
	else if ((pos = sref.find(type_pos)) > 0) {// currentpos
		boost::shared_ptr<NonKVPosition> body(new NonKVPosition);
		int j1(0), j2(0);
		body->type = type_pos;
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
		body->type = type_focus;
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
		for (++i; i < n && sref.at(i) != sep; ++i) proto->utc += sref.at(i);
		proto->utc += "T";
		for (++i; i < n && sref.at(i) != sep; ++i) proto->utc += sref.at(i);
		for (++i, j = 0; i < n && sref.at(i) != sep; ++i, ++j) buff[j] = sref.at(i);
		buff[j] = '\0';

		if (j > 0) proto->sn = atoi(buff);
		else proto.reset();
	}

	return proto;
}

string NonKVProtocol::FindHome(bool ra, bool dec) {
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf (buff, 100, "g#%s%shomera%ddec%d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		ra, dec,
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		++sn_findhome_);
	if (sn_findhome_ == 99999) sn_findhome_ = 0;
	return string(buff);
}

string NonKVProtocol::HomeSync(double ra, double dec) {
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%ssync%07d%%%+07d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		int(ra * 10000), int(dec * 10000),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		++sn_homesync_);
	if (sn_homesync_ == 99999) sn_homesync_ = 0;
	return string(buff);
}

string NonKVProtocol::Slew(double ra, double dec) {
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%sslew%07d%%%+07d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		int(ra * 10000), int(dec * 10000),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		++sn_slew_);
	if (sn_slew_ == 99999) sn_slew_ = 0;
	return string(buff);
}

string NonKVProtocol::SlewHD(double ha, double dec) {
	char buff[100];
	ptime utc = second_clock::universal_time();
	if (ha < 0.0) ha += 360.0;
	snprintf(buff, 100, "g#%s%sHA%07d%%%+07d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		int(ha * 10000), int(dec * 10000),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		++sn_slewhd_);
	if (sn_slewhd_ == 99999) sn_slewhd_ = 0;
	return string(buff);
}

string NonKVProtocol::Guide(int ra, int dec) {
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%sguide%+06d%%%+06d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		ra, dec,
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		++sn_guide_);
	if (sn_guide_ == 99999) sn_guide_ = 0;
	return string(buff);
}

string NonKVProtocol::Park() {
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%spark%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		++sn_park_);
	if (sn_park_ == 99999) sn_park_ = 0;
	return string(buff);
}

string NonKVProtocol::AbortSlew() {
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%sabortslew%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(),
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		++sn_abort_);
	if (sn_abort_ == 99999) sn_abort_ = 0;
	return string(buff);
}

// const char* mount_proto::compact_fwhm(const std::string& group_id, const std::string& unit_id,
// 		const std::string& camera_id, double fwhm, int& n) {
// 	/* 组装调焦指令 */
// 	char *buff = get_buffptr();
// 	n = sprintf(buff, "g#%s%sfwhm%s%04d%%\n", group_id.c_str(), unit_id.c_str(), camera_id.c_str(),
// 			int(fwhm * 100));
// 	return buff;
// }

string NonKVProtocol::Focus(const std::string& cid, int pos) {
	/* 组装调焦指令 */
	char buff[100];
	ptime utc = second_clock::universal_time();
	snprintf(buff, 100, "g#%s%sfocus%s%+05d%%%s%%%s%%%05d%%\n",
		gid_.c_str(), uid_.c_str(), cid.c_str(),
		pos,
		to_iso_extended_string(utc.date()).c_str(),
		to_simple_string(utc.time_of_day()).c_str(),
		++sn_focus_);
	if (sn_focus_ == 99999) sn_focus_ = 0;
	return string(buff);
}

// const char* mount_proto::compact_mirror_cover(const std::string& group_id, const std::string& unit_id,
// 		const std::string& camera_id, const int command, int& n) {
// 	/* 组装镜盖操作指令 */
// 	char *buff = get_buffptr();
// 	n = sprintf(buff, "g#%s%smirr%s%s%%\n", group_id.c_str(), unit_id.c_str(), camera_id.c_str(),
// 			command == 1 ? "open" : "close");
// 	return buff;
// }
