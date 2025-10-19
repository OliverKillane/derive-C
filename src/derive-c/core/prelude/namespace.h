#pragma once

#define NS_EXPANDED(pre, post) pre##_##post
#define NS(pre, post) NS_EXPANDED(pre, post)
#define EXPAND(...) __VA_ARGS__
#define PRIV(name) NS(__private, name)
