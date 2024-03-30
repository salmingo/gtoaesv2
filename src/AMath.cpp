/**
 Name        : AMath.cpp 常用数学函数定义文件
 Author      : Xiaomeng Lu
 Version     : 0.1
 Date        : Oct 13, 2012
 Last Date   : Oct 13, 2012
 Description : 天文数字图像处理中常用的数学函数
 **/

#include <stdlib.h>
#include <string.h>
#include "ADefine.h"
#include "AMath.h"

namespace AstroUtil
{
/*---------------------------------------------------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 大小端数据转换 -------------------------------*/
bool TestSwapEndian()
{
	short v = 0x0102;
	char* p = (char*) &v;
	return (p[0] == 2);
}

void SwapEndian(void* array, int nelement, int ncell)
{
	if (ncell % 2 != 0) return;	// 不满足转换条件
	int np = ncell - 1;
	char *p, *e, *p1, *p2;
	char ch;

	for (p = (char*) array, e = p + ncell * nelement; p < e; p += ncell) {
		for (p1 = p, p2 = p1 + np; p1 < p2; p1++, p2--) {
			ch = *p1;
			*p1 = *p2;
			*p2 = ch;
		}
	}
}
/*------------------------------- 大小端数据转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
// 计算不同地理纬度下GEO轨道的视赤纬, 返回: 角度
double VisualGeo(double lat) {
	double R(6378.135); // 地球半径, km
	double H(35786);	// GEO轨道高度
	double R1(R + H);
	double H1, sina;

	lat *= AU_D2R;
	H1 = R * R + R1 * R1 - 2 * R * R1 * cos(lat);
	sina = R * sin(lat) / sqrt(H1);
	return (lat >= 0.0 ? -asin(sina) : asin(sina)) * AU_R2D;
}

/*--------------------------------- 相关系数 ---------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 部分星等转换 -------------------------------*/
double Sr2Arcsec(double sr)
{
	return (sr * AU_R2AS * AU_R2AS);
}

double Arcsec2Sr(double sas)
{
	return (sas / AU_R2AS / AU_R2AS);
}

double Mag2Watt(double mag)
{
	return (1.78E-8 * pow(10., -0.4 * mag));
}

double Watt2Mag(double watt)
{
	return (-2.5 * log10(1E8 * watt / 1.78));
}

double Candela2Watt(double cd)
{
	return (10000. * cd / 683.);
}

double Watt2Candela(double watt)
{
	return (watt * 683. / 10000.);
}

double Mag2Photo(double mag, double wl)
{
	double h = 6.626176 * 1E-34; // 普朗克常数, [J.s]
	double c = 3.0 * 1E8;		 // 光速, [m/s]
	double watt = Mag2Watt(mag);
	double f = c / wl * 1E9;	 // 频率
	return (watt / h / f);
}

double Photo2Mag(double photo, double wl)
{
	double h = 6.626176 * 1E-34;
	double c = 3.0 * 1E8;
	double f = c / wl * 1E9;
	double watt = photo * h * f;
	return Watt2Mag(watt);
}
/*------------------------------- 部分星等转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*--------------------------------- 误差系数 ---------------------------------*/
double erf(double x)
{
    double t, z, y;
    z = fabs(x);
    t = 1.0 / (1.0 + 0.5 * z);
    y = t * exp(-z * z - 1.26551223 + t * ( 1.00002368 + t * ( 0.37409196 + t *
               ( 0.09678418 + t * (-0.18628806 + t * ( 0.27886807 + t *
               (-1.13520398 + t * ( 1.48851587 + t * (-0.82215223 + t * 0.17087277)))))))));
    if (x >= 0.0) y = 1.0 - y;
    else        y = y - 1.0;
    return y;
}

double reverse_erf(double z)
{
    double lo = -4;    // 默认对应-1
    double hi =  4;    // 默认对应+1
    if (z >= 0) lo = 0;
    else        hi = 0;
    double me = (lo + hi) * 0.5;
    double z1 = erf(me);
    int i = 1;
    while (fabs(z1 - z) > 1.2E-7) {
        if (z1 < z) lo = me;
        else        hi = me;
        me = (lo + hi) * 0.5;
        z1 = erf(me);
        ++i;
    }
    return me;
}

double CNDF(double x, double mu, double sigma)
{
    double z = (x - mu) / sqrt(2.0) / sigma;
    return 0.5 * (1 + erf(z));
}

double RCNDF(double p, double mu, double sigma)
{
    double z = reverse_erf(p * 2 - 1);
    double x = z * sqrt(2.0) * sigma + mu;
    return x;
}
/*--------------------------------- 误差系数 ---------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------*/
//////////////////////////////////////////////////////////////////////////////
AMath::AMath() {
	SD0_ = CD0_ = A0_ = 0.0;
}

AMath::~AMath() {
}

bool AMath::LSFitLinear(int m, int n, double *x, double *y, double *c) {
	bool rslt(false);
	// A*c=Y ==> c = A^-1*Y
	double *A  = new double[n * n];
	double *Y  = new double[n];
	double *L, *R, *Aptr, *Yptr, t;
	int i, j, k;

	bzero(A, sizeof(double) * n * n);
	bzero(Y, sizeof(double) * n);

	for (i = 0, Aptr = A, Yptr = Y; i < n; ++i, ++Yptr) {
		// 构建矩阵A
		for (j = 0; j < n; ++j, ++Aptr) {
			L = x + i * m;
			R = x + j * m;
			for (k = 0, t = 0.0; k < m; ++k, ++L, ++R) t += *L * *R;
			*Aptr = t;
		}

		// 构建矢量Y
		L = x + i * m;
		R = y;
		for (k = 0, t = 0.0; k < m; ++k, ++L, ++R) t += *L * *R;
		*Yptr = t;
	}
	// 拟合系数
	if (LUdcmp(n, A)) {
		LUsolve(Y, c);
		rslt = true;
	}

	delete []A;
	delete []Y;
	return rslt;
}

bool AMath::LUdcmp(int n, double *a) {
	ludcmp_.Reset(n, a);

	std::vector<int> &idx = ludcmp_.idx;
	int &np = ludcmp_.np;
	const double TINY = 1.0E-30;
	std::vector<double> scale(n);
	int i, j, k, imax(0);
	double t, big, *ptr, *ptr1, *ptr2;

	for (i = 0, ptr = a; i < n; ++i) {
		for (j = 0, big = 0.0; j < n; ++j, ++ptr) {
			if ((t = fabs(*ptr)) > big) big = t;
		}
		if (big == 0.0) {
			np = -1;
			return false;	// 奇异矩阵
		}
		scale[i] = 1.0 / big;
	}

	for (k = 0, np = 0, ptr = a; k < n; ++k, ptr += n) {// ptr: k行首, a[k][0]
		for (i = k, big = 0.0, ptr1 = ptr + k; i < n; ++i, ptr1 += n) {// ptr1初始: a[k][k]
			if ((t = scale[i] * fabs(*ptr1)) > big) {
				big  = t;
				imax = i;
			}
		}
		if (k != imax) {
			// ptr1初始: a[imax][0]
			// ptr2初始: a[k][0]
			for (j = 0, ptr1 = a + imax * n, ptr2 = ptr; j < n; ++j, ++ptr1, ++ptr2) {
				t     = *ptr1;
				*ptr1 = *ptr2;
				*ptr2 = t;
			}
			++np;
			scale[imax] = scale[k];
		}
		idx[k] = imax;
		if (ptr[k] == 0.0) ptr[k] = TINY;
		for (i = k + 1, ptr1 = ptr + n; i < n; ++i, ptr1 += n) {// ptr1: a[i][0]=a[k+1][0]
			t = ptr1[k] /= ptr[k];
			for (j = k + 1, ptr2 = ptr + j; j < n; ++j, ++ptr2) {// ptr2: a[k][j]
				ptr1[j] -= t * *ptr2;
			}
		}
	}

	return np >= 0;
}

bool AMath::LUsolve(double *b, double *x) {
	if (ludcmp_.IsSingular()) return false;
	int n = ludcmp_.n;
	int i, j, k, ii(0);
	double sum;
	std::vector<int> &idx = ludcmp_.idx;
	double *luptr;

	if (x != b) memcpy(x, b, sizeof(double) * n);
	for (i = 0, luptr = ludcmp_.luptr; i < n; ++i, luptr += n) {
		k    = idx[i];
		sum  = x[k];
		x[k] = x[i];
		if (ii) {
			for (j = ii - 1; j < i; ++j) sum -= luptr[j] * x[j];
		}
		else if (sum != 0.0) ii = i + 1;
		x[i] = sum;
	}
	for (i = n - 1, luptr = ludcmp_.luptr + i * n; i >= 0; --i, luptr -= n) {
		sum  = x[i];
		for (j = i + 1; j < n; ++j) sum -= luptr[j] * x[j];
		x[i] = sum / luptr[i];
	}

	return true;
}

bool AMath::LUsolve(int m, double *b, double *x) {
	if (ludcmp_.IsSingular()) return false;
	int n = ludcmp_.n;
	int i, j;
	double *col = new double[n];
	double *ptr;

	for (j = 0; j < m; ++j) {
		for (i = 0, ptr = b + j; i < n; ++i, ptr += m) col[i] = *ptr;
		LUsolve(col, col);
		for (i = 0, ptr = x + j; i < n; ++i, ptr += m) *ptr = col[i];
	}

	delete []col;
	return true;
}

double AMath::LUDet(int n, double *a) {
	if (ludcmp_.luptr == a && ludcmp_.np < 0) return 0.0;
	if (!LUdcmp(n, a)) return 0.0;

	int i;
	double *ptr, t(1.0);

	for (i = 0, ptr = a; i < n; ++i, ptr += (n + 1)) t *= *ptr;
	if (ludcmp_.np % 2) t = -t;
	return t;
}

// 计算逆矩阵
bool AMath::MatrixInvert(int n, double *a) {
	int n2 = n * n;
	double *y = new double[n2];
	double *yptr;
	int i;
	bool rslt(false);

	bzero(y, sizeof(double) * n2);
	for (i = 0, yptr = y; i < n; ++i, yptr += (n + 1)) *yptr = 1.0;
	if (LUdcmp(n, a)) {
		LUsolve(n, y, y);
		memcpy(a, y, sizeof(double) * n2);
		rslt = true;
	}
	delete []y;
	return rslt;
}

// 计算矩阵乘积
void AMath::MatrixMultiply(int m, int p, int n, double *L, double *R, double *Y) {
	int i, j, k;
	double sum;
	double *Lptr, *Rptr, *Yptr;
	double* Lt = new double[m * p];
	double* Rt = new double[p * n];

	memcpy(Lt, L, sizeof(double) * m * p);
	memcpy(Rt, R, sizeof(double) * p * n);

	for (j = 0, Yptr = Y; j < m; ++j) {
		for (i = 0; i < n; ++i, ++Yptr) {
			Lptr = Lt + j * p;
			Rptr = Rt + i;
			for (k = 0, sum = 0.0; k < p; ++k, ++Lptr, Rptr += n) {
				sum += *Lptr * *Rptr;
			}
			*Yptr = sum;
		}
	}

	delete []Lt;
	delete []Rt;
}

///////////////////////////////////////////////////////////////////////////////
/*------------------------------- 部分球坐标转换 -------------------------------*/
double AMath::SphereRange(double A1, double D1, double A2, double D2)
{
	double x = cos(D1) * cos(D2) * cos(A1 - A2) + sin(D1) * sin(D2);
	return acos(x);
}

void AMath::Sphere2Cart(double R, double A, double D, double& x, double& y, double& z)
{
	double cd = cos(D);
	x = R * cd * cos(A);
	y = R * cd * sin(A);
	z = R * sin(D);
}

void AMath::Cart2Sphere(double x, double y, double z, double& R, double& A, double& D)
{
	R = sqrt(x * x + y * y + z * z);
	if (fabs(y) < AU_EPS && fabs(x) < AU_EPS)
		A = 0;
	else if ((A = atan2(y, x)) < 0)
		A += AU_2PI;
	D = atan2(z, sqrt(x * x + y * y));
}

void AMath::PolarForward(double A0, double D0, double& A, double& D)
{
	double r = 1.0;
	double x1, y1, z1;	// 原坐标系投影位置
	double x2, y2, z2;	// 新坐标系投影位置
	double sd = sin(D0);
	double cd = cos(D0);
	double sa = sin(A0);
	double ca = cos(A0);
	// 在原坐标系的球坐标转换为直角坐标
	Sphere2Cart(r, A, D, x1, y1, z1);
	/*! 对直角坐标做旋转变换. 定义矢量V=(alpha0, beta0)
	 * 主动视角, 旋转矢量V
	 * 先绕Z轴逆时针旋转: -alpha0, 将矢量V旋转至XZ平面
	 * 再绕Y轴逆时针旋转: -(PI90 - beta0), 将矢量V旋转至与Z轴重合
	 **/
	 x2 = sd * ca * x1 + sd * sa * y1 - cd * z1;
	 y2 = -sa * x1 + ca * y1;
	 z2 = cd * ca * x1 + cd * sa * y1 + sd * z1;
	// 将旋转变换后的直角坐标转换为球坐标, 即以(alpha0, beta0)为极轴的新球坐标系中的位置
	Cart2Sphere(x2, y2, z2, r, A, D);
}

void AMath::PolarReverse(double A0, double D0, double& A, double& D)
{
	double r = 1.0;
	double x1, y1, z1;
	double x2, y2, z2;
	double sd = sin(D0);
	double cd = cos(D0);
	double sa = sin(A0);
	double ca = cos(A0);

	// 在新坐标系的球坐标转换为直角坐标
	Sphere2Cart(r, A, D, x1, y1, z1);
	/*! 对直角坐标做旋转变换.  定义矢量V=(alpha0, beta0)
	 * 主动旋转, 旋转矢量V
	 * 先绕Y轴逆时针旋转: PI90 - beta0
	 * 再绕Z轴逆时针旋转: alpha0
	 **/
	x2 = ca * sd * x1 - sa * y1 + ca * cd * z1;
	y2 = sa * sd * x1 + ca * y1 + sa * cd * z1;
	z2 = -cd * x1 + sd * z1;
	// 将旋转变换后的直角坐标转换为球坐标
	Cart2Sphere(x2, y2, z2, r, A, D);
}

// 为批量投影准备投影点的三角变换
void AMath::PrepareProject(double A0, double D0) {
	SD0_ = sin(D0);
	CD0_ = cos(D0);
	A0_  = A0;
}

// 球坐标投影到平面坐标
void AMath::Sphere2Plane(double A, double D, double &xi, double &eta)
{
	double fract = SD0_ * sin(D) + CD0_ * cos(D) * cos(A - A0_);
	xi  = cos(D) * sin(A - A0_) / fract;
	eta = (CD0_ * sin(D) - SD0_ * cos(D) * cos(A - A0_)) / fract;
}

// 被投影的平面坐标转换到球坐标
void AMath::Plane2Sphere(double xi, double eta, double &A, double &D)
{
	double fract = CD0_ - eta * SD0_;
	A = cycmod(A0_ + atan2(xi, fract), AU_2PI);
	D = atan(((eta * CD0_ + SD0_) * cos(A - A0_)) / fract);
}
/*------------------------------- 部分球坐标转换 -------------------------------*/
///////////////////////////////////////////////////////////////////////////////
/*----------------------------------- 内插 -----------------------------------*/
void AMath::spline(int n, double x[], double y[], double c1, double cn, double c[])
{
	int i;
	double p, qn, sig, un;
	double* u = (double*) calloc(n, sizeof(double));
	double limit = 0.99 * AU_MAX;
	if (c1 > limit) {
		c[0] = 0;
		u[0] = 0;
	}
	else {
		c[0] = -0.5;
		u[0] = (3 * (y[1] - y[0]) / (y[1] - y[0] - c1)) / (x[1] - x[0]);
	}
	for (i = 1; i < n - 1; ++i) {
		sig = (x[i] - x[i - 1]) / (x[i + 1] - x[i - 1]);
		p = sig * c[i - 1] + 2;
		c[i] = (sig - 1) / p;
		u[i]=(6.*((y[i+1]-y[i])/(x[i+1]-x[i])-(y[i]-y[i-1])/(x[i]-x[i-1]))/(x[i+1]-x[i-1])-sig*u[i-1])/p;
	}
	if (cn > limit) {
		qn = 0;
		un = 0;
	}
	else {
		qn = 0.5;
		un = 3./(x[n-1]-x[n-2])*(cn-(y[n-1]-y[n-2])/(x[n-1]-x[n-2]));
	}
	c[n-1]=(un-qn*u[n-2])/(qn*c[n-2]+1.);
	for (i = n - 2; i >= 0; --i) {
		c[i] = c[i] * c[i + 1] + u[i];
	}
	free(u);
}

bool AMath::splint(int n, double x[], double y[], double c[], double xo, double& yo)
{
	int k, khi, klo;
	double a, b, h;

	klo = 0;
	khi = n - 1;
	while ((khi - klo) > 1) {
		k = (khi + klo) / 2;
		if (x[k] > xo) khi = k;
		else           klo = k;
	}
	h = x[khi] - x[klo];
	if (fabs(h) < AU_EPS)
		return false;
	a = (x[khi] - xo) / h;
	b = (xo - x[klo]) / h;
	yo = a * y[klo] + b * y[khi] + ((a * a - 1) * a * c[klo] + (b * b - 1) * b * c[khi]) * h * h / 6;

	return true;
}

void AMath::spline2(int nr, int nc, double x1[], double x2[], double y[], double c[])
{
	int i, j, k;
	double* ytmp = (double*) calloc(nc, sizeof(double));
	double* ctmp= (double*) calloc(nc, sizeof(double));

	for (j = 0; j < nr; ++j) {
		for (k = 0, i = j * nc; k < nc; ++k, ++i) ytmp[k] = y[i];
		spline(nc, x2, ytmp, AU_MAX, AU_MAX, ctmp);
		for (k = 0, i = j * nc; k < nc; ++k, ++i) c[i] = ctmp[k];
	}

	free(ytmp);
	free(ctmp);
}

bool AMath::splint2(int nr, int nc, double x1[], double x2[], double y[], double c[], double x1o, double x2o, double& yo)
{
	bool bret = true;
	int i, j, k;
	double* ctmp  = (double*) calloc(nr > nc ? nr : nc, sizeof(double));
	double* ytmp  = (double*) calloc(nc, sizeof(double));
	double* yytmp = (double*) calloc(nr, sizeof(double));

	for (j = 0; j < nr; ++j) {
		for (k = 0, i = j * nc; k < nc; ++k, ++i) {
			ytmp[k] = y[i];
			ctmp[k] = c[i];
		}
		if (!(bret = splint(nc, x2, ytmp, ctmp, x2o, yytmp[j]))) break;
	}
	if (bret) {
		spline(nr, x1, yytmp, AU_MAX, AU_MAX, ctmp);
		bret = splint(nr, x1, yytmp, ctmp, x1o, yo);
	}

	free(ctmp);
	free(ytmp);
	free(yytmp);

	return bret;
}

double AMath::Bilinear(double XI[], double YI[], double ZI[], double X0, double Y0)
{
	double f1 = ZI[0] * (XI[1] - X0) * (YI[1] - Y0);
	double f2 = ZI[1] * (X0 - XI[0]) * (YI[1] - Y0);
	double f3 = ZI[2] * (XI[1] - X0) * (Y0 - YI[0]);
	double f4 = ZI[3] * (X0 - XI[0]) * (Y0 - YI[0]);

	return (f1 + f2 + f3 + f4) / (XI[1] - XI[0]) / (YI[1] - YI[0]);
}

void AMath::Lagrange(int N, double XI[], double YI[], int OD, int M, double XO[], double YO[])
{
	if (OD < 2) OD = 2;	// OD＝2时等效线性内插
	if (OD > N) OD = N;	// OD＝N时等效所有已知数据参与内插
	int OH = OD / 2;
	int SI, EI;			// SI: 参与内插的起始位置; EI: 参与内插的结束位置
	int i, j, k;
	double t0, t1;
	double x;

	for (k = 0; k < M; ++k) {// 逐点进行内插
		x = XO[k];
		// 查找最邻近的大于x的起始位置
		for (j = 0; j < N; ++j) {
			if (XI[j] > x) break;
		}
		// 检查参与内插的位置
		SI = j - OH;
		EI = SI + OD - 1;
		if (SI < 0) {
			SI = 0;
			EI = OD - 1;
		}
		if (EI >= N) {
			EI = N - 1;
			SI = EI - OD + 1;
		}
		// 内插
		t0 = 0;
		for (i = SI; i <= EI; ++i) {
			t1 = 1;
			for (j = SI; j < i; ++j) t1 = t1 * (x - XI[j]) / (XI[i] - XI[j]);
			for (j = i + 1; j <= EI; ++j) t1 = t1 * (x - XI[j]) / (XI[i] - XI[j]);
			t0 = t0 + t1 * YI[i];
		}
		YO[k] = t0;
	}
}
/*----------------------------------- 内插 -----------------------------------*/
///////////////////////////////////////////////////////////////////////////////
double AMath::Correlation(int N, double X[], double Y[])
{
	double sumx = 0;
	double sumy = 0;
	double sqx = 0;
	double sqy = 0;
	double sumxy = 0;
	double meanx, meany, rmsx, rmsy;
	int i;

	for (i = 0; i < N; ++i) {
		sumx += X[i];
		sumy += Y[i];
		sumxy += X[i] * Y[i];
		sqx += X[i] * X[i];
		sqy += Y[i] * Y[i];
	}
	meanx = sumx / N;
	meany = sumy / N;
	rmsx = sqrt((sqx - sumx * meanx) / (N - 1));
	rmsy = sqrt((sqy - sumy * meany) / (N - 1));
	return (sumxy - N * meanx * meany) / rmsx / rmsy;
}
/*--------------------------------- 相关系数 ---------------------------------*/
//////////////////////////////////////////////////////////////////////////////
}
