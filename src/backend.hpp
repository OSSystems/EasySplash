#ifndef _BACKEND_HPP
#define _BACKEND_HPP

class Backend
{
public:
    virtual int getScreenWidth() = 0;
    virtual int getScreenHeight() = 0;
    virtual int getDepth() = 0;
};

#endif
