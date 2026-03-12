---
name: cpp23
description: "Write modern, idiomatic C++23 code with best practices and modules. Use this skill for any C++ task: new files, refactoring, algorithms, data structures, systems programming, performance-critical code, or library design. Triggers on mentions of C++, .cpp, .ixx, .hpp, or any request to write, fix, or review C++ code. Produces clean, safe, expressive code using C++23 named modules (import std;) and project modules instead of #include headers."
---

This skill governs the generation of modern C++23 code using **modules**. All output must be idiomatic, safe, and expressive — no legacy `#include` patterns, no unnecessary boilerplate, no C-style code unless explicitly requested. Default to `import std;` for the standard library and named module units for all project code.

---

## Core Philosophy

Write code that is:
- **Correct first** — well-defined behavior, no UB
- **Expressive** — code reads like intent, not implementation
- **Safe by default** — ownership is clear, lifetimes are tracked
- **Zero-cost where possible** — abstraction without overhead
- **Modern** — prefer C++23 features, modules over headers, fall back to C++20 if compiler support is uncertain

Before writing any code, ask:
- What owns this data?
- What are the lifetimes here?
- Can this be `constexpr`?
- Can this be expressed as a range pipeline?
- Is there a standard algorithm or view for this?
- Should this be its own module unit or partition?

---

## Language Features: Use These

### Types & Variables

```cpp
// Always use auto where type is obvious or verbose
auto result = std::vector{};
auto it     = map.find(key);

// Use structured bindings everywhere
auto [x, y, z] = get_position();
for (auto const& [key, val] : registry) { ... }

// Prefer std::optional over sentinel values
std::optional load_config(std::string_view path);

// Use std::expected (C++23) for error propagation
std::expected parse(std::span buf);

// Never use raw new/delete; prefer smart pointers or value types
auto obj   = std::make_unique(args...);
auto shared = std::make_shared();
```

### Functions & Callables

```cpp
// Explicit [[nodiscard]] on anything whose return value matters
[[nodiscard]] auto compute() -> Result;

// Use std::string_view for read-only string params (not const std::string&)
void log(std::string_view msg);

// Trailing return types for clarity with complex types
auto transform(std::ranges::range auto&& r) -> std::vector;

// Lambdas: capture minimally, use explicit types when helpful
auto pred = [threshold](int v) noexcept { return v > threshold; };

// Use deducing this (C++23) to eliminate CRTP boilerplate
struct Widget {
    auto name(this auto&& self) -> decltype(auto) { return self.name_; }
};
```

### Classes & Object Model

```cpp
// Rule of Zero: let the compiler generate everything when possible
struct Point {
    double x{}, y{};
    // No user-declared ctor/dtor/copy/move needed
};

// When you must define special members, define all (Rule of Five)
// Always = default or = delete before writing custom implementations

// Prefer aggregate initialization
auto p = Point{.x = 1.0, .y = 2.0};  // designated initializers

// Mark everything that can be const, constexpr, or noexcept
constexpr auto square(double v) noexcept -> double { return v * v; }

// Use explicit on single-arg constructors (almost always)
struct Radius { explicit Radius(double v) : value{v} {} double value; };
```

### Ownership & Memory

```cpp
// Ownership hierarchy (prefer higher on the list)
// 1. Value semantics (stack / member)
// 2. std::unique_ptr    (sole owner)
// 3. std::shared_ptr    (shared ownership, use sparingly)
// 4. Raw pointer / ref  (non-owning observer ONLY)

// Use std::span for non-owning views of contiguous data
void process(std::span data);

// Use std::string_view for non-owning string views
// Never store string_view as a member (dangling risk)

// Prefer references over raw pointers for non-nullable observers
void render(Scene const& scene, Renderer& renderer);
```

### Ranges & Algorithms

```cpp
// No #include needed — import std; covers everything

// Compose range pipelines instead of raw loops
auto results = data
    | std::views::filter([](auto const& x) { return x.active; })
    | std::views::transform(&Record::value)
    | std::views::take(10);

// Use std::ranges:: algorithms (they accept ranges, not iterator pairs)
std::ranges::sort(vec);
std::ranges::copy(src, std::back_inserter(dst));
auto it = std::ranges::find_if(items, pred);

// C++23: std::views::zip, std::views::enumerate, std::views::chunk
for (auto [i, v] : std::views::enumerate(vec)) { ... }
for (auto [a, b] : std::views::zip(xs, ys)) { ... }
```

### Concurrency

```cpp
// Prefer std::jthread over std::thread (auto-joins, supports stop tokens)
std::jthread worker{[](std::stop_token st) {
    while (!st.stop_requested()) { /* work */ }
}};

// Use std::atomic for shared state; avoid locks where possible
std::atomic counter{0};
counter.fetch_add(1, std::memory_order_relaxed);

// Prefer std::mutex + std::lock_guard / std::scoped_lock
std::mutex mtx;
{
    std::scoped_lock lock{mtx};
    shared_data.push_back(item);
}

// Use std::future / std::async for simple async tasks
auto fut = std::async(std::launch::async, compute_heavy, input);
auto val = fut.get();
```

### Error Handling

```cpp
// C++23: std::expected for recoverable errors (prefer over exceptions for perf-sensitive paths)
auto open_file(std::string_view path) -> std::expected;

// Use exceptions for truly exceptional conditions
// Define domain-specific exception types inheriting std::exception
struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// NEVER use error codes as return values without [[nodiscard]]
// NEVER use errno directly; wrap in std::system_error where needed

// Monadic operations on std::expected (C++23)
auto result = open_file(path)
    .and_then(parse_contents)
    .transform(normalize)
    .or_else([](auto e) -> std::expected {
        log_error(e);
        return std::unexpected{e};
    });
```

### Compile-Time Programming

```cpp
// Make everything constexpr that can be
constexpr auto factorial(unsigned n) -> unsigned long long {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

// Use if constexpr for compile-time branching in templates
template
auto serialize(T const& v) {
    if constexpr (std::is_arithmetic_v) {
        return std::to_string(v);
    } else {
        return v.to_string();
    }
}

// Concepts over SFINAE — always
template
auto sum(R&& r) { return std::ranges::fold_left(r, 0, std::plus{}); }

// Use std::is_constant_evaluated() / if consteval (C++23) when needed
constexpr auto fast_or_safe(int x) {
    if consteval { return safe_impl(x); }
    else         { return fast_impl(x); }
}
```

### Formatting & I/O

```cpp
// Use std::format (C++20) / std::print (C++23) — never printf
// No #include needed with import std;

auto msg = std::format("Hello, {}! You have {} messages.", name, count);
std::print("Value: {:.2f}\n", pi);
std::println(stderr, "Error: {}", err.message());
```

---

## Patterns to Avoid

| ❌ Avoid | ✅ Prefer |
|---|---|
| `#include <vector>` etc. | `import std;` |
| `.hpp` / `.h` header files | `.ixx` module units |
| `#pragma once` / include guards | Named module interface units |
| `new` / `delete` | `make_unique`, value types |
| `NULL` / `0` as pointer | `nullptr` |
| C-style casts `(T)x` | `static_cast<T>`, `std::bit_cast<T>` |
| `#define` constants | `constexpr` variables |
| `#include <string.h>` | `import std;` |
| Raw arrays `int arr[N]` | `std::array<int, N>` |
| `std::endl` | `'\n'` (avoid flush overhead) |
| `typedef` | `using` |
| `void*` | Templates or `std::any` / `std::variant` |
| `printf` / `scanf` | `std::print` / `std::format` |
| Iterator pairs in algorithms | `std::ranges::` algorithms |
| `volatile` for concurrency | `std::atomic` |
| `throw` in destructors | Mark destructors `noexcept` |
| Returning `std::pair<bool, T>` | `std::optional<T>` or `std::expected<T, E>` |

---

## Module Conventions

C++23 modules replace `#include` entirely. Always use modules for new code.

### Standard Library: `import std;`

```cpp
// One line replaces ALL standard library headers
import std;

// That's it. std::vector, std::ranges, std::expected, std::print — all available.
// Never write #include , #include , etc. in module code.
```

### Module Unit Structure

Every module has an **interface unit** (`.ixx`) and optionally one or more **implementation units** (`.cpp`).

```
mylib/
  mylib.ixx          ← primary module interface
  mylib-core.ixx     ← module partition: mylib:core
  mylib-core.cpp      ← partition implementation (optional split)
  main.cpp            ← consumer (not a module)
```

### Primary Module Interface Unit

```cpp
// mylib/mylib.ixx
export module mylib;          // declares the module

import std;                   // import standard library (not exported to consumers)

export import :core;          // re-export partition mylib:core
export import :io;            // re-export partition mylib:io

// Optionally export individual declarations here too
export namespace mylib {
    // anything declared here is visible to importers
}
```

### Module Partition

```cpp
// mylib/mylib-core.ixx
export module mylib:core;     // partition of module "mylib"

import std;

export namespace mylib {

/// Brief one-line description.
class MyClass {
public:
    explicit MyClass(std::string name);

    [[nodiscard]] auto name() const noexcept -> std::string_view;
    [[nodiscard]] auto process(int x) -> std::expected;

private:
    std::string name_;
};

} // namespace mylib
```

### Module Implementation Unit

```cpp
// mylib/mylib-core.cpp
module mylib:core;            // implements the partition (no "export")

import std;

namespace mylib {

MyClass::MyClass(std::string name)
    : name_{std::move(name)}
{}

auto MyClass::name() const noexcept -> std::string_view {
    return name_;
}

auto MyClass::process(int x) -> std::expected {
    if (x < 0) return std::unexpected{std::errc::invalid_argument};
    return x * 2;
}

} // namespace mylib
```

### Consumer (Application Code)

```cpp
// src/main.cpp  — NOT a module unit (no "export module" or "module" declaration)
import std;
import mylib;

int main() {
    auto obj = mylib::MyClass{"hello"};
    auto result = obj.process(42);
    if (result) std::println("Result: {}", *result);
    else        std::println(stderr, "Error: {}", std::make_error_code(result.error()).message());
}
```

### Module Rules & Gotchas

```
✅ DO:
  - export module name;          in exactly ONE .ixx per module
  - export module name:part;     for each partition
  - module name:part;            in implementation units (no export keyword)
  - import std;                  at top of every module/consumer that needs stdlib
  - export { ... }               to batch-export a group of declarations
  - export import :partition;    to re-export partitions from the primary interface

❌ DON'T:
  - Mix #include and import in the same translation unit (use one or the other)
  - Put export module in an implementation unit (.cpp)
  - Export using-directives (export using namespace std; is illegal)
  - Rely on macros crossing module boundaries (macros don't export)
  - Use global module fragment unless wrapping legacy headers:
      module;
      #include <legacy_c_lib.h>   // only for non-modularized dependencies
      export module mylib;
```

### Wrapping Legacy / Third-Party Headers

When a dependency doesn't provide modules, use the **global module fragment**:

```cpp
// mylib/compat.ixx
module;                        // global module fragment begins here
#include    // legacy header, not modularized
#include <third_party/lib.h>
export module mylib:compat;   // named module begins here

import std;

// Now wrap / re-export what consumers need
export namespace mylib::compat {
    using ::legacy_type;
    using ::legacy_function;
}
```

---

## Naming Conventions

```cpp
// snake_case for variables, functions, namespaces, files
int item_count = 0;
auto compute_hash(std::string_view s) -> std::size_t;
namespace my_lib { ... }

// PascalCase for types (classes, structs, concepts, enums)
struct HttpRequest { ... };
concept Serializable = requires(T t) { t.serialize(); };
enum class Color { Red, Green, Blue };

// UPPER_SNAKE_CASE only for macros (avoid macros entirely when possible)
#define MY_LIB_VERSION_MAJOR 2

// Prefix interfaces/abstract base classes with 'I' only if your team does (pick one style)
// Prefer concepts over abstract base classes
```

---

## File Structure Template

See the **Module Conventions** section above for canonical `.ixx` / `.cpp` templates. File extension conventions:

| File | Purpose |
|---|---|
| `name.ixx` | Module interface unit (primary or partition) — use `.ixx` on MSVC |
| `name.cpp` | Module implementation unit or non-module consumer (e.g. `main.cpp`) |
| `name.hpp` | **Only** for legacy/third-party wrapping in global module fragment |

---

## CMake Integration (C++23 Modules)

Modules require CMake 3.28+ with `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` or a compiler that ships `std` module support natively. The example below targets **Clang 17+ / GCC 14+ / MSVC 19.36+**.

```cmake
cmake_minimum_required(VERSION 3.28)
project(myproject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable import std; support (CMake 3.30+ stabilized; 3.28/3.29 need the flag)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
set(CMAKE_CXX_MODULE_STD ON)

# --- Library module ---
add_library(mylib)
target_sources(mylib
    PUBLIC
        FILE_SET CXX_MODULES FILES
            mylib/mylib.ixx
            mylib/mylib-core.ixx
    PRIVATE
        mylib/mylib-core.cpp
)
target_compile_features(mylib PUBLIC cxx_std_23)

# --- Executable ---
add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE mylib)

# Warnings & errors
target_compile_options(app PRIVATE
    $<$:-Wall -Wextra -Wpedantic -Werror>
    $<$:/W4 /WX /EHsc>
)

# Sanitizers in Debug
target_compile_options(app PRIVATE
    $<$<AND:$,$<NOT:$>>:-fsanitize=address,undefined>
)
target_link_options(app PRIVATE
    $<$<AND:$,$<NOT:$>>:-fsanitize=address,undefined>
)
```

> **Note:** `FILE_SET CXX_MODULES` is the canonical CMake 3.28+ way to declare module sources. It tells the build system to scan for dependencies between module units before compiling — do not use `target_sources` with plain `.ixx` files without `FILE_SET`.

---

## Quick Reference: C++20/23 Features

| Feature | Module / Keyword | Notes |
|---|---|---|
| Concepts | `import std;` | Replace all SFINAE |
| Ranges | `import std;` | Views are lazy; prefer pipelines |
| `std::span` | `import std;` | Non-owning contiguous view |
| `std::format` | `import std;` | Type-safe printf replacement |
| `std::print` | `import std;` | C++23; formatted output |
| `std::expected` | `import std;` | C++23; error monad |
| `std::jthread` | `import std;` | Auto-joining thread |
| `std::atomic_ref` | `import std;` | Atomic ops on existing objects |
| Coroutines | `import std;` | Low-level; use a library (cppcoro) |
| Named modules | `export module x;` | Primary interface: one per module |
| Module partitions | `export module x:p;` | Split large modules cleanly |
| `import std;` | C++23 standard | Replaces all stdlib `#include` |
| `std::mdspan` | `import std;` | C++23; multidimensional span |
| `std::flat_map` | `import std;` | C++23; sorted-vector backed map |
| `std::generator` | `import std;` | C++23; range coroutine |
| Deducing `this` | `this auto&&` | C++23; eliminates CRTP |
| `if consteval` | keyword | C++23; replaces `is_constant_evaluated` |