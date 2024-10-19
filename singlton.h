/******************************************************************************
 *
 * @file       singlton.h
 * @brief      单例模板
 *
 * @author     Suycx
 * @date       2024/10/18
 * @history
 *****************************************************************************/
#ifndef SINGLTON_H
#define SINGLTON_H

#include "global.h"

template< typename T >
class Singlton{
public:
    static std::shared_ptr<T> GetInstance() {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            _instance = std::shared_ptr<T>(new T);
        });
        return _instance;
    }

    ~Singlton() {
        std::cout << "--------- ~Singlton() ---------" << std::endl;
    }

    void PrintAddress() {
        std::cout << _instance.get() << std::endl;
    }

protected:
    Singlton() = default;
    Singlton(const Singlton<T>&) = delete;
    Singlton<T>& operator=(const Singlton<T>&) = delete;

    static std::shared_ptr<T> _instance;
};

// 静态成员变量初始化为空指针
template < typename T >
std::shared_ptr<T> Singlton<T>::_instance = nullptr;

#endif // SINGLTON_H
