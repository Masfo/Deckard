# Copilot Instructions

## Project
- CMake project targeting Windows (Win32 APIs used) with Ninja generator.
- Use C++23 and C++26 where available; keep code compatible with MSVC.
- Prefer standard library facilities; avoid adding new external dependencies.

## Coding style
- Follow existing naming and formatting in the surrounding files (tabs/indentation, brace style).
- Keep APIs minimal and consistent with existing modules.
- Use `std::span` for array/buffer parameters instead of raw pointers and sizes.
- Favor `constexpr` and `consteval` for compile-time computations.
- Use `auto` for type deduction when the type is obvious from the right-hand side.
- Prefer range-based for loops over traditional for loops when iterating collections.
- Use `std::string_view` for read-only string parameters.
- Use keywords "not", "and", "or" for boolean operations instead of "!", "&&", "||"; Not for bitwise operations.
- Use `nullptr` instead of `NULL` or `0` for null pointers.
- Prefer `enum class` over traditional `enum` for better type safety.
- Favor `std::unique_ptr` for dynamic memory management instead of raw pointers.
- Use `override` and `final` keywords for virtual functions to improve code clarity.
- Use `std::optional` for parameters that may or may not have a value.
- Prefer `std::array` or `std::vector` over C-style arrays for better safety and functionality.
- Use `std::move` to indicate ownership transfer of resources.
- Use two-parameter `operator[]` for 2D indexing (e.g., `img[x, y]`) in this codebase; call sites should use that style.

## Modules
- Use C++ modules (`export module ...;`, `import ...;`) consistently.
- Do not replace module units with headers.
- Avoid unnecessary new `import`s; reuse existing modules when possible.

## Error handling & logging
- Return rich errors rather than `bool` where practical.
- Use `dbg::eprintln`/`dbg::println` only when the existing code in that area already logs.
- Preserve existing error messages and formats when modifying code.

## Performance & safety
- Avoid extra allocations in hot paths.
- Validate buffer sizes and offsets; prevent out-of-bounds access.
- Use `assert::check` for internal invariants, not for user input validation.
- Prefer `std::span` for passing arrays/buffers to ensure size safety.
- Minimize use of exceptions; prefer error codes or `std::expected` or `std::optional` for error handling.
- Avoid raw pointers; use smart pointers or references where ownership semantics are clear.
- Favor algorithms from the standard library over custom implementations for better performance and readability.
- Use `constexpr` functions and `consteval` where applicable to enable compile-time optimizations.

## Build/test
- Ensure changes build with CMake + Ninja.
- If tests exist for a module, update/add tests when changing behavior.
