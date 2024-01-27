/**
 * @file ProtoKV.h 定义基于键值对的通信协议
 * @author 卢晓猛 (lxm@nao.cas.cn)
 * @brief
 * @version 0.1
 * @date 2023-10-09
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef PROTOKV_H
#define PROTOKV_H

#include "ProtoKVBase.h"
#include "AstroDeviceDef.h"

#define KVTYPE_APPGWAC     "append_gwac"    ///< 追加GWAC观测计划
#define KVTYPE_APPPLAN     "append_plan"    ///< 追加观测计划
#define KVTYPE_CHKPLAN     "check_plan"     ///< 查询观测计划
#define KVTYPE_RMVPLAN     "remove_plan"    ///< 删除观测计划. 若未执行, 则删除; 在执行, 中断后删除
#define KVTYPE_PLAN        "plan"           ///< 计划状态
#define KVTYPE_ABORT       "abort"          ///< 中断. 服务器->中断计划; 转台->中断指向和跟踪; 相机->中断曝光

//////////////////////////////////////////////////////////////////////////////
// 观测系统
#define KVTYPE_OBSS         "obss"          ///< 观测系统工作状态

//////////////////////////////////////////////////////////////////////////////
// 转台
#define KVTYPE_SLEWTO       "slew"          ///< 指向
#define KVTYPE_PARK         "park"          ///< 复位
#define KVTYPE_GUIDE        "guide"         ///< 导星. 与图像处理结果闭环 --> 转台
#define KVTYPE_HOME         "home"          ///< 搜索零点
#define KVTYPE_SYNC         "sync"          ///< 同步零点
#define KVTYPE_MOUNT        "mount"         ///< 转台工作状态
// 转台
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 相机
#define KVTYPE_TKIMG        "take_image"    ///< 采集图像
#define KVTYPE_EXPOSE       "expose"        ///< 曝光指令
#define KVTYPE_CAMSET       "camset"        ///< 相机参数: 查询/修改
#define KVTYPE_CAMERA       "camera"        ///< 相机工作状态
// 相机
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 调焦
#define KVTYPE_FOCUS        "focus"         ///< 调焦
#define KVTYPE_FWHM         "fwhm"          ///< 闭环调焦
// 调焦
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 消旋器
#define KVTYPE_DEROT       "derot"          ///< 消旋器指令和状态
// 消旋器
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 圆顶
#define KVTYPE_DOME        "dome"           ///< 圆顶指令和状态
// 圆顶
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 镜盖
#define KVTYPE_MCOVER      "mcover"         ///< 镜盖指令和状态
// 镜盖
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 滤光片
#define KVTYPE_FILTER      "filter"         ///< 滤光片指令和状态
// 滤光片
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 测站位置
#define KVTYPE_GEOSITE     "geosite"        ///< 测站位置: 查询/修改/状态
// 滤光片
//////////////////////////////////////////////////////////////////////////////
/**
 * @brief 新的观测计划: GWAC
 * @note
 * - 服务器: 维护调度观测计划
 * - 相机: 观测计划的描述信息
 */
struct KVAppGWAC : public KVBase
{
    string plan_sn;     ///< 计划编号
    string objid;       ///< 目标名称
	string obstype; 	///< 计划类型
    int    coorsys = 0; ///< 0: 赤道/赤经赤纬; 1: 赤道/时角赤纬
    double ra = 0;          ///< 赤经/时角, 角度
    double dec= 0;         ///< 赤纬, 角度
    double epoch= 0;       ///< 历元. 0.0=当前历元
    /**
     * imgtype的特殊用法:
     * @li 当没有指定imgtype时, 采用默认值
     * - 当expdur==0时, imgtype <-- BIAS
     * - 当expdur!=0时, imgtype <-- OBJECT
     */
    string imgtype;     ///< 图像类型. BIAS/DARK/FLAT/OBJECT=LIGHT/FOCUS
    double exptime = 0;     ///< 曝光时间
	double delay  = 0; 		///< 帧间延迟
    int    frmcnt = 0;      ///< 总帧数
	string grid_id;		///< 分区模式
	string field_id;	///< 天区编号
    string plan_begin;  ///< 计划开始时间. CCYY-MM-DDThh:mm:ss
    string plan_end;    ///< 计划结束时间. CCYY-MM-DDThh:mm:ss
    KVVec  kvs;         ///< 一般键值对

public:
    KVAppGWAC() {
		type = KVTYPE_APPGWAC;
	}

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        if (plan_sn.size()) ss << join_kv("plan_sn", plan_sn);
        if (objid.size())   ss << join_kv("objid", objid);
		ss << join_kv("obstype",  obstype);
        ss << join_kv("coor_sys", coorsys);
        ss << join_kv("ra",    ra);
        ss << join_kv("dec",   dec);
        ss << join_kv("epoch", epoch);
        ss << join_kv("imgtype",  imgtype);
        ss << join_kv("exptime",  exptime);
        ss << join_kv("frmcnt",   frmcnt);
		if (grid_id.size())    ss << join_kv("grid_id",  grid_id);
		if (field_id.size())   ss << join_kv("field_id", field_id);
        if (plan_begin.size()) ss << join_kv("plan_beg", plan_begin);
        if (plan_end.size())   ss << join_kv("plan_end", plan_end);

        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            ss << join_kv(it->keyword, it->value);
        }

        ss << std::endl;
        return ss.str();
    }
};

/**
 * @brief 新的观测计划
 * @note
 * - 服务器: 维护调度观测计划
 * - 相机: 观测计划的描述信息
 */
struct KVAppPlan : public KVBase
{
    string plan_sn;     ///< 计划编号
    string objid;       ///< 目标名称
    int    coorsys;     ///< 0: 赤道; 1: 地平; 2: TLE
    double ra;          ///< 赤经, 角度
    double dec;         ///< 赤纬, 角度
    double epoch;       ///< 历元. 0.0=当前历元
    double azi;         ///< 方位角, 角度, 南零点
    double ele;         ///< 俯仰角, 角度
    string tle1;        ///< 第一行TLE根数
    string tle2;        ///< 第二行TLE根数
    /**
     * imgtype的特殊用法:
     * @li 当没有指定objid时, imgtype替代objid
     * - 当expdur==0时, objid <-- bias
     * - 当expdur!=0时, objid <-- objt
     * @li 当没有指定imgtype时, 采用默认值
     * - 当expdur==0时, imgtype <-- BIAS
     * - 当expdur!=0时, imgtype <-- OBJECT
     */
    string imgtype;     ///< 图像类型. BIAS/DARK/FLAT/OBJECT=LIGHT/FOCUS
    /**
     * filter的特殊用法:
     * @li 可采用一般字符串格式: x, 表示所有关联相机都采用该滤光片
     * @li 可采用复合字符串格式: x1|y1;x2|y2.
     * - x1分配给关联的第一个相机; y1分配给第二个相机
     * - 当x1或y1序列图像采集完成后, 继续将x2和y2分别分配给不同相机
     */
    string filter;      ///< 滤光片名称
    double exptime;     ///< 曝光时间
    int    frmcnt;      ///< 总帧数
    int    loopcnt;     ///< 循环次数
    int    priority;    ///< 优先级
    string plan_begin;  ///< 计划开始时间. CCYY-MM-DDThh:mm:ss
    string plan_end;    ///< 计划结束时间. CCYY-MM-DDThh:mm:ss
    KVVec  kvs;         ///< 一般键值对

public:
    KVAppPlan() {
		type = KVTYPE_APPPLAN;
	}

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        if (plan_sn.size()) ss << join_kv("plan_sn", plan_sn);
        if (objid.size())   ss << join_kv("objid", objid);
        ss << join_kv("coor_sys", coorsys);
        if (coorsys == COORSYS_EQUA) {
            ss << join_kv("ra",    ra);
            ss << join_kv("dec",   dec);
            ss << join_kv("epoch", epoch);
        }
        else if (coorsys == COORSYS_ALTAZ) {
            ss << join_kv("azi", azi);
            ss << join_kv("ele", ele);
        }
        else {
            ss << join_kv("tle1", tle1);
            ss << join_kv("tle2", tle2);
        }
        if (imgtype.size()) ss << join_kv("imgtype", imgtype);
        if (filter.size())  ss << join_kv("filter", filter);
        ss << join_kv("exptime",  exptime);
        ss << join_kv("frmcnt",  frmcnt);
        ss << join_kv("loopcnt", loopcnt);
        ss << join_kv("priority",priority);
        if (plan_begin.size()) ss << join_kv("plan_beg", plan_begin);
        if (plan_end.size())   ss << join_kv("plan_end", plan_end);

        for (KVVec::const_iterator it = kvs.begin(); it != kvs.end(); ++it) {
            ss << join_kv(it->keyword, it->value);
        }

        ss << std::endl;
        return ss.str();
    }
};

/**
 * @brief 查询观测计划及执行结果
 */
struct KVCheckPlan : public KVBase {
    string plan_sn;     ///< 计划编号. ==空时查询所有计划

public:
    KVCheckPlan() {
        type = KVTYPE_CHKPLAN;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        if (plan_sn.size()) ss << join_kv("plan_sn", plan_sn);
        ss << std::endl;
        return ss.str();
    }
};

struct KVRemovePlan : public KVBase {
    string plan_sn;     ///< 计划编号. ==空时删除所有计划

public:
    KVRemovePlan() {
        type = KVTYPE_RMVPLAN;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        if (plan_sn.size()) ss << join_kv("plan_sn", plan_sn);
        ss << std::endl;
        return ss.str();
    }
};

/**
 * @brief 观测计划工作状态
 */
struct KVPlan : public KVBase {
    string plan_sn;     ///< 计划编号. take_image生成自定义计划, 编号: CCYYMMDD_SN. SN: 0001->9999
    string tm_start;    ///< 开始执行时间
    string tm_stop;     ///< 执行结束/中断/抛弃时间
    int    state;       ///< 计划状态, 由AstroDeviceDef中StateObservationPlan定义

public:
    KVPlan() {
        type = KVTYPE_PLAN;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        if (plan_sn.size())  ss << join_kv("plan_sn",  plan_sn);
        if (tm_start.size()) ss << join_kv("tm_start", tm_start);
        if (tm_stop.size())  ss << join_kv("tm_stop",  tm_stop);
        ss << join_kv("state", state);
        ss << std::endl;
        return ss.str();
    }
};

/**
 * @brief 中断当前工作
 * @note
 * - 服务器: 中断观测计划
 * - 转台: 中断指向和跟踪
 * - 相机: 中断曝光
 */
struct KVAbort : public KVBase {
public:
    KVAbort() {
        type = KVTYPE_ABORT;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << std::endl;
        return ss.str();
    }
};

//////////////////////////////////////////////////////////////////////////////
// 观测系统
/**
 * @brief 观测系统工作状态
 */
struct KVOBSS : public KVBase {
    int  state;      ///< 工作状态. 0: 未启动; 1: 已启动
    int  mount;      ///< 转台联机标志
    int  camera;     ///< 相机联机标志

public:
    KVOBSS() {
        type = KVTYPE_OBSS;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("state",  state);
        ss << join_kv("mount",  mount);
        ss << join_kv("camera", camera);
        ss << std::endl;
        return ss.str();
    }
};

//////////////////////////////////////////////////////////////////////////////
// 转台
/**
 * @brief 控制转台指向位置并在到达位置后转入跟踪或静止
 */
struct KVSlewto : public KVBase {
    int    coorsys;     ///< 0: 赤道; 1: 地平; 2: TLE
    double ra;          ///< 赤经, 角度
    double dec;         ///< 赤纬, 角度
    double epoch;       ///< 历元. 0.0=当前历元
    double azi;         ///< 方位角, 角度, 南零点
    double ele;         ///< 俯仰角, 角度
    string tle1;        ///< 第一行TLE根数
    string tle2;        ///< 第二行TLE根数

public:
    KVSlewto() {
        type = KVTYPE_SLEWTO;
        coorsys   = 0;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("coorsys", coorsys);
        if (coorsys == COORSYS_EQUA) {
            ss << join_kv("ra",    ra);
            ss << join_kv("dec",   dec);
            ss << join_kv("epoch", epoch);
        }
        else if (coorsys == COORSYS_ALTAZ) {
            ss << join_kv("azi",  azi);
            ss << join_kv("ele",  ele);
        }
        else {
            ss << join_kv("tle1", tle1);
            ss << join_kv("tle2", tle2);
        }
        ss << std::endl;
        return ss.str();
    }
};

/**
 * @brief 转台复位
 */
struct KVPark : public KVBase {
public:
    KVPark() {
        type = KVTYPE_PARK;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << std::endl;
        return ss.str();
    }
};

/**
 * @brief 导星
 */
struct KVGuide : public KVBase {
	int result = 0;		///< 导星结果. 1: 指向偏差小于阈值
	int op     = 0;		///< 导星操作. 0: 导星结束; 1: 开始导星
    int ra  = 0;   ///< 图像中心赤经, 角秒
    int dec = 0;   ///< 图像中心赤纬, 角秒

public:
    KVGuide() {
        type = KVTYPE_GUIDE;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
		ss << join_kv("result", result);
		ss << join_kv("op",     op);
		if (fabs(ra) > 0 || fabs(dec) > 0) {
	        ss << join_kv("ra",     ra);
	        ss << join_kv("dec",    dec);
		}
        ss << std::endl;
        return ss.str();
    }
};

/**
 * @brief 转台搜索零点
 */
struct KVHome : public KVBase {
public:
    KVHome() {
        type = KVTYPE_HOME;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << std::endl;
        return ss.str();
    }
};

/**
 * @brief 转台同步零点
 */
struct KVSync : public KVBase {
    double ra;          ///< 赤经, 角度
    double dec;         ///< 赤纬, 角度
    double epoch;       ///< 历元. 0.0=当前历元

public:
    KVSync() {
        type = KVTYPE_SYNC;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("ra",    ra);
        ss << join_kv("dec",   dec);
        ss << join_kv("epoch", epoch);
        ss << std::endl;
        return ss.str();
    }
};

/**
 * @brief 转台实时工作状态
 */
struct KVMount : public KVBase {
    int    state;   ///< 工作状态. 由StateMount定义
    int    errcode; ///< 故障字
    double mjd = -1;     ///< 修正儒略日, UTC
    double lst = -1;     ///< 本地恒星时, 小时
    double ra = 1000;      ///< 当前指向赤经, 视位置, 角度
    double dec = 1000;     ///< 当前指向赤纬, 视位置, 角度
    double ra2k = 1000;    ///< 当前指向赤经, 历元2000, 角度
    double dec2k = 1000;   ///< 当前指向赤纬, 历元2000, 角度
    double azi = 1000;     ///< 当前指向方位角, 角度, 南零点  <-- 视位置
    double ele = 1000;     ///< 当前指向俯仰角, 角度         <-- 视位置
	// 目标位置, 不参与通信协议
	double objra = 1000;	///< 目标位置: 赤经, 角度
	double objdec = 1000;	///< 目标位置: 赤纬, 角度

public:
    KVMount() {
        type  = KVTYPE_MOUNT;
		state = MOUNT_ERROR;
		errcode = 1;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("state",   state);
        ss << join_kv("errcode", errcode);
        ss << join_kv("mjd",     mjd);
        ss << join_kv("lst",     lst);
        ss << join_kv("ra",      ra);
        ss << join_kv("dec",     dec);
        ss << join_kv("ra2k",    ra2k);
        ss << join_kv("dec2k",   dec2k);
        ss << join_kv("azi",     azi);
        ss << join_kv("ele",     ele);
        ss << std::endl;
        return ss.str();
    }
};
// 转台
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 相机
/**
 * @brief 采集图像: 手动曝光; 不与转台状态联动
 */
struct KVTakeImage : public KVAppGWAC {
public:
    KVTakeImage() {
        type = KVTYPE_TKIMG;
    }

    string ToString() const {
        return KVAppGWAC::ToString();
    }
};

/**
 * @brief 曝光指令
 */
struct KVExpose : public KVBase {
    int command;    ///< 由CommandExpose定义
	int frmno; 		///< 起始帧序号

public:
    KVExpose() {
        type = KVTYPE_EXPOSE;
		frmno= 0;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("command", command);
		ss << join_kv("frmno",   frmno);
        ss << std::endl;
        return ss.str();
    }
};

/**
 * @brief 相机工作参数:
 */
struct KVCamSet : public KVBase {
    int  opType;    ///< 0: 查询; 1: 状态; 2: 修改
    /* 档位. 未修改项, 填入当前值 */
    int  bitDepth;  ///< 数字带宽
    int  iADC;      ///< A/D通道
    int  iReadPort; ///< 读出端口
    int  iReadRate; ///< 读出速度
    int  iVSRate;   ///< 行转移速度
    int  iGain;     ///< 前置增益
    int  coolSet;   ///< 制冷温度
    /* 对应数值 */
    int    bitPixel;///< 数字带宽
    string ADC;     ///< ADC通道名称
    string readPort;///< 读出端口名称
    string readRate;///< 读出速度
    double vsRate;  ///< 行转移速度
    double gain;    ///< 前置增益

public:
    KVCamSet() {
        type   = KVTYPE_CAMSET;
        opType = -1;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("optype", opType);
        if (opType) {
            if (opType == 2) {
                ss << join_kv("bitDepth",  bitDepth);
                ss << join_kv("iADC",      iADC);
                ss << join_kv("iReadPort", iReadPort);
                ss << join_kv("iReadRate", iReadRate);
                ss << join_kv("iVSRate",   iVSRate);
                ss << join_kv("iGain",     iGain);
                ss << join_kv("coolSet",   coolSet);
            }
            ss << join_kv("bitPixel", bitPixel);
            ss << join_kv("ADC",      ADC);
            ss << join_kv("readPort", readPort);
            ss << join_kv("readRate", readRate);
            ss << join_kv("vsRate",   vsRate);
            ss << join_kv("gain",     gain);
        }
        ss << std::endl;
        return ss.str();
    }
};

/**
 * @brief 相机实时工作状态
 */
struct KVCamera : public KVBase {
    int    state;   ///< 工作状态, 由StateCameraControl定义
    int    errcode; ///< 故障字
    double left;    ///< 曝光剩余时间
    double percent; ///< 曝光进度, 百分比
    int    coolget; ///< 芯片温度
    string imgtype; ///< 图像类型
    string filter;  ///< 滤光片名称
    int    freedisk;///< 可用磁盘空间, GB
    string plan_sn; ///< 计划编号
    int    loopno;  ///< 循环序号
    int    frmno;   ///< 帧序号
    string fileName;    ///< 最后一个FITS文件的名称

public:
    KVCamera() {
        type = KVTYPE_CAMERA;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("state",    state);
        ss << join_kv("errcode",  errcode);
        ss << join_kv("left",     left);
        ss << join_kv("percent",  percent);
        ss << join_kv("coolget",  coolget);
        ss << join_kv("imgtype",  imgtype);
        ss << join_kv("filter",   filter);
        ss << join_kv("freedisk", freedisk);
        ss << join_kv("plan_sn",  plan_sn);
        ss << join_kv("loopno",   loopno);
        ss << join_kv("frmno",    frmno);
        ss << join_kv("filename", fileName);
        ss << std::endl;
        return ss.str();
    }
};
// 相机
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 调焦
/**
 * @brief 开环调焦: 指令和位置
 */
struct KVFocus : public KVBase {
    int opType = 0;     ///< 0: 位置; 1: 控制
	int state  = 0;		///< 0: 静止; 1: 定位中
    int relPos = 0;     ///< 相对调焦量
    int pos = 999999;       ///< 当前位置. 无效值=999999
	int posTar = 999999;	///< 目标位置. 无效值=999999

public:
    KVFocus() {
        type   = KVTYPE_FOCUS;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("optype", opType);
        if (opType == 0) {
			ss << join_kv("state",    state);
            ss << join_kv("pos",      pos);
            ss << join_kv("posTar",   posTar);
        }
        else if (opType == 1) {
            ss << join_kv("relpos",   relPos);
        }
        ss << std::endl;
        return ss.str();
    }
};

struct KVFWHM : public KVBase {
    double fwhm;    ///< 半高全宽
    string tmimg;   ///< FWHM对应的图像时标

public:
    KVFWHM() {
        type = KVTYPE_FWHM;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("fwhm",  fwhm);
        ss << join_kv("tmimg", tmimg);
        ss << std::endl;
        return ss.str();
    }
};
// 调焦
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 消旋器
struct KVDerot : public KVBase {
    int    opType;    ///< 0: 位置; 1: 控制
    int    command;   ///< 1: 定位; 0: 停止
    int    state;     ///< 状态. 1: 运动; 0: 停止
    double posTar;    ///< 目标位置, 角度
    double pos;       ///< 实时位置, 角度

public:
    KVDerot() {
        type = KVTYPE_DEROT;
        opType = -1;
        command= -1;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("optype",  opType);
        if (opType == 0) {
            ss << join_kv("state",   state);
            ss << join_kv("pos",     pos);
        }
        else if (opType == 1) {
            ss << join_kv("command", command);
            ss << join_kv("postar",  posTar);
        }
        ss << std::endl;
        return ss.str();
    }
};
// 消旋器
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 圆顶
struct KVDome : public KVBase {
    int    opType;    ///< 0: 位置; 1: 控制
    int    command;   ///< 在CommandDome中定义
    int    state;     ///< 天窗状态, 在StateSlit中定义
    double azi;       ///< 天窗方位, 角度, 南零点
    double ele;       ///< 天窗高度, 角度
    double aziObj;    ///< 天窗目标方位, 角度, 南零点
    double eleObj;    ///< 天窗目标高度, 角度

public:
    KVDome() {
        type = KVTYPE_DOME;
        opType = -1;
        command= -1;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("optype",  opType);
        if (opType == 0) {
            ss << join_kv("state",   state);
            ss << join_kv("azi",     azi);
            ss << join_kv("ele",     ele);
        }
        else if (opType == 1) {
            ss << join_kv("command", command);
        }
        ss << join_kv("aziobj",     aziObj);
        ss << join_kv("eleobj",     eleObj);
        ss << std::endl;
        return ss.str();
    }
};
// 圆顶
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 镜盖
struct KVMirrCover : public KVBase {
    int    opType;    ///< 0: 位置; 1: 控制
    int    command;   ///< 在CommandMirrorCover中定义
    int    state;     ///< 镜盖状态, 在StateMirrorCover中定义

public:
    KVMirrCover() {
        type = KVTYPE_MCOVER;
        opType = -1;
        command= -1;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("optype",  opType);
        if (opType == 0) {
            ss << join_kv("state",   state);
        }
        else if (opType == 1) {
            ss << join_kv("command", command);
        }
        ss << std::endl;
        return ss.str();
    }
};
// 镜盖
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 滤光片: 单独控制
struct KVFilter : public KVBase {
    int    opType;    ///< 0: 位置; 1: 控制
    string name;      ///< 滤光片名称

public:
    KVFilter() {
        type   = KVTYPE_FILTER;
        opType = -1;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("optype", opType);
        ss << join_kv("name",   name);
        ss << std::endl;
        return ss.str();
    }
};
// 滤光片
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// 测站位置
struct KVGeoSite : public KVBase {
    int    opType;  ///< 0: 查询; 1: 状态; 2: 修改
    string name;    ///< 测站名称
    double lon;     ///< 地理经度, 角度, 东经+
    double lat;     ///< 地理纬度, 角度, 北纬+
    double alt;     ///< 海拔高度

public:
    KVGeoSite() {
        type   = KVTYPE_GEOSITE;
        opType = -1;
    }

    string ToString() const {
        std::stringstream ss;
        ss << KVBase::ToString();
        ss << join_kv("optype", opType);
        if (opType) {
            ss << join_kv("name",   name);
            ss << join_kv("lon",    lon);
            ss << join_kv("lat",    lat);
            ss << join_kv("alt",    alt);
        }
        ss << std::endl;
        return ss.str();
    }
};
// 测站位置
//////////////////////////////////////////////////////////////////////////////

typedef boost::shared_ptr<KVAppPlan>    KVAppPlanPtr;
typedef boost::shared_ptr<KVAppGWAC>    KVAppGWACPtr;
typedef boost::shared_ptr<KVCheckPlan>  KVChkPlanPtr;
typedef boost::shared_ptr<KVRemovePlan> KVRmvPlanPtr;
typedef boost::shared_ptr<KVPlan>       KVPlanPtr;
typedef boost::shared_ptr<KVAbort>      KVAbtPtr;
typedef boost::shared_ptr<KVOBSS>       KVObssPtr;
typedef boost::shared_ptr<KVSlewto>     KVSlewPtr;
typedef boost::shared_ptr<KVPark>       KVParkPtr;
typedef boost::shared_ptr<KVGuide>      KVGuidePtr;
typedef boost::shared_ptr<KVHome>       KVHomePtr;
typedef boost::shared_ptr<KVSync>       KVSyncPtr;
typedef boost::shared_ptr<KVMount>      KVMountPtr;
typedef boost::shared_ptr<KVTakeImage>  KVTkImgPtr;
typedef boost::shared_ptr<KVExpose>     KVExpPtr;
typedef boost::shared_ptr<KVCamSet>     KVCamSetPtr;
typedef boost::shared_ptr<KVCamera>     KVCamPtr;
typedef boost::shared_ptr<KVFocus>      KVFocusPtr;
typedef boost::shared_ptr<KVFWHM>       KVFwhmPtr;
typedef boost::shared_ptr<KVDerot>      KVDerotPtr;
typedef boost::shared_ptr<KVDome>       KVDomePtr;
typedef boost::shared_ptr<KVMirrCover>  KVMCoverPtr;
typedef boost::shared_ptr<KVFilter>     KVFilPtr;
typedef boost::shared_ptr<KVGeoSite>    KVSitePtr;

#endif
