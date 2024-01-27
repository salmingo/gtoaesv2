/**
 * @file BoostInclude.h boost类库的常用引用和定义文件
 * @version 1.0
 * @date 2023-11-03
 * - 声明常用boost智能指针和智能数组类型
 */

#ifndef BOOSTINCLUDE_H_
#define BOOSTINCLUDE_H_

#include <string.h>
// 指针和指针型数组
#include <boost/smart_ptr/shared_array.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

// 容器
#include <boost/circular_buffer.hpp>

// 线程
#include <boost/thread/thread.hpp>

// 数据类型
typedef boost::shared_array<char> ArrayChar;
typedef boost::shared_array<wchar_t> ArrayCharW;
typedef boost::shared_array<unsigned char> ArrayCharU;
typedef boost::shared_array<short> ArrayShort;
typedef boost::shared_array<unsigned short> ArrayShortU;
typedef boost::shared_array<int> ArrayInt;
typedef boost::shared_array<float> ArrayFloat;
typedef boost::shared_array<double> ArrayDouble;
typedef boost::circular_buffer<char> ArrayCharCrc;

typedef boost::thread Thread;
typedef boost::shared_ptr<Thread> ThrdPtr;
typedef boost::unique_lock<boost::mutex> MtxLck;

// 中断线程
extern void interrupt_thread(Thread &thrd);
extern void interrupt_thread(ThrdPtr &thrd);

template <typename T>
boost::shared_array<T> make_shared_array(int n)
{
	boost::shared_array<T> ptr;
	ptr.reset(new T[n]);
	memset(ptr.get(), 0, n * sizeof(T));
	return ptr;
}

// 适用于MFC的字符集转换
#ifdef _WINDOWS
extern ArrayCharW Char2Wide(const char* str, const int n);
extern ArrayChar Wide2Char(const wchar_t* wstr, const int n);
#endif

#endif
