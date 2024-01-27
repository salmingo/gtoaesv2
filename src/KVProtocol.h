/**
 * @file KVProtocol.h  键值对通信协议操作接口
 * @author 卢晓猛 (lxm@nao.cas.cn)
 * @brief
 * - 解析通信协议
 * - 封装通信协议, 转换为字符串
 * @version 0.1
 * @date 2023-10-12
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef KVPROTOCOL_H
#define KVPROTOCOL_H

#include "ProtoKV.h"

class KVProtocol
{
public:
    KVProtocol();
    ~KVProtocol();

// 接口
public:
    /**
     * @brief 解析字符串, 构建对应指针
     * @param rcvd  从网络上收到的字符串
     * @return KVBasePtr
     */
    KVBasePtr Resolve(const char* rcvd);

// 功能
private:
    /**
     * @brief 以=为分隔符, 解析关键字和键值
     * @param kv       keyword=value格式字符串
     * @param keyword  关键字
     * @param value    键值
     * @return keyword和value有效标志
     */
    bool resolve_kv(const string& kv, string& keyword, string& value);
    /**
     * @brief 解析键值对集合
     * @param str    键值对集合字符串, key1=val1,[key2=val2,...]
     * @param kvs    解析后键值对集合
     * @param basis  临时存储基础信息
     */
    void resolve_kv(const char* str, KVVec& kvs, KVBase& basis);

// 功能: 按协议类型创建对应实例指针
private:
    /**
     * @brief 追加观测计划
     */
    KVBasePtr resolve_append_plan(const KVVec& kvs);
    /**
     * @brief 追加观测计划: GWAC
     */
    KVBasePtr resolve_append_gwac(const KVVec& kvs);
    /**
     * @brief 检查观测计划状态
     */
    KVBasePtr resolve_check_plan(const KVVec& kvs);
    /**
     * @brief 删除观测计划
     */
    KVBasePtr resolve_remove_plan(const KVVec& kvs);
    /**
     * @brief 观测计划实时
     */
    KVBasePtr resolve_plan(const KVVec& kvs);
    /**
     * @brief 中断当前任务
     */
    KVBasePtr resolve_abort(const KVVec& kvs);

    /**
     * @brief 观测系统实时工作状态
     */
    KVBasePtr resolve_obss(const KVVec& kvs);
    /**
     * @brief 转台实时工作状态
     */
    KVBasePtr resolve_mount(const KVVec& kvs);
    /**
     * @brief 相机实时工作状态
     */
    KVBasePtr resolve_camera(const KVVec& kvs);

    /**
     * @brief 转台: 搜索零点
     */
    KVBasePtr resolve_home(const KVVec& kvs);
    /**
     * @brief 转台: 同步零点
     */
    KVBasePtr resolve_sync(const KVVec& kvs);
    /**
     * @brief 转台: 复位
     */
    KVBasePtr resolve_park(const KVVec& kvs);
    /**
     * @brief 转台: 指向
     */
    KVBasePtr resolve_slewto(const KVVec& kvs);
    /**
     * @brief 转台: 导星
     */
    KVBasePtr resolve_guide(const KVVec& kvs);

    /**
     * @brief 相机: 手动曝光
     */
    KVBasePtr resolve_take_image(const KVVec& kvs);
    /**
     * @brief 相机: 曝光指令
     */
    KVBasePtr resolve_expose(const KVVec& kvs);
    /**
     * @brief 相机参数: 状态/查询/修改
     */
    KVBasePtr resolve_camset(const KVVec& kvs);

    /**
     * @brief 调焦: 开环调焦
     */
    KVBasePtr resolve_focus(const KVVec& kvs);
    /**
     * @brief 调焦: 闭环调焦
     */
    KVBasePtr resolve_fwhm(const KVVec& kvs);

    /**
     * @brief 消旋器: 指令和状态
     */
    KVBasePtr resolve_derot(const KVVec& kvs);

    /**
     * @brief 圆顶和天窗: 指令和状态
     */
    KVBasePtr resolve_dome(const KVVec& kvs);

    /**
     * @brief 镜盖: 指令和状态
     */
    KVBasePtr resolve_mcover(const KVVec& kvs);

    /**
     * @brief 滤光片(单独控制): 指令和状态
     */
    KVBasePtr resolve_filter(const KVVec& kvs);

    /**
     * @brief 测站位置: 状态/查询/修改
     */
    KVBasePtr resolve_geosite(const KVVec& kvs);
};

#endif
