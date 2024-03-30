/*!
 * @file AstroDeviceDef.h 声明与光学天文望远镜观测相关的状态、指令、类型等相关的常量
 * @version 0.1
 * @date 2017-11-24
 * @version 0.2
 * @date 2020-11-01
 * @note
 * - 重新定义enum类型名称和数值
 * - 重新定义enum类型对应字符串的获取方式
 */

#ifndef ASTRO_DEVICE_DEFINE_H_
#define ASTRO_DEVICE_DEFINE_H_

/////////////////////////////////////////////////////////////////////////////
/*!
 * @brief 网络工作环境下, 终端设备的归类
 */
enum {///< 对应主机类型
	PEER_CLIENT,	 ///< 客户端
	PEER_MOUNT_GWAC, ///< 转台, GWAC
	PEER_MOUNT_GFT,  ///< 转台, GFT
	PEER_CAMERA_GWAC,///< 相机, GWAC
	PEER_CAMERA_GFT, ///< 相机, GFT
	PEER_FOCUS,		 ///< 调焦//GWAC
	PEER_DATAPROC,	 ///< 数据处理
	PEER_LAST		 ///< 占位, 不使用
};

/* 状态与指令 */
/////////////////////////////////////////////////////////////////////////////
enum {///< 坐标系类型
	COORSYS_MIN = -1,
	COORSYS_ALTAZ,	///< 地平系
	COORSYS_EQUA,	///< 赤道系
	COORSYS_ORBIT,	///< 引导
	COORSYS_MAX
};

static const char* DESC_TYPE_COORSYS[] = {
	"AltAzimuth",
	"Equatorial",
	"TwoLineElement"
};

/////////////////////////////////////////////////////////////////////////////
enum {// 转台状态
	MOUNT_MIN = -1,
	MOUNT_ERROR,	///< 错误
	MOUNT_FREEZE,	///< 静止
	MOUNT_HOMING,	///< 找零
	MOUNT_HOMED,	///< 找到零点
	MOUNT_PARKING,	///< 复位
	MOUNT_PARKED,	///< 已复位
	MOUNT_SLEWING,	///< 指向
	MOUNT_TRACKING,	///< 跟踪
	MOUNT_GUIDING,	///< 导星
	MOUNT_MAX
};

static const char* DESC_STATE_MOUNT[] = {
	"Error",
	"Freeze",
	"Homing",
	"Homed",
	"Parking",
	"Parked",
	"Slewing",
	"Tracking",
	"Guiding"
};

/////////////////////////////////////////////////////////////////////////////
enum {// 消旋状态
	DEROT_MIN = -1,
	DEROT_ERROR,	//< 错误
	DEROT_TRACK,	//< 跟踪/静止
	DEROT_MOVING,	//< 运动/指向
	DEROT_MAX
};

static const char* DESC_STATE_DEROT[] = {
	"Error",
	"Tracking",
	"Slewing"
};

/////////////////////////////////////////////////////////////////////////////
enum {// 圆顶指令
	DOME_MIN = -1,
	DOME_CLOSE,	///< 关闭
	DOME_OPEN,	///< 打开
	DOME_STOP,	///< 停止
	DOME_MOVABS,	///< 转动: 绝对位置
	DOME_MOVREL,	///< 转动: 相对位置
	DOME_MAX
};

static const char* DESC_CMD_SLIT[] = {
	"Close",
	"Open",
	"Stop",
	"MoveAbs",
	"MoveRel"
};

enum {// 天窗状态
	SLIT_MIN = -1,
	SLIT_ERROR,		///< 错误
	SLIT_OPENING,	///< 打开中
	SLIT_OPEN,		///< 已打开
	SLIT_FULLY_OPEN,///< 完全打开
	SLIT_CLOSING,	///< 关闭中
	SLIT_CLOSED,	///< 已关闭
	SLIT_MAX
};

static const char* DESC_STATE_SLIT[] = {
	"Error",
	"Opening",
	"Open",
	"Fully Open",
	"Closing",
	"Closed"
};

/////////////////////////////////////////////////////////////////////////////
enum {// 镜盖指令
	MCC_MIN = -1,
	MCC_CLOSE,	///< 关闭镜盖
	MCC_OPEN,	///< 打开镜盖
	MCC_MAX
};

static const char* DESC_CMD_MCOVER[] = {
	"Close",
	"Open"
};

/////////////////////////////////////////////////////////////////////////////
enum {///< 镜盖状态
	MC_MIN = -1,
	MC_ERROR,	///< 错误
	MC_OPENING,	///< 正在打开
	MC_OPEN,	///< 已打开
	MC_CLOSING,	///< 正在关闭
	MC_CLOSED,	///< 已关闭
	MC_MAX
};

static const char* DESC_STATE_MCOVER[] = {
	"Error",
	"Opening",
	"Opened",
	"Closing",
	"Closed"
};

/////////////////////////////////////////////////////////////////////////////
enum {///< 调焦器状态
	FOCUS_MIN = -1,
	FOCUS_ERROR,	///< 错误
	FOCUS_FREEZE,	///< 静止
	FOCUS_MOVING,	///< 定位
	FOCUS_MAX
};

static const char* DESC_STATE_FOCUS[] = {
	"Error",
	"Freeze",
	"Moving"
};

/////////////////////////////////////////////////////////////////////////////
enum {///< 图像类型
	IMGTYP_MIN = -1,
	IMGTYP_BIAS,	///< 本底
	IMGTYP_DARK,	///< 暗场
	IMGTYP_FLAT,	///< 平场
	IMGTYP_OBJECT,	///< 目标
	IMGTYP_LIGHT = IMGTYP_OBJECT,	///< 天光
	IMGTYP_FOCUS,	///< 调焦
	IMGTYP_MAX
};

static const char* DESC_TYPE_IMGTYPE[] = {
	"BIAS",
	"DARK"
	"FLAT",
	"OBJECT"
	"FOCUS"
};

/////////////////////////////////////////////////////////////////////////////
enum {///< 相机控制指令
	EXP_MIN = -1,
	EXP_START,	///< 开始曝光
	EXP_STOP,	///< 中止曝光
	EXP_PAUSE,	///< 暂停曝光
	EXP_MAX
};

static const char* DESC_CMD_EXPOSE[] = {
	"Start",
	"Stop",
	"Pause",
};

/////////////////////////////////////////////////////////////////////////////
/* 相机工作状态: 底层 */
enum {
	CAMERA_MIN = -1,
	CAMERA_ERROR,	///< 错误
	CAMERA_IDLE,	///< 空闲
	CAMERA_EXPOSE,	///< 曝光
	CAMERA_IMGRDY,	///< 图像已准备完毕, 可存为文件或显示等
	CAMERA_MAX
};

static const char* DESC_STATE_CAMERA[] = {
	"Error",
	"Idle",
	"Exposing",
	"ImageReady"
};

/* 相机工作状态: 上层 */
enum {///< 相机调度状态
	CAMCTL_MIN = -1,
	CAMCTL_ERROR,			///< 错误. <-- CAMERA_ERROR
	CAMCTL_IDLE,			///< 空闲. <-- CAMERA_IDLE
	CAMCTL_EXPOSING,		///< 曝光过程中. <-- CAMERA_EXPOSE
	CAMCTL_IMGRDY,			///< 已完成曝光. <-- CAMERA_IMGRDY
	CAMCTL_PAUSEED,			///< 已暂停曝光. <-- CAMERA_IDLE + EXP_PAUSE/内部逻辑
	CAMCTL_WAIT_TIME,		///< 曝光序列开始前延时等待
	CAMCTL_WAIT_FLAT,		///< 平场间延时等待
	CAMCTL_MAX
};

static const char* DESC_STATE_CAMCTL[] = {
	"Error",
	"Idle",
	"Exposing",
	"ImageReady",
	"Paused",
	"WaitTime",
	"WaitFlat"
};

/////////////////////////////////////////////////////////////////////////////
enum {///< 观测计划状态
	OBSPLAN_MIN = -1,
	OBSPLAN_ERROR,		///< 错误. 特指不存在plan_sn指向的计划
	OBSPLAN_CATALOGED,	///< 入库
	OBSPLAN_ALLOCATED,	///< 已分配
	OBSPLAN_WAITING,	///< 等待执行
	OBSPLAN_RUNNING,	///< 执行中
	OBSPLAN_OVER,		///< 完成
	OBSPLAN_INTERRUPTED,///< 中断
	OBSPLAN_ABANDONED,	///< 自动抛弃
	OBSPLAN_DELETED,	///< 手动删除
	OBSPLAN_MAX
};

static const char *DESC_STATE_OBSPLAN[] = {
	"Error",
	"Cataloged",
	"Allocated",
	"Waiting",
	"Running",
	"Over",
	"Interrupted",
	"Abandoned",
	"Deleted"
};

/////////////////////////////////////////////////////////////////////////////
enum {///< 观测时间分类
	ODT_MIN = -1,
	ODT_DAYTIME,///< 白天, 可执行BIAS\DARK\FOCUS计划
	ODT_FLAT,	///< 平场, 可执行平场计划
	ODT_NIGHT,	///< 夜间, 可执行非平场计划
	ODT_MAX
};

static const char* DESC_TYPE_ODT[] = {
	"Daytime",
	"Flat",
	"Night"
};

#endif
