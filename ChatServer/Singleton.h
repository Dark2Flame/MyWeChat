#pragma once
#include <iostream>
#include <mutex>
#include <memory>

template<typename T>
class Singleton
{
private:
    Singleton(const Singleton<T> &singleT) = default;
    Singleton &operator=(const Singleton<T> &singleT) = delete;
    static std::shared_ptr<T> _instance;
    //static std::_mutex s_mutex;
protected:
    Singleton() = default; // 方便子类调用
public:
    ~Singleton()
    {
        std::cout << "this is auto template destruct" << std::endl;
    }

    static std::shared_ptr<T> GetInst()
    {
        static std::once_flag s_flag; // once_flag只会初始化一次
        std::call_once(s_flag,[&](){ // lambda 只会在s_flag未初始化的时候被调；‘，。/用
            //_instance = std::shared_ptr<T>(new T);
            _instance = std::shared_ptr<T>(new T);
        });
        return _instance;
    }

    void PrintAdress()
    {
        std::cout << _instance.get() << std::endl;
    }
};

// 将实例指针初始化为空表示未创建实例
template<typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

