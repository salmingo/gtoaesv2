/**
 * @file Boost.h boost类库的常用引用和定义文件
 * @version 1.0
 * @date 2023-11-03
 */
#ifdef _WINDOWS
#include "StdAfx.h"
#endif

#include "BoostInclude.h"

// 中断线程
void interrupt_thread(boost::thread &thrd) {
	if (thrd.joinable()) {
		thrd.interrupt();
		thrd.join();
	}
}

void interrupt_thread(ThrdPtr& thrd) {
	if (thrd.unique() && thrd->joinable()) {
		thrd->interrupt();
		thrd->join();
		thrd.reset();
	}
}

#ifdef _WINDOWS
ArrayCharW Char2Wide(const char* str, const int n) {
	ArrayCharW wstr = make_shared_array<wchar_t>(n + 1);
	MultiByteToWideChar(CP_ACP, 0, str, n, wstr.get(), n);
	wstr[n] = 0;
	return wstr;
}

ArrayChar Wide2Char(const wchar_t* wstr, const int n) {
	ArrayChar str = make_shared_array<char>(n + 1);
	WideCharToMultiByte(CP_ACP, 0, wstr, n, str.get(), n, 0, 0);
	str[n] = 0;
	return str;
}

#endif
