/*
 * @file astro_common.h 天文常数及宏定义
 */

#ifndef ADEFINE_H_
#define ADEFINE_H_

#include <math.h>

namespace AstroUtil {
/*--------------------------------------------------------------------------*/
// 平面角转换系数
#define AU_PI		3.141592653589793238462643		//< 圆周率
#define AU_2PI		6.283185307179586476925287		//< 2倍圆周率
#define AU_R2D		5.729577951308232087679815E1	//< 弧度转换为角度
#define AU_D2R		1.745329251994329576923691E-2	//< 角度转换为弧度
#define AU_R2AS		2.062648062470963551564734E5	//< 弧度转换为角秒
#define AU_AS2R		4.848136811095359935899141E-6	//< 角秒转换为弧度
#define AU_H2D		15.0							//< 小时转换为角度
#define AU_D2H		6.666666666666666666666667E-2	//< 角度转换为小时
#define AU_H2R		2.617993877991494365385536E-1	//< 小时转换为弧度
#define AU_R2H		3.819718634205488058453210		//< 弧度转换为小时
#define AU_S2R		7.272205216643039903848712E-5	//< 秒转换为弧度
#define AU_R2S		1.375098708313975701043156E4	//< 弧度转换为秒
#define AU_D2AS		3600.0							//< 角度转换为角秒
#define AU_AS2D		2.777777777777777777777778E-4	//< 角秒转换为角度
#define AU_MAS		3600000			// 1度对应的毫角秒
#define AU_MAS5		18000000		// 5度对应的毫角秒
#define AU_MAS90	324000000		// 90度对应的毫角秒
#define AU_MAS180	648000000		// 180度对应的毫角秒
#define AU_MAS360	1296000000		// 360度对应的毫角秒

// 时间转换常数
#define AU_JD2K		2451545.0	//< 历元2000对应的儒略日
#define AU_MJD0		2400000.5	//< 修正儒略日零点所对应的儒略日
#define AU_MJD2K	51544.5		//< 历元2000对应的修正儒略日
#define AU_MJD77	43144.0		//< 1977年1月1日0时对应的修正儒略日
#define AU_TTMTAI	32.184		//< TTMTAI=TT-TAI
#define AU_DAYS_JY	365.25		//< 儒略历每年天数
#define AU_DAYS_JC	36525.0		//< 儒略历每世纪天数
#define AU_DAYS_JM	365250.0	//< 儒略历每千年天数
#define AU_DAYSEC	86400.0		//< 每日秒数

// 极限阈值
#define AU_EPS	1E-6			//< 最小值
#define AU_MAX	1E30			//< 最大值

// 计算正割函数
#define sec(x)			(1.0 / cos(x))
// 计算实数的小数部分
#define frac(x)			((x) - floor(x))
// 调整到[0, T)周期内
#define cycmod(x, T)	((x) - floor((x) / (T)) * (T))
/*--------------------------------------------------------------------------*/
}

#endif /* ADEFINE_H_ */
