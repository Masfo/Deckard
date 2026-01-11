# Copilot Instructions

## Project
- CMake project targeting Windows (Win32 APIs used) with Ninja generator.
- Use C++23 where available; keep code compatible with MSVC.
- Prefer standard library facilities; avoid adding new external dependencies.

## Coding style
- Follow existing naming and formatting in the surrounding files (tabs/indentation, brace style).
- Prefer `std::expected<T, std::string>` for fallible operations in new code.
- Keep APIs minimal and consistent with existing `deckard::*` modules.

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

## Build/test
- Ensure changes build with CMake + Ninja.
- If tests exist for a module, update/add tests when changing behavior.
