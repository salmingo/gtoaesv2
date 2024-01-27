/**
 * @file ProtoKVBase.h  声明基于"操作符-键值对"的ASCII编码通信协议的基类
 * @author 卢晓猛 (lxm@nao.cas.cn)
 * @copyright ATRD Group, NAOC (c) 2023
 * @note
 * - 通信协议采用编码字符串格式, 字符串以换行符结束
 * - 协议由三部分组成, 其格式为:
 *   type [keyword=value,]+<term>
 *   type   : 协议类型
 *   keyword: 单项关键字
 *   value  : 单项值
 *   term   : 换行符
 * - 类型与键值对之间分隔符采用空格
 * - 键值对之间分隔符采用逗号
 * - 键与值之间分隔符采用等号
 * - 协议结束符采用换行符
 *
 * @date 2017年2月16日
 * @version 0.1
 * @brief
 * -- GWAC系统
 *
 * @version 0.2
 * @date 2023年10月8日
 *
 *
 */
#ifndef PROTO_KV_BASE_H
#define PROTO_KV_BASE_H

#include <string>
#include <vector>
#include <sstream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using std::string;

template <class T> string join_kv(const string& key, const T& val) {
    std::stringstream ss;
    ss << key << "=" << val << ",";
    return ss.str();
}

/**
 * @brief 定义键-值对
 *
 */
struct KeyValPair {
    string keyword; ///< 关键字
    string value;   ///< 数值

public:
    KeyValPair() = default;
    KeyValPair(const string& key, const string& val) {
        keyword = key;
        value   = val;
    }
};
typedef std::vector<KeyValPair> KVVec;

/**
 * @brief 定义键值对协议公共父类
 */
struct KVBase
{
    // 成员变量
    string  type;   ///< 指令类型
    string  utc;    ///< 时间戳. 格式: YYYY-MM-DDThh:mm:ss
    string  gid;    ///< 组标志
    string  uid;    ///< 单元标志
    string  cid;    ///< 相机标志

public:
    KVBase() = default;
	virtual ~KVBase() = default;
    KVBase& operator=(const KVBase& other) {
        if (this != &other) {
            utc  = other.utc;
            gid  = other.gid;
            uid  = other.uid;
            cid  = other.cid;
        }
        return *this;
    }

	void UpdateUTC() {
		namespace POSIX_TIME = boost::posix_time;
		utc = POSIX_TIME::to_iso_extended_string(POSIX_TIME::second_clock::universal_time());
	}

    virtual string ToString() const {
        std::stringstream ss;
        ss << type << " ";
        if (utc.size()) ss << join_kv("utc", utc);
        if (gid.size()) ss << join_kv("gid", gid);
        if (uid.size()) ss << join_kv("uid", uid);
        if (cid.size()) ss << join_kv("cid", cid);
        return ss.str();
    }
};
typedef boost::shared_ptr<KVBase> KVBasePtr;
//////////////////////////////////////////////////////////////////////////////
/*!
 * @brief 将KVProtoBase继承类的boost::shared_ptr型指针转换为KVBasePtr类型
 * @param proto 协议指针
 * @return
 * KVBasePtr类型指针
 */
template <class T> KVBasePtr to_kvbase(T proto) {
	return boost::static_pointer_cast<KVBase>(proto);
}

/*!
 * @brief 将KVBasePtr类型指针转换为其继承类的boost::shared_ptr型指针
 * @param proto 协议指针
 * @return
 * KVBasePtr继承类指针
 */
template <class T> boost::shared_ptr<T> from_kvbase(KVBasePtr proto) {
	return boost::static_pointer_cast<T>(proto);
}

#endif
