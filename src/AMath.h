/**
 Name        : AMath.h 常用数学函数声明文件
 Author      : Xiaomeng Lu
 Version     : 0.1
 Date        : Oct 13, 2012
 Last Date   : Oct 13, 2012
 Description : 天文数字图像处理中常用的数学函数
 **/
#ifndef _AMATH_H_
#define _AMATH_H_

#include <math.h>
#include <vector>

namespace AstroUtil
{
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 大小端数据转换 -------------------------------*/
// 由于内部使用计算机大多数为Intel架构, 即小端顺序, 因此除特殊规定数据外, 其它自定义数据文件
// 均以小端顺序存储. 并对自定义数据提供根据CPU架构的大小端顺序执行自动转换
/*!
 * \brief 测试是否需要执行大小端转换
 * \return
 * 若测试结果为小端则返回true, 否则返回false
 **/
extern bool TestSwapEndian();
/*!
 * \brief 执行大小端转换
 * \param[in] array    待转换数组
 * \param[in] nelement 数组内成员数量
 * \param[in] ncell    数组内成员的字节长度
 **/
extern void SwapEndian(void* array, int nelement, int ncell);
/*------------------------------- 大小端数据转换 -------------------------------*/
// 计算不同地理纬度下GEO轨道的视赤纬, 返回: 角度
extern double VisualGeo(double lat);
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 部分星等转换 -------------------------------*/
/*!
 * \brief 立体角转换为平方角秒
 * \param[in] sr 立体弧度
 * \return
 * 立体角在单位球面上对应的面积, 量纲: 平方角秒
 */
extern double Sr2Arcsec(double sr);
/*!
 * \brief 平方角秒转换为立体角
 * \param[in] sas 平方角秒
 * \return
 * 单位球面上单位面积对应的立体张角, 量纲: 立体弧度
 */
extern double Arcsec2Sr(double sas);
/*!
 * \brief 在绝对星等和标准能量之间转换
 * \param[in] mag  星等
 * \param[in] watt 标准能量, 量纲: [W/m^2]
 */
extern double Mag2Watt(double mag);
extern double Watt2Mag(double watt);
/*!
 * \brief 在不同标准能量量纲之间转换
 * \param[in] cd   能量: [cd/cm^2] (cd: 坎德拉)
 * \param[in] watt 能量: [W/m^2/Sr]
 */
extern double Candela2Watt(double cd);
extern double Watt2Candela(double watt);
/*!
 * \brief 在光电子和星等之间转换
 * \param[in] mag        星等
 * \param[in] wavelength 波长, 量纲: nm
 * \param[in] photo      光电子计数
 */
extern double Mag2Photo(double mag, double wavelength);
extern double Photo2Mag(double photo, double wavelength);
/*------------------------------- 部分星等转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*@
 * 开发排序算法是为了减少固化时对开发库的依赖
 */
/*---------------------------------- 排序算法 ----------------------------------*/
/*!
 * \brief 从数组中返回第k个元素
 * \param[in] array 数组
 * \param[in] n     数组长度
 * \param[in] k     自小至大排序的数组中, 索引k对应的元素为函数返回值
 * \return
 * 索引k对应的元素值
 */
template<typename DType>
DType k_select(DType* array, int n, int k)
{
	if (k < 0 || k >= n) return (DType) -1;
	int i, j, ir, l, mid;
	DType a, t;
	bool bwhile = true;

	l = 0;
	ir= n - 1;
	do {
		if ((ir - l) <= 1) {// 判定区长度为1或者2
			if ((ir - l) == 1) {
				if (array[ir] < array[l]) {
					t = array[l];
					array[l] = array[ir];
					array[ir] = t;
				}
			}
			bwhile = false;
		}
		else {
			mid = (l + ir) / 2;
			t = array[mid];
			array[mid] = array[l + 1];
			array[l + 1] = t;
			if (array[l] > array[ir]) {
				t = array[l];
				array[l] = array[ir];
				array[ir] = t;
			}
			if (array[l + 1] > array[ir]) {
				t = array[l + 1];
				array[l + 1] = array[ir];
				array[ir] = t;
			}
			if (array[l] > array[l + 1]) {
				t = array[l];
				array[l] = array[l + 1];
				array[l + 1] = t;
			}
			i = l + 1;
			j = ir;
			a = array[l + 1];
			while (1) {
				while (array[++i] < a);
				while (array[--j] > a);
				if (j <= i) break;
				t = array[i];
				array[i] = array[j];
				array[j] = t;
			}
			array[l + 1] = array[j];
			array[j] = a;
			if (j >= k) ir = j - 1;
			if (j <= k) l = i;
		}
	}while(bwhile);

	return array[k];
}
/*---------------------------------- 排序算法 ----------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*--------------------------------- 误差系数 ---------------------------------*/
/**
 * \brief (高斯)误差分布函数数值计算. 公式来源:\n
 * 《Numerical Recipes in Fortran 77》 Page 214\n
 * 公式计算误差: max(error)<=1.2*10^-7\n
 * 数据比对验证: 误差函数表
 * \param[in] x 归一化的自变量, 即使用高斯函数期望值和标准方差归一化后的自变量：
 * \f$x=fract{x-mu}{sqrt{2}sigma}\f$
 * \return
 * 正态函数在[0,x]区间的积分值
 **/
extern double erf(double x);
/**
 * \brief erf()的逆函数, 计算误差函数对应的自变量位置
 * \param[in] z 误差函数
 * \return
 * 与正态函数数值z对应的自变量区间. 积分区间定义为[x1, x2]，x1==0, x2==x
 **/
extern double reverse_erf(double z);
/*!
 * \brief
 * 累计正态分布函数(Cumulative Normal Distribution Function)的缩写
 * 用于计算正态分布函数的累计概率密度
 * \param[in] x      自变量
 * \param[in] mu     正态分布函数的期望值
 * \param[in] sigma  正态分布函数的标准差
 * \return
 * 累计概率密度
 **/
extern double CNDF(double x, double mu, double sigma);
extern double RCNDF(double p, double mu, double sigma);
/*--------------------------------- 误差系数 ---------------------------------*/
///////////////////////////////////////////////////////////////////////////////
class AMath {
/* 数据类型 */
public:
	/*!
	 * @struct LUdcmp LU矩阵分解
	 */
	struct LUdcmp {
		int n;				/// n=矩阵高度=矩阵宽度
		int np;				/// 行置换次数. >= 0: 次数; < 0: 奇异矩阵
		double *luptr;		/// LU分解结果指针==输入矩阵指针
		std::vector<int> idx;	/// LU分解过程中的行置换索引

	public:
		LUdcmp() {
			n = np = -1;
			luptr = NULL;
		}

		/*!
		 * @brief 重置LU分解输入矩阵
		 * @param _n   矩阵宽度=矩阵高度=n
		 * @param ptr  矩阵存储地址转换为行优先存储的一维指针
		 */
		void Reset(int _n, double *_ptr) {
			if (_n != n) idx.resize(_n);
			n     = _n;
			np    = 0;
			luptr = _ptr;
		}

		/*!
		 * @brief 判断是否奇异矩阵
		 * @return
		 * 奇异矩阵标志
		 */
		bool IsSingular() {
			return np < 0;
		}
	};

public:
	AMath();
	virtual ~AMath();

protected:
	LUdcmp ludcmp_;		/// LU矩阵分解
	double SD0_, CD0_;	/// 投影中心纬轴的正余弦
	double A0_;			/// 投影中心的经轴, 量纲: 弧度

public:
//////////////////////////////////////////////////////////////////////////////
	/* 最小二乘法和矩阵基本运算 */
	/*!
	 * @brief 线性最小二乘拟合
	 * @param m  样本数量
	 * @param n  基函数数量
	 * @param x  每个基函数对每个自变量的计算结果, 其数学形式是二维数组.
	 *           每行对应一个基函数, 每列对应一个自变量.
	 *           数组维度是: n*m; 矩阵
	 * @param y  样本的因变量. 数组维度是: m*1; 矢量
	 * @param c  待拟合系数. 数组维度是: n*1; 矢量
	 * @return
	 * 拟合成功标志. true: 成功; false: 失败
	 * @note
	 * - 初始版本. 待升级: 使用范式函数替代自变量表述方式
	 * - 数组x和y由用户接口填充数据
	 * - 最小二乘拟合使用逆矩阵. 当矩阵无对应逆矩阵时, 求解失败.
	 *   例如: 样本数量不足
	 */
	bool LSFitLinear(int m, int n, double *x, double *y, double *c);
	/*!
	 * @brief LU分解: LU decompose
	 * @param n    矩阵维度
	 * @param a    n*n二维矩阵
	 * @return
	 * LU分解成功标志
	 * @note
	 * 分解结果存储在原矩阵中
	 */
	bool LUdcmp(int n, double *a);
	/*!
	 * @brief LU反向替代法求解未知参数
	 * @param b   等式右侧, n*1矢量
 	 * @param x   求解结果, n*1矢量
	 * @return
	 * 求解结果
	 * @note
	 * - 求解方程A×X=B中的X
	 * - 若A是奇异矩阵则求解失败
	 * - 调用顺序:
	 *   1. LUdcmp
	 *   2. LUsolve
	 */
	bool LUsolve(double *b, double *x);
	/*!
	 * @brief LU反向替代法求解未知参数
	 * @param m   矩阵宽度, 即列数
	 * @param b   等式右侧, n*m矩阵
 	 * @param x   求解结果, n*m矩阵
	 * @return
	 * 求解结果
	 * @note
	 * - 求解方程A×X=B中的X
	 * - 若A是奇异矩阵则求解失败
	 * - 调用顺序:
	 *   1. LUdcmp
	 *   2. LUsolve
	 */
	bool LUsolve(int m, double *b, double *x);
	/*!
	 * @brief 基于LU分解计算行列式
	 * @param n    矩阵维度
	 * @param a    n*n二维矩阵
	 * @return
	 * 行列式
	 * @note
	 * - 奇异矩阵返回0.0
	 */
	double LUDet(int n, double *a);
	/*!
	 * @brief 计算逆矩阵
	 * @param n     二维矩阵维度
	 * @param a     输入: 原始矩阵; 输出: 逆矩阵
	 * @return
	 * 逆矩阵求解结果. true: 成功; false: 失败
	 * @note
	 * - 使用LU分解和反向替代求解逆矩阵
	 */
	bool MatrixInvert(int n, double *a);
	/*!
	 * @brief 计算矩阵乘积
	 * @param m  矩阵L的高度
	 * @param p  矩阵L的宽度==矩阵R的高度
	 * @param n  矩阵R的宽度
	 * @param L  m*p矩阵
	 * @param R  p*n矩阵
	 * @param Y  矩阵乘积, m*n矩阵
	 * @note
	 * - Y的存储空间由用户管理
	 */
	void MatrixMultiply(int m, int p, int n, double *L, double *R, double *Y);
	/*!
	 * @brief 计算转置矩阵
	 * @param m     原始矩阵高度==转置矩阵宽度
	 * @param n     原始矩阵宽度==转置矩阵高度
	 * @param a     原始矩阵
	 * @param b     转置矩阵
	 * @note
	 * - 转置矩阵方法1: 使用m*n缓冲区, 算法复杂度m*n
	 */
	template <typename T>
	void MatrixTranspose(int m, int n, T *a, T *b) {
		int row, col;
		T *aptr, *bptr;

		for (row = 0, bptr = b; row < n; ++row) {
			for (col = 0, aptr = a + row; col < m; ++col, ++bptr, aptr += n) {
				*bptr = *aptr;
			}
		}
	}

public:
	///////////////////////////////////////////////////////////////////////////////
	/*------------------------------- 部分球坐标转换 -------------------------------*/
	/*!
	 * \brief 计算球上两点之间的距离
	 * \param[in] A1   位置1的alpha位置, 量纲: 弧度
	 * \param[in] D1   位置1的beta位置, 量纲: 弧度
	 * \param[in] A2   位置2的alpha位置, 量纲: 弧度
	 * \param[in] D2   位置2的beta位置, 量纲: 弧度
	 * \return
	 * 两点在球上的距离, 量纲: 弧度
	 **/
	double SphereRange(double A1, double D1, double A2, double D2);
	/*!
	 * \brief 将球坐标转换为笛卡尔坐标. 默认为右手坐标系, 且XY对应球坐标基平面, Z为极轴.
	 *        A从X轴正向逆时针增加, X轴对应0点; D为与XY平面的夹角, Z轴正向为正.
	 *        此转换中球坐标与赤道坐标系相同.
	 *        若球坐标为左手坐标系, 则A应取坐标系数值的负值
	 * \param[in] R     位置矢量的模. 对于天球坐标系取为1
	 * \param[in] A     经轴, 量纲: 弧度
	 * \param[in] D     纬轴, 量纲: 弧度
	 * \param[out] x    X轴坐标
	 * \param[out] y    Y轴坐标
	 * \param[out] z    Z轴坐标
	 **/
	void Sphere2Cart(double R, double A, double D, double& x, double& y, double& z);
	/*!
	 * \brief 将笛卡尔坐标转换为球坐标. 默认为右手坐标系, 且XY对应球坐标基平面, Z为极轴.
	 *        A从X轴正向逆时针增加, X轴对应0点; D为与XY平面的夹角, Z轴正向为正.
	 *        此转换中球坐标与赤道坐标系相同.
	 *        若球坐标为左手坐标系, 则A为坐标系数值的负值
	 * \param[in] x    X轴坐标
	 * \param[in] y    Y轴坐标
	 * \param[in] z    Z轴坐标
	 * \param[out] R   位置矢量的模. 对于天球坐标系取为1
	 * \param[out] A   经轴, 量纲: 弧度
	 * \param[out] D   纬轴, 量纲: 弧度
	 **/
	void Cart2Sphere(double x, double y, double z, double& R, double& A, double& D);
	/*!
	 * \brief (A, D)在以(A0, D0)为极轴的球坐标系内的位置. 默认为右手坐标系.
	 * \param[in] A0   新极轴的经轴坐标, 量纲: 弧度
	 * \param[in] D0   新极轴的纬轴坐标, 量纲: 弧度
	 * \param     A    输入时为在原坐标系内的位置, 输出为在新坐标系中的位置, 量纲: 弧度
	 * \param     D    输入时为在原坐标系内的位置, 输出为在新坐标系中的位置, 量纲: 弧度
	 **/
	void PolarForward(double A0, double D0, double& A, double& D);
	/*!
	 * \brief 将在以(A0, D0)为极轴的球坐标系内的位置(A, D), 还原为原始球坐标位置. 默认为右手坐标系.
	 * \param[in] A0   新极轴的经轴坐标, 量纲: 弧度
	 * \param[in] D0   新极轴的纬轴坐标, 量纲: 弧度
	 * \param     A    输入时为在新坐标系内的位置, 输出为在原坐标系中的位置, 量纲: 弧度
	 * \param     D    输入时为在新坐标系内的位置, 输出为在原坐标系中的位置, 量纲: 弧度
	 **/
	void PolarReverse(double A0, double D0, double& A, double& D);
	/*!
	 * @brief 为批量投影转换准备共同的投影点变换
	 * \param[in] A0 投影中心对应的球坐标，对应在球坐标基准面内的角度. 对于赤道坐标系则为赤经, 量纲: 弧度
	 * \param[in] D0 投影中心对应的球坐标，对应在球坐标相对基准面的角度. 对于赤道坐标系则为赤纬, 量纲: 弧度
	 */
	void PrepareProject(double A0, double D0);
	/*!
	 * \brief 球坐标投影到平面坐标
	 * \param[in] A  目标对应的球坐标，对应在球坐标基准面内的角度. 对于赤雷晗翟蛭嗑�, 量纲: 弧度
	 * \param[in] D  目标对应的球坐标，对应在球坐标相对基准面的角度. 对于赤道坐标系则为赤纬, 量纲: 弧度
	 * \param[out] xi  (A, D)在以(A0, D0)为投影轴的理想平面中的投影, 在平面度量系中的I轴坐标
	 * \param[out] eta (A, D)在以(A0, D0)为投影轴的理想平面中的投影, 在平面度量系中的II轴坐标
	 **/
	void Sphere2Plane(double A, double D, double &xi, double &eta);
	/*!
	 * \brief 被投影的平面坐标转换到球坐标
	 * \param[in] xi  (A, D)在以(A0, D0)为投影轴的理想平面中的投影, 在平面度量系中的I轴坐标
	 * \param[in] eta (A, D)在以(A0, D0)为投影轴的理想平面中的投影, 在平面度量系中的II轴坐标
	 * \param[out] A  目标对应的球坐标，对应在球坐标基准面内的角度. 对于赤道坐标系则为赤经, 量纲: 弧度
	 * \param[out] D  目标对应的球坐标，对应在球坐标相对基准面的角度. 对于赤道坐标系则为赤纬, 量纲: 弧度
	 **/
	void Plane2Sphere(double xi, double eta, double &A, double &D);
	/*------------------------------- 部分球坐标转换 -------------------------------*/

public:
	///////////////////////////////////////////////////////////////////////////////
	/*----------------------------------- 内插 -----------------------------------*/
	/*!
	 * \brief 计算一元三次样条函数的内插系数
	 * \param[in]  n  已知采样位置的数量
	 * \param[in]  x  已知采样位置的横轴坐标, 以数组存储
	 * \param[in]  y 已知采样位置的纵轴坐标, 以数组存储
	 * \param[in]  c1 第一个采样点的微分
	 * \param[in]  cn 最后一个采样点的微分
	 * \param[out] c  内插系数
	 * \note
	 * 若第一个和最后一个位置的微分未知, 则将c1和cn置为1E30
	 */
	void spline(int n, double x[], double y[], double c1, double cn, double c[]);
	bool splint(int n, double x[], double y[], double c[], double xo, double& yo);
	/*!
	 * \brief 计算二元三次样条函数的内插系数
	 * \param[in]  nr 二维矩阵y的高度, 同时是一维矢量x1的长度
	 * \param[in]  nc 二维矩阵y的宽度, 同时是一维矢量x2的长度
	 * \param[in]  x1 函数y=f(x1,x2)中, 第一个自变量采样构成的矢量
	 * \param[in]  x2 函数y=f(x1,x2)中, 第二个自变量采校构成的矢量
	 * \param[in]  y  函数因变量对应采样
	 * \param[out] c  二维内插系数, 其大小与y相同, 即nrxnc
	 */
	void spline2(int nr, int nc, double x1[], double x2[], double y[], double c[]);
	bool splint2(int nr, int nc, double x1[], double x2[], double y[], double c[], double x1o, double x2o, double& yo);
	/*!
	 * \brief 使用双线性内插计算四点区域内某点的值
	 * \param[in]  XI 已知采样位置的X轴坐标, 以数组存储, 数组长度2
	 * \param[in]  YI 已知采样位置的Y轴坐标, 以数组存储, 数组长度2
	 * \param[in]  ZI 已知采样位置的Z轴坐标, 以数组存储, 数组长度2x2
	 * \param[in]  X0 未知采样位置的X轴坐标
	 * \param[in]  Y0 未知采样位置的Y轴坐标
	 * \return
	 * (X0, Y0)对应的数值
	 * \note
	 * ZI[]存储行优先顺序的四点数值, 即对应(x, y), (x + 1, y), (x, y + 1), (x + 1, y + 1)
	 */
	double Bilinear(double XI[], double YI[], double ZI[], double X0, double Y0);
	/*!
	 * \brief 拉格朗日内插
	 * \param[in]  N 已知采样位置的数量
	 * \param[in]  XI 已知采样位置的X轴坐标, 以数组存储
	 * \param[in]  YI 已知采样位置的Y轴坐标, 以数组存储
	 * \param[in]  OD 内插阶数, 即采用邻近的OD个数据点进行内插, 该值不能小于2
	 * \param[in]  M  未知采样位置的数量
	 * \param[in]  XO 未知采样位置的横轴坐标, 以数组存储
	 * \param[out] YO 未知采样位置的纵轴坐标, 即函数求解目标, 以数组存储
	 */
	void Lagrange(int N, double XI[], double YI[], int OD, int M, double XO[], double YO[]);
	/*----------------------------------- 内插 -----------------------------------*/
	///////////////////////////////////////////////////////////////////////////////
	/*--------------------------------- 相关系数 ---------------------------------*/
	/*!
	 * \brief 计算两组数的相关系数
	 * \param[in] N 数组长度
	 * \param[in] X 数组1
	 * \param[in] Y 数组2
	 * \return
	 * 相关系数
	 */
	double Correlation(int N, double X[], double Y[]);
	/*--------------------------------- 相关系数 ---------------------------------*/
};
///////////////////////////////////////////////////////////////////////////////
}
#endif
