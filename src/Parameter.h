/**
 * @file Parameter.h 声明软件使用的配置参数
 * @author 卢晓猛 (lxm@nao.cas.cn)
 * @brief
 * @version 0.1
 * @date 2024-01-02
 *
 * © ARTD Group, NAOC
 *
 */
#ifndef PARAMETER_H
#define PARAMETER_H

#include <string>

using std::string;

struct Parameter
{
	/* 成员变量 */
	// 服务端口
	int portClient	= 5010;	//< 客户端
	int portMount	= 5011;	//< 转台
	int portCamera	= 5012;	//< 相机
	int portFocus   = 5013;	//< 调焦

	// 数据库
	bool dbEnabled  = false;//< 标志: 启用
	string dbUrl    = "";	//< URL地址

	// 测站位置
	string siteName = "Xinglong";	//< 名称
	double siteLon  = 117.57454;	//< 地理经度, 角度, 东经为正
	double siteLat  = 40.39593;		//< 地理纬度, 角度, 北纬为正
	double siteAlt  = 900;		//< 海拔, 米

public:
	// 初始化配置参数
	bool Init(const string& filepath);
	// 加载配置参数
	bool Load(const string& filepath);
	// 保存配置参数
	bool Save(const string& filepath);
};

#endif
