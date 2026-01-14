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

// JUSTIFY: private by namespacing
//  - There is no easy way to mark private in C
//  - This makes it obvious, and furthermore prefix keeps it at the bottom of intellisense
//  suggestions
#define PRIV(name) NS(__private, name)

// JUSTIFY: Marking public functions
//  - When a template is expanded, not all of the methods are used by the user
//  - Use of static means unused warnings occur
#define PUBLIC [[maybe_unused]]

// JUSTIFY: Marking public functions
//  - When a template is expanded, not all of the methods are used by the user
//  - Use of static means unused warnings occur
#define INTERNAL [[maybe_unused]]
