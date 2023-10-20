# Deckard - Utility Modules

1. Add as submodule to your git tree.
2. in your CMakelists.txt add:
```
add_subdirectory(Deckard)
target_link_libraries(${CMAKE_PROJECT_NAME} Deckard)
```


## Modules:
  - **deckard.debug**
    ```cpp
    import deckard.debug;

    trace("Hello {}", "World"); // winmain.cpp(15): Hello World
    dbgln("Similar to trace");  // Similar to trace
    dbg("Like dbgln but ");     // 
    dbg("w/o newline\n");       // Like dbgln but w/o newline 
	
    assert_msg(true, "Message");
    assert(true);

    ```
  - **Types**
      ```cpp
      import deckard.types;

      // i8,u8,i16,u16,i32,u32,i64,u64

      u8 u  = 64_u8;
      i64 i = 112233_i64;

      auto value = as<u32>(256);
      ```
  - **Fileview**
    ```cpp 
    import deckard.file;

    Fileview f;
    f.open("dice.qoi", FileAccess::ReadWrite);

    f[f.size()-1] = 0x01;
    ```
