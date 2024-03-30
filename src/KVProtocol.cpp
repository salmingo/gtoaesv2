
#include <ctype.h>
#include <math.h>
#include <vector>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <typeinfo>
#include "KVProtocol.h"
#include "GLog.h"

using namespace boost;
using namespace boost::algorithm;

typedef std::vector<string> vecstr;    ///< 字符串列表

KVProtocol::KVProtocol() {
}

KVProtocol::~KVProtocol() {
}

KVBasePtr KVProtocol::Resolve(const char* rcvd) {
    const char* ptr = rcvd;
    string type;
    char ch;

    // 解析操作符
	while (*ptr && *ptr == ' ') ++ptr; // 容错
    ch = tolower(*ptr);
	for (; *ptr && *ptr != ' '; ++ptr) type += *ptr;
	while (*ptr && *ptr == ' ') ++ptr; // 分隔符' '; 容错
    // 解析键值对
    KVBase basis;
    KVVec kvs;
    if (*ptr) resolve_kv(ptr, kvs, basis);
    // 分类型赋值
    KVBasePtr proto;
    if (ch == 'a') {
        if      (iequals(type, KVTYPE_APPPLAN)) proto = resolve_append_plan(kvs);
        if      (iequals(type, KVTYPE_APPGWAC)) proto = resolve_append_gwac(kvs);
        else if (iequals(type, KVTYPE_ABORT))   proto = resolve_abort(kvs);
    }
    else if (ch == 'c') {
        if      (iequals(type, KVTYPE_CHKPLAN)) proto = resolve_check_plan(kvs);
        else if (iequals(type, KVTYPE_CAMERA))  proto = resolve_camera(kvs);
        else if (iequals(type, KVTYPE_CAMSET))  proto = resolve_camset(kvs);
    }
    else if (ch == 'd') {
        if      (iequals(type, KVTYPE_DEROT)) proto = resolve_derot(kvs);
        else if (iequals(type, KVTYPE_DOME))  proto = resolve_dome(kvs);
    }
    else if (ch == 'f') {
        if      (iequals(type, KVTYPE_FOCUS))       proto = resolve_focus(kvs);
        else if (iequals(type, KVTYPE_FOCUS_SYNC))  proto = resolve_focus_sync(kvs);
        else if (iequals(type, KVTYPE_FWHM))  proto = resolve_fwhm(kvs);
        else if (iequals(type, KVTYPE_FILTER))proto = resolve_filter(kvs);
    }
    else if (ch == 'm') {
        if      (iequals(type, KVTYPE_MOUNT))  proto = resolve_mount(kvs);
        else if (iequals(type, KVTYPE_MCOVER)) proto = resolve_mcover(kvs);
    }
    else if (ch == 'p') {
        if      (iequals(type, KVTYPE_PLAN)) proto = resolve_plan(kvs);
        else if (iequals(type, KVTYPE_PARK)) proto = resolve_park(kvs);
    }
    else if (ch == 's') {
        if      (iequals(type, KVTYPE_SLEWTO)) proto = resolve_slewto(kvs);
        else if (iequals(type, KVTYPE_SYNC))   proto = resolve_sync(kvs);
    }
	else if (ch == 't') {
	    if      (iequals(type, KVTYPE_TKIMG))     proto = resolve_take_image(kvs);
	    else if (iequals(type, KVTYPE_TRACK))     proto = resolve_track(kvs);
	    else if (iequals(type, KVTYPE_TRACKVEL))  proto = resolve_trackvel(kvs);
	}
    else if (iequals(type, KVTYPE_EXPOSE))  proto = resolve_expose(kvs);
    else if (iequals(type, KVTYPE_GUIDE))   proto = resolve_guide(kvs);
    else if (iequals(type, KVTYPE_HOME))    proto = resolve_home(kvs);
    else if (iequals(type, KVTYPE_OBSS))    proto = resolve_obss(kvs);
    else if (iequals(type, KVTYPE_RMVPLAN)) proto = resolve_remove_plan(kvs);

    if (proto.unique()) *proto = basis;
    else _gLog.Write(LOG_FAULT, "%s:%s: %s", typeid(this).name(), __FUNCTION__, rcvd);

    return proto;
}

//////////////////////////////////////////////////////////////////////////////
// 功能
/**
 * @brief 以=为分隔符, 解析关键字和键值
 * @param kv       keyword=value格式字符串
 * @param keyword  关键字
 * @param value    键值
 * @return keyword和value有效标志
 */
bool KVProtocol::resolve_kv(const string& kv, string& keyword, string& value) {
    char seps[] = "=";
    vecstr tokens;

    split(tokens, kv, is_any_of(seps), token_compress_on);
    if (tokens.size() != 2) return false;
    keyword = tokens[0]; trim(keyword);
    value   = tokens[1]; trim(value);
    return !(keyword.empty() || value.empty());
}

/**
 * @brief 解析键值对集合
 * @param str    键值对集合字符串, key1=val1,[key2=val2,...]
 * @param kvs    解析后键值对集合
 * @param basis  临时存储基础信息
 */
void KVProtocol::resolve_kv(const char* str, KVVec& kvs, KVBase& basis) {
    char seps[] = ",";
    vecstr tokens;
    string keyword, value;

    split(tokens, str, is_any_of(seps), token_compress_on);
    for(vecstr::iterator it = tokens.begin(); it != tokens.end(); ++it) {
        if (!resolve_kv(*it, keyword, value)) continue;
        if      (iequals(keyword, "utc")) basis.utc = value;
        else if (iequals(keyword, "gid")) basis.gid = value;
        else if (iequals(keyword, "uid")) basis.uid = value;
        else if (iequals(keyword, "cid")) basis.cid = value;
        else kvs.push_back(KeyValPair(keyword, value));
    }
}

//////////////////////////////////////////////////////////////////////////////
// 功能: 按协议类型创建对应实例指针
/**
 * @brief 追加观测计划
 */
KVBasePtr KVProtocol::resolve_append_plan(const KVVec& kvs) {
    KVAppPlanPtr proto(new KVAppPlan);
    KVVec& kvsProto = proto->kvs;

    proto->coorsys = COORSYS_EQUA;
    proto->epoch   = 2000.0;
    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "plan_sn"))  proto->plan_sn = it->value;
            else if (iequals(it->keyword, "objid"))    proto->objid   = it->value;
            else if (iequals(it->keyword, "obstype"))  proto->obstype = it->value;
            else if (iequals(it->keyword, "coor_sys")) proto->coorsys = std::stoi(it->value);
            else if (iequals(it->keyword, "ra"))       proto->ra      = std::stod(it->value);
            else if (iequals(it->keyword, "dec"))      proto->dec     = std::stod(it->value);
            else if (iequals(it->keyword, "ecoch"))    proto->epoch   = std::stod(it->value);
            else if (iequals(it->keyword, "azi"))      proto->azi     = std::stod(it->value);
            else if (iequals(it->keyword, "ele"))      proto->ele     = std::stod(it->value);
            else if (iequals(it->keyword, "tle1"))     proto->tle1    = it->value;
            else if (iequals(it->keyword, "tle2"))     proto->tle2    = it->value;
            else if (iequals(it->keyword, "imgtype"))  proto->imgtype = it->value;
            else if (iequals(it->keyword, "filter"))   proto->filter  = it->value;
            else if (iequals(it->keyword, "exptime"))  proto->exptime = std::stod(it->value);
            else if (iequals(it->keyword, "delay"))    proto->delay   = std::stod(it->value);
            else if (iequals(it->keyword, "frmcnt"))   proto->frmcnt  = std::stoi(it->value);
            else if (iequals(it->keyword, "loopcnt"))  proto->loopcnt = std::stoi(it->value);
            else if (iequals(it->keyword, "priority")) proto->priority= std::stoi(it->value);
            else if (iequals(it->keyword, "plan_beg")) proto->plan_begin= it->value;
            else if (iequals(it->keyword, "plan_end")) proto->plan_end  = it->value;
            else kvsProto.push_back(*it);
        }

        // 修订缺省项
        if (proto->imgtype.empty()) proto->imgtype = fabs(proto->exptime) < 1E-3 ? "BIAS" : "OBJECT";
        if (proto->objid.empty())   proto->objid   = iequals(proto->imgtype, "BIAS") ? "bias"
            : (iequals(proto->imgtype, "DARK") ? "dark"
                : (iequals(proto->imgtype, "FLAT") ? "flat"
                    : (iequals(proto->imgtype, "FOCUS") ? "focs" : "objt")));
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }

    return to_kvbase(proto);
}

/**
 * @brief 追加观测计划: GWAC
 */
KVBasePtr KVProtocol::resolve_append_gwac(const KVVec& kvs) {
    KVBasePtr proto = resolve_append_plan(kvs);
	if (proto.unique()) proto->type = KVTYPE_APPGWAC;
	return proto;
}

/**
 * @brief 检查观测计划状态
 */
KVBasePtr KVProtocol::resolve_check_plan(const KVVec& kvs) {
    KVChkPlanPtr proto = boost::make_shared<KVCheckPlan>();

    for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
        if (iequals(it->keyword, "plan_sn")) proto->plan_sn = it->value;
    }
    return to_kvbase(proto);
}

/**
 * @brief 删除观测计划
 */
KVBasePtr KVProtocol::resolve_remove_plan(const KVVec& kvs) {
    KVRmvPlanPtr proto = boost::make_shared<KVRemovePlan>();

    for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
        if (iequals(it->keyword, "plan_sn")) proto->plan_sn = it->value;
    }
    return to_kvbase(proto);
}

/**
 * @brief 观测计划实时
 */
KVBasePtr KVProtocol::resolve_plan(const KVVec& kvs) {
    KVPlanPtr proto = boost::make_shared<KVPlan>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "state"))    proto->state    = std::stoi(it->value);
            else if (iequals(it->keyword, "plan_sn"))  proto->plan_sn  = it->value;
            else if (iequals(it->keyword, "tm_start")) proto->tm_start = it->value;
            else if (iequals(it->keyword, "tm_stop"))  proto->tm_stop  = it->value;
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 中断当前任务
 */
KVBasePtr KVProtocol::resolve_abort(const KVVec& kvs) {
    KVAbtPtr proto = boost::make_shared<KVAbort>();
    return to_kvbase(proto);
}

/**
 * @brief 观测系统实时工作状态
 */
KVBasePtr KVProtocol::resolve_obss(const KVVec& kvs) {
    KVObssPtr proto = boost::make_shared<KVOBSS>();
    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "state"))  proto->state  = std::stoi(it->value);
            else if (iequals(it->keyword, "mount"))  proto->mount  = std::stoi(it->value);
            else if (iequals(it->keyword, "camera")) proto->camera = std::stoi(it->value);
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 转台实时工作状态
 */
KVBasePtr KVProtocol::resolve_mount(const KVVec& kvs) {
    KVMountPtr proto = boost::make_shared<KVMount>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "state"))    proto->state    = std::stoi(it->value);
            else if (iequals(it->keyword, "errcode"))  proto->errcode  = std::stoi(it->value);
            else if (iequals(it->keyword, "mjd"))      proto->mjd      = std::stod(it->value);
            else if (iequals(it->keyword, "lst"))      proto->lst      = std::stod(it->value);
            else if (iequals(it->keyword, "ra"))       proto->ra       = std::stod(it->value);
            else if (iequals(it->keyword, "dec"))      proto->dec      = std::stod(it->value);
            else if (iequals(it->keyword, "ra2k"))     proto->ra2k     = std::stod(it->value);
            else if (iequals(it->keyword, "dec2k"))    proto->dec2k    = std::stod(it->value);
            else if (iequals(it->keyword, "azi"))      proto->azi      = std::stod(it->value);
            else if (iequals(it->keyword, "ele"))      proto->ele      = std::stod(it->value);
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 相机实时工作状态
 */
KVBasePtr KVProtocol::resolve_camera(const KVVec& kvs) {
    KVCamPtr proto = boost::make_shared<KVCamera>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "state"))    proto->state    = std::stoi(it->value);
            else if (iequals(it->keyword, "errcode"))  proto->errcode  = std::stoi(it->value);
            else if (iequals(it->keyword, "left"))     proto->left     = std::stod(it->value);
            else if (iequals(it->keyword, "percent"))  proto->percent  = std::stod(it->value);
            else if (iequals(it->keyword, "freedisk")) proto->freedisk = std::stoi(it->value);
            else if (iequals(it->keyword, "coolget"))  proto->coolget  = std::stoi(it->value);
            else if (iequals(it->keyword, "loopno"))   proto->loopno   = std::stoi(it->value);
            else if (iequals(it->keyword, "frmno"))    proto->frmno    = std::stoi(it->value);
            else if (iequals(it->keyword, "imgtype"))  proto->imgtype  = it->value;
            else if (iequals(it->keyword, "filter"))   proto->filter   = it->value;
            else if (iequals(it->keyword, "filename")) proto->fileName = it->value;
            else if (iequals(it->keyword, "plan_sn"))  proto->plan_sn  = it->value;
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 转台: 搜索零点
 */
KVBasePtr KVProtocol::resolve_home(const KVVec& kvs) {
    KVHomePtr proto = boost::make_shared<KVHome>();
    return to_kvbase(proto);
}

/**
 * @brief 转台: 同步零点
 */
KVBasePtr KVProtocol::resolve_sync(const KVVec& kvs) {
    KVSyncPtr proto = boost::make_shared<KVSync>();

    try {
        proto->epoch = 2000.0;
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "ra"))    proto->ra    = std::stod(it->value);
            else if (iequals(it->keyword, "dec"))   proto->dec   = std::stod(it->value);
            else if (iequals(it->keyword, "ecoch")) proto->epoch = std::stod(it->value);
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 转台: 复位
 */
KVBasePtr KVProtocol::resolve_park(const KVVec& kvs) {
    KVParkPtr proto = boost::make_shared<KVPark>();
    return to_kvbase(proto);
}

/**
 * @brief 转台: 指向
 */
KVBasePtr KVProtocol::resolve_slewto(const KVVec& kvs) {
    KVSlewPtr proto = boost::make_shared<KVSlewto>();

    proto->coorsys = COORSYS_EQUA;
    proto->epoch   = 2000.0;
    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "coor_sys")) proto->coorsys = std::stoi(it->value);
            else if (iequals(it->keyword, "ra"))       proto->ra      = std::stod(it->value);
            else if (iequals(it->keyword, "dec"))      proto->dec     = std::stod(it->value);
            else if (iequals(it->keyword, "ecoch"))    proto->epoch   = std::stod(it->value);
            else if (iequals(it->keyword, "azi"))      proto->azi     = std::stod(it->value);
            else if (iequals(it->keyword, "ele"))      proto->ele     = std::stod(it->value);
            else if (iequals(it->keyword, "tle1"))     proto->tle1    = it->value;
            else if (iequals(it->keyword, "tle2"))     proto->tle2    = it->value;
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 转台: 导星
 */
KVBasePtr KVProtocol::resolve_guide(const KVVec& kvs) {
    KVGuidePtr proto = boost::make_shared<KVGuide>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "ra"))       proto->ra      = std::stoi(it->value);
            else if (iequals(it->keyword, "dec"))      proto->dec     = std::stoi(it->value);
            else if (iequals(it->keyword, "result"))   proto->result  = std::stoi(it->value);
            else if (iequals(it->keyword, "op"))       proto->op      = std::stoi(it->value);
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 转台: 切换跟踪模式
 */
KVBasePtr KVProtocol::resolve_track(const KVVec& kvs) {
	KVTrackPtr proto = boost::make_shared<KVTrack>();
    return to_kvbase(proto);
}

/**
 * @brief 转台: 改变跟踪速度
 */
KVBasePtr KVProtocol::resolve_trackvel(const KVVec& kvs) {
    KVTrackVelPtr proto = boost::make_shared<KVTrackVel>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "ra"))   proto->ra   = std::stod(it->value);
            else if (iequals(it->keyword, "dec"))  proto->dec  = std::stod(it->value);
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 相机: 手动曝光
 */
KVBasePtr KVProtocol::resolve_take_image(const KVVec& kvs) {
    KVBasePtr proto = resolve_append_plan(kvs);
	if (proto.unique()) proto->type = KVTYPE_TKIMG;
	return proto;
}

/**
 * @brief 相机: 曝光指令
 */
KVBasePtr KVProtocol::resolve_expose(const KVVec& kvs){
    KVExpPtr proto = boost::make_shared<KVExpose>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if (iequals(it->keyword, "command"))     proto->command = std::stoi(it->value);
            else if (iequals(it->keyword, "frmno"))  proto->frmno   = std::stoi(it->value);
            else if (iequals(it->keyword, "loopno")) proto->loopno  = std::stoi(it->value);
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 相机参数: 状态/查询/修改
 */
KVBasePtr KVProtocol::resolve_camset(const KVVec& kvs) {
    KVCamSetPtr proto = boost::make_shared<KVCamSet>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "optype"))     proto->opType     = std::stoi(it->value);
            else if (iequals(it->keyword, "bitDepth"))   proto->bitDepth   = std::stoi(it->value);
            else if (iequals(it->keyword, "iADC"))       proto->iADC       = std::stoi(it->value);
            else if (iequals(it->keyword, "iReadPort"))  proto->iReadPort  = std::stoi(it->value);
            else if (iequals(it->keyword, "iReadRate"))  proto->iReadRate  = std::stoi(it->value);
            else if (iequals(it->keyword, "iVSRate"))    proto->iVSRate    = std::stoi(it->value);
            else if (iequals(it->keyword, "iGain"))      proto->iGain      = std::stoi(it->value);
            else if (iequals(it->keyword, "coolSet"))    proto->coolSet    = std::stoi(it->value);
            else if (iequals(it->keyword, "bitPixel"))   proto->bitPixel   = std::stoi(it->value);
            else if (iequals(it->keyword, "ADC"))        proto->ADC        = it->value;
            else if (iequals(it->keyword, "readPort"))   proto->readPort   = it->value;
            else if (iequals(it->keyword, "readRate"))   proto->readRate   = it->value;
            else if (iequals(it->keyword, "vsRate"))     proto->vsRate     = std::stod(it->value);
            else if (iequals(it->keyword, "gain"))       proto->gain       = std::stod(it->value);
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 调焦: 开环调焦
 */
KVBasePtr KVProtocol::resolve_focus(const KVVec& kvs) {
    KVFocusPtr proto = boost::make_shared<KVFocus>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "optype"))  proto->opType  = std::stoi(it->value);
			else if (iequals(it->keyword, "state"))   proto->state   = std::stoi(it->value);
            else if (iequals(it->keyword, "relpos"))  proto->relPos  = std::stoi(it->value);
            else if (iequals(it->keyword, "pos"))     proto->pos     = std::stoi(it->value);
            else if (iequals(it->keyword, "posTar"))  proto->posTar  = std::stoi(it->value);
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 调焦: 重置焦点零点
 */
KVBasePtr KVProtocol::resolve_focus_sync(const KVVec& kvs) {
	KVFocusSyncPtr proto = boost::make_shared<KVFocusSync>();
	return to_kvbase(proto);
}

/**
 * @brief 调焦: 闭环调焦
 */
KVBasePtr KVProtocol::resolve_fwhm(const KVVec& kvs) {
    KVFwhmPtr proto = boost::make_shared<KVFWHM>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "value")) proto->value = std::stod(it->value);
            else if (iequals(it->keyword, "tmimg")) proto->tmimg = it->value;
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 消旋器: 指令和状态
 */
KVBasePtr KVProtocol::resolve_derot(const KVVec& kvs) {
    KVDerotPtr proto = boost::make_shared<KVDerot>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "optype"))  proto->opType  = std::stoi(it->value);
            else if (iequals(it->keyword, "command")) proto->command = std::stoi(it->value);
            else if (iequals(it->keyword, "state"))   proto->state   = std::stoi(it->value);
            else if (iequals(it->keyword, "postar"))  proto->posTar  = std::stod(it->value);
            else if (iequals(it->keyword, "pos"))     proto->pos     = std::stod(it->value);
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 圆顶和天窗: 指令和状态
 */
KVBasePtr KVProtocol::resolve_dome(const KVVec& kvs) {
    KVDomePtr proto = boost::make_shared<KVDome>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "optype"))  proto->opType  = std::stoi(it->value);
            else if (iequals(it->keyword, "command")) proto->command = std::stoi(it->value);
            else if (iequals(it->keyword, "state"))   proto->state   = std::stoi(it->value);
            else if (iequals(it->keyword, "azi"))     proto->azi     = std::stod(it->value);
            else if (iequals(it->keyword, "ele"))     proto->ele     = std::stod(it->value);
            else if (iequals(it->keyword, "aziobj"))  proto->aziObj  = std::stod(it->value);
            else if (iequals(it->keyword, "eleobj"))  proto->eleObj  = std::stod(it->value);
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 镜盖: 指令和状态
 */
KVBasePtr KVProtocol::resolve_mcover(const KVVec& kvs) {
    KVMCoverPtr proto = boost::make_shared<KVMirrCover>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "optype"))  proto->opType  = std::stoi(it->value);
            else if (iequals(it->keyword, "command")) proto->command = std::stoi(it->value);
            else if (iequals(it->keyword, "state"))   proto->state   = std::stoi(it->value);
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 滤光片(单独控制): 指令和状态
 */
KVBasePtr KVProtocol::resolve_filter(const KVVec& kvs) {
    KVFilPtr proto = boost::make_shared<KVFilter>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "optype")) proto->opType = std::stoi(it->value);
            else if (iequals(it->keyword, "name"))   proto->name   = it->value;
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}

/**
 * @brief 测站位置: 状态/查询/修改
 */
KVBasePtr KVProtocol::resolve_geosite(const KVVec& kvs) {
    KVSitePtr proto = boost::make_shared<KVGeoSite>();

    try {
        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            if      (iequals(it->keyword, "optype")) proto->opType = std::stoi(it->value);
            else if (iequals(it->keyword, "name"))   proto->name   = std::stod(it->value);
            else if (iequals(it->keyword, "lon"))    proto->lon    = std::stod(it->value);
            else if (iequals(it->keyword, "lat"))    proto->lat    = std::stod(it->value);
            else if (iequals(it->keyword, "alt"))    proto->alt    = std::stod(it->value);
        }
    }
    catch(std::invalid_argument& ex1) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex1.what());
    }
    catch(std::out_of_range& ex2) {
        proto.reset();
        _gLog.Write(LOG_WARN, "[%s]: %s", proto->type.c_str(), ex2.what());
    }
    return to_kvbase(proto);
}
