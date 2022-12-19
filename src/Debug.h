#pragma once

#include <cstdio>

#include "Loop.h"
#include "Config.h"

#if DEBUG
#define LOGD(fmt, ...) printf("%+-10ld | " fmt, Loop::log(), ##__VA_ARGS__)
#else
#define LOGD(fmt, ...) do { } while (false)
#endif
