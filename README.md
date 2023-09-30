# Deckard - Utility Modules

1. Add as submodule to your git tree.
2. in your CMakelists.txt add:
```
add_subdirectory(Deckard)
add_deckard(<your project> Deckard)
```


## Modules:
  - **deckard.debug**
    ```cpp
    import deckard.debug;
    
    deckard::trace("Hello {}", "World"); // winmain.cpp(15): Hello World

    deckard::assert_msg(true, "Message");
    deckard::assert(true);

    ```
 - **Types**
      ```cpp
      import deckard.types;

      // i8,u8,i16,u16,i32,u32,i64,u64

      u8 u  = 64_u8;
      i64 i = 112233_i64;

      auto value = as<uint32>(256);

      ```
   - **Fileview**
        ```cpp 
        import deckard.file;

        Fileview f;
        f.open("dice.qoi", FileAccess::ReadWrite);

        f[f.size()-1] = 0x01;
        ```
