#pragma once

#define _DC_NS_EXPANDED(pre, post) pre##_##post
#define DC_EXPAND(...) __VA_ARGS__
#define DC_STRINGIFY(MACRO) #MACRO
#define DC_EXPAND_STRING(NAME) DC_STRINGIFY(NAME)

// JUSTIFY: Not namespaced under `DC`
//  - `NS` is everywhere - ugly if expanded to DC_NS, and not a name likely to
//    conflict with others
//  - Same rationale for PRIV
#define NS(pre, post) _DC_NS_EXPANDED(pre, post)
#define PRIV(name) NS(__private, name)
