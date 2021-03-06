/*==============================================================================

 Copyright 2018 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

#pragma once

/*
 * This is not finished / implemented
 */
class SystemSemaphore
{
public:
    SystemSemaphore (const String& name);
    ~SystemSemaphore();

    bool lock();
    bool unlock();

    bool isValid();

private:
    class Impl;
    ScopedPointer<Impl> impl;
};
