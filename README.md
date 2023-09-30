# Deckard - Utility Modules

1. Add as submodule to your git tree.
2. in your CMakelists.txt add:
```
add_subdirectory(Deckard)
add_deckard(<your project> Deckard)
```


## Modules:
  - **piku.debug**
    ```cpp
    import piku.debug;
    
    piku::trace("Hello {}", "World"); // winmain.cpp(15): Hello World

    piku::println("Hi, World"); // Hi, World
    piku::print("no newline");

    piku::assert_msg(true, "Message");
    piku::assert(true);

    ```
