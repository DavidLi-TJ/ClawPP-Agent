#ifndef CLAWPP_COMMON_RESULT_H
#define CLAWPP_COMMON_RESULT_H

#include <QString>
#include <utility>
#include <type_traits>

/**
 * @namespace clawpp
 * @brief Claw++ 项目命名空间
 */
namespace clawpp {

/**
 * @class Result
 * @brief 结果类型模板类
 * 用于表示操作的成功或失败结果，类似于 Rust 的 Result<T, E>
 * @tparam T 成功时返回的类型
 */
template<typename T>
class Result {
    bool m_ok;                                      ///< 是否成功
    typename std::aligned_storage<sizeof(T), alignof(T)>::type m_storage;  ///< 值存储
    QString m_error;                                ///< 错误信息
    
public:
    ~Result();
    
    Result(const Result& other);
    
    Result(Result&& other) noexcept;
    
    Result& operator=(const Result& other);
    
    /**
     * @brief 创建成功结果
     * @param value 成功时的值
     * @return 成功结果对象
     */
    static Result ok(T value);
    
    /**
     * @brief 创建失败结果
     * @param error 错误信息
     * @return 失败结果对象
     */
    static Result err(QString error);
    
    bool isOk() const;      ///< 检查是否成功
    bool isErr() const;     ///< 检查是否失败
    
    T& value();             ///< 获取值（非常量）
    const T& value() const; ///< 获取值（常量）
    
    const QString& error() const;  ///< 获取错误信息
    
    /**
     * @brief 获取值或返回默认值
     * @param defaultValue 默认值
     * @return 成功时返回值，失败时返回默认值
     */
    T valueOr(T defaultValue) const;

private:
    Result() = default;
};

/**
 * @class Result<void>
 * @brief Result 的 void 特化版本
 * 用于不返回值的操作
 */
template<>
class Result<void> {
    bool m_ok;
    QString m_error;
    
public:
    static Result ok();
    static Result err(QString error);
    
    bool isOk() const;
    bool isErr() const;
    const QString& error() const;

private:
    Result() = default;
};

}

#endif
