#pragma once
#include <string>

enum class SSLVersion
{
    TLS_1_0,
    TLS_1_1,
    TLS_1_2,
    TLS_1_3
};

enum class SSLError
{
    NONE,
    WANT_READ,
    WANT_WRITE,
    SYSCALL,
    SSL,
    UNKNOW
};

enum class SSLState
{
    HANDSHAKE,
    ESTABLISHED,
    SHUTDOWN,
    ERROR
};