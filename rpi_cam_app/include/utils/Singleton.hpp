#ifndef _SINGLETON_H
#define _SINGLETON_H

#include <memory>
#include <mutex>

namespace utils
{
    
    template<typename T>
    class Singleton
    {
    public:
        static T* get_instance()
        {
            std::call_once(initFlag, &Singleton::initSingleton);
            return instance.get();
        }
        Singleton(const Singleton&) = delete;
        Singleton& operator=(const Singleton&)=delete;

    protected:
        Singleton() = default;
        virtual ~Singleton() = default;
    private:
        static std::unique_ptr<T> instance;
        static std::once_flag initFlag;

        static void initSingleton()
        {
            instance.reset(new T());
        }
    };

    template <typename T>
    std::unique_ptr<T> Singleton<T>::instance = nullptr;
    template <typename T>
    std::once_flag Singleton<T>::initFlag;

} // namespace utils

#endif