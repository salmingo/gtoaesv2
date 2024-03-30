/*
 * @file mountproto.h 封装与转台相关的通信协议
 * @author         卢晓猛
 * @version        2.0
 * @date           2017年2月20日
 * ============================================================================
 * @date 2017年6月6日
 * - 参照asciiproto.h, 为buff_添加互斥锁
 * ============================================================================
 * @date 2024年1月12日
 * @file NonKVProtocol.h 封装与转台相关的通信协议
 */

#ifndef NONKV_PROTOCOL_H_
#define NONKV_PROTOCOL_H_

#include <string>
#include <limits.h>
#include "BoostInclude.h"

using std::string;

#define NONKVTYPE_STATE		"status"
#define NONKVTYPE_POS		"currentpos"
#define NONKVTYPE_FOCUS		"focus"
#define NONKVTYPE_RESPONSE	"rsp"

/*!
 * @brief 通信协议基类, 包含组标志
 */
struct NonKVBase {
	string type;///< 协议类型
	string gid;	///< 组标志
	string uid;	///< 单元标志
	string utc;	///< UTC时间
	int sn;		///< 序列号

public:
	NonKVBase() = default;
	virtual ~NonKVBase() = default;
};

/*!
 * @brief 转台实时工作状态
 */
struct NonKVStatus : public NonKVBase {
	int n;			//< 转台实际数量
	int8_t state[20];	//< 转台状态
					//< @li -1: 未定义
					//< @li  0: 错误
					//< @li  1: 静止
					//< @li  2: 正在找零点
					//< @li  3: 找到零点
					//< @li  4: 正在复位
					//< @li  5: 已复位
					//< @li  6: 指向目标
					//< @li  7: 跟踪中

public:
	NonKVStatus() {
		type = NONKVTYPE_STATE;
		n = 0;
	}
};

/*!
 * @brief 实时转台指向位置
 */
struct NonKVPosition : public NonKVBase {
	double ra;			//< 转台当前指向位置的赤经坐标, 量纲: 角度
	double dec;			//< 转台当前指向位置的赤纬坐标, 量纲: 角度

public:
	NonKVPosition() {
		type = NONKVTYPE_POS;
	}
};

/*!
 * @brief 焦点位置
 */
struct NonKVFocus : public NonKVBase {
	int pos[10];

public:
	NonKVFocus() {
		type = NONKVTYPE_FOCUS;
		for (int i = 0; i < 10; ++i) pos[i] = INT_MAX;
	}
};

struct MirrCoverState {// 镜盖开关状态
	string cid;	//< 相机标志
	int state;			//< 镜盖状态
						//	-2：正在关闭
						//	-1：已关闭
						//	1：已打开
						//	2：正在打开
						//	0：未知
public:
	MirrCoverState() {
		state = 0;
	}
};

/*!
 * @breif 镜盖状态
 */
struct NonKVMirrCoverState : public NonKVBase {
	int n;				//< 相机数量
	MirrCoverState state[10];	//< 镜盖状态

public:
	NonKVMirrCoverState() {
		type = "mirr";
	}
};

/**
 * @brief 指令回馈
 */
struct NonKVResponse : public NonKVBase {
public:
	NonKVResponse() {
		type = NONKVTYPE_RESPONSE;
	}
};

typedef boost::shared_ptr<NonKVBase> NonKVBasePtr;

class NonKVProtocol {
public:
	NonKVProtocol();
	NonKVProtocol(const string& gid, const string& uid);
	virtual ~NonKVProtocol();

protected:
	// 成员变量
	string gid_;	///< 组标志
	string uid_;	///< 单元标志
	boost::mutex mtxSerno_;	///< 序列号互斥锁
	int serno_ = 1;	///< 指令序列号

private:
	// 解析焦点位置
	int resolve_focus(const char* id);
	// 增加序列号
	int increase_serno();

public:
	/*!
	 * @brief 解析通信协议
	 * @param rcvd   从网络中收到的信息
	 * @return
	 * 协议. 若无法识别协议类型则返回空指针
	 */
	NonKVBasePtr Resolve(const char* rcvd);

public:
	/*!
	 * @brief 组装构建find_home指令
	 * @param serno 指令序号
	 * @param ra    赤经轴指令
	 * @param dec   赤纬轴指令
	 * @return
	 * 组装后协议
	 */
	string FindHome(int& serno, bool ra = true, bool dec = true);
	/*!
	 * @brief 组装构建home_sync指令
	 * @param serno 指令序号
	 * @param ra    当前转台指向位置对应的当前历元赤经位置, 量纲: 角度
	 * @param dec   当前转台指向位置对应的当前历元赤纬位置, 量纲: 角度
	 * @return
	 * 组装后协议
	 */
	string HomeSync(int& serno, double ra, double dec);
	/*!
	 * @brief 组装构建slew指令
	 * @param serno 指令序号
	 * @param ra    当前历元赤经位置, 量纲: 角度
	 * @param dec   当前历元赤纬位置, 量纲: 角度
	 * @return
	 * 组装后协议
	 */
	string Slew(int& serno, double ra, double dec);
	/*!
	 * @brief 组装构建slewhd指令
	 * @param serno    指令序号
	 * @param ha       当前历元时角位置, 量纲: 角度
	 * @param dec      当前历元赤纬位置, 量纲: 角度
	 * @return
	 * 组装后协议
	 */
	string SlewHD(int& serno, double ha, double dec);
	/*!
	 * @brief 组装构建guide指令
	 * @param serno  指令序号
	 * @param ra     赤经偏差量, 量纲: 角秒
	 * @param dec    赤纬偏差量, 量纲: 角秒
	 * @return
	 * 组装后协议
	 */
	string Guide(int& serno, int ra, int dec);
	/*!
	 * @brief 组装构建park指令
	 * @param serno  指令序号
	 * @return
	 * 组装后协议
	 */
	string Park(int& serno);
	/*!
	 * @brief 组装构建abort_slew指令
	 * @param serno  指令序号
	 * @return
	 * 组装后协议
	 */
	string AbortSlew(int& serno);
	/**
	 * @brief 组装构建track指令
	 * @param serno  指令序号
	 */
	string Track(int& serno);
	/**
	 * @brief 组装构建trackVel指令
	 * @param serno  指令序号
	 * @param ra     赤经速度, 量纲: 角秒@秒
	 * @param dec    赤纬速度, 量纲: 角秒@秒
	 * @note 设置转台自定义跟踪速度
	 */
	string TrackVelocity(int& serno, double ra, double dec);
	/*!
	 * @brief 组装构建focus指令
	 * @param serno  指令序号
	 * @param cid    相机标志
	 * @param tmobs  图像采集时间, 格式: hhmmsssss
	 * @param fwhm   星像FWHM, 量纲: 像素
	 * @return
	 * 组装后协议
	 */
	string FWHM(int& serno, const string& cid, const string& tmobs, double fwhm);
	/*!
	 * @brief 由focus组装构建focus指令
	 * @param serno  指令序号
	 * @param cid    相机标志
	 * @param pos    焦点位置
	 * @return
	 * 组装后协议
	 */
	string Focus(int& serno, const string& cid, int pos);
	/*!
	 * @brief 由focus组装构建focus_sync指令
	 * @param serno  指令序号
	 * @param cid    相机标志
	 * @return
	 * 组装后协议
	 */
	string FocusSync(int& serno, const string& cid);
};

#endif /* MOUNTPROTO_H_ */
