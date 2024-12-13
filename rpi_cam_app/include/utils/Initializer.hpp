#ifndef _INITIALIZER_HPP
#define _INITIALIZER_HPP

namespace utils
{
    class Initialzier
    {
    public:
        virtual int init() = 0;
        virtual int start() = 0;
        virtual int stop() = 0;
    };
};

#endif