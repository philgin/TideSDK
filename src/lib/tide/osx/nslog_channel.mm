/**
 * Copyright (c) 2012 - 2014 TideSDK contributors 
 * http://www.tidesdk.org
 * Includes modified sources under the Apache 2 License
 * Copyright (c) 2008 - 2012 Appcelerator Inc
 * Refer to LICENSE for details of distribution and use.
 **/

#include "../tide.h"

namespace tide
{
    NSLogChannel::NSLogChannel()
        : formatter("[%s] [%p] %t")
    {
    }

    void NSLogChannel::log(const Poco::Message& msg)
    {
        std::string text;
        formatter.format(msg, text);
        NSLog(@"%s", text.c_str());
    }
}
