/*!
 * @file GLog.h  类GLog声明文件
 * @author       卢晓猛
 * @description  日志文件访问接口
 * 使用互斥锁管理文件写入操作, 将并行操作转换为串性操作, 避免日志混淆
 * @version      2.0
 * @date         2020年9月30日
 * - 使用标准c/c++库替代boost库
 */

#ifndef SRC_GLOG_H_
#define SRC_GLOG_H_

#include <stdio.h>
#include <string>
#include <ctime>
#include <mutex>

enum LOG_TYPE {// 日志类型
	LOG_NORMAL,	///< 普通
	LOG_WARN,	///< 警告
	LOG_FAULT	///< 错误
};

class GLog {
public:
	GLog(FILE *out = NULL);
	GLog(const char* dirName, const char* fileNamePrefix);
	virtual ~GLog();

public:
	/*!
	 * @brief 记录日志
	 * @param where   日志发生位置
	 * @param type    日志类型
	 * @param format  日志格式
	 */
	void Write(const char *format, ...);
	void Write(LOG_TYPE type, const char *format, ...);
	void Write(const char *where, LOG_TYPE type, const char *format, ...);

protected:
	/*!
	 * @brief 依据时间检查是否需要创建新的日志文件
	 * @return
	 * 检查并创建日志文件
	 */
	bool valid_file(std::tm &loctm);

protected:
	typedef std::unique_lock<std::mutex> mutex_lock;
	/* 成员变量 */
	std::mutex	mtx_;		//< 互斥锁
	FILE	*fd_;		//< 文件描述符
	std::string	dirName_;	//< 日志目录
	std::string prefix_;	//< 日志文件名前缀
	int			dayOld_;	//< UTC日期
};
extern GLog _gLog;		//< 工作日志

#endif /* SRC_GLOG_H_ */
