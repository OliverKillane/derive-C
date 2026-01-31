/// @brief A single include for derive-c macros & utility functions
// Panic (always first)
#include <derive-c/core/panic.h> // IWYU pragma: export

// Helpful
#include <derive-c/core/attributes.h>   // IWYU pragma: export
#include <derive-c/core/derive.h>       // IWYU pragma: export
#include <derive-c/core/math.h>         // IWYU pragma: export
#include <derive-c/core/namespace.h>    // IWYU pragma: export
#include <derive-c/core/placeholder.h>  // IWYU pragma: export
#include <derive-c/core/require.h>      // IWYU pragma: export
#include <derive-c/core/scope.h>        // IWYU pragma: export
#include <derive-c/core/zerosized.h>    // IWYU pragma: export
#include <derive-c/core/unit.h>         // IWYU pragma: export
#include <derive-c/core/cpu_features.h> // IWYU pragma: export

// Traits
#include <derive-c/core/traits/clone.h>  // IWYU pragma: export
#include <derive-c/core/traits/debug.h>  // IWYU pragma: export
#include <derive-c/core/traits/delete.h> // IWYU pragma: export
#include <derive-c/core/traits/eq.h>     // IWYU pragma: export
#include <derive-c/core/traits/iter.h>   // IWYU pragma: export
#include <derive-c/core/traits/ord.h>    // IWYU pragma: export
#include <derive-c/core/traits/ref.h>    // IWYU pragma: export

// Debug formatting
#include <derive-c/core/debug/fmt.h> // IWYU pragma: export
