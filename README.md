# Zygisk-Il2CppDumper
Il2CppDumper with Zygisk, dump il2cpp data at runtime, can bypass protection, encryption and obfuscation.

中文说明请戳[这里](README.zh-CN.md)

## How to use
1. Install [Magisk](https://github.com/topjohnwu/Magisk) v24 or later and enable Zygisk
2. Download the source code
3. Edit `game.h`, modify `GamePackageName` to the game package name
4. Use Android Studio to run the gradle task `:module:assembleRelease` to compile, the zip package will be generated in the `out` folder
5. Install modules in Magisk
6. Start the game, `dump.cs` will be generated in the `/data/data/GamePackageName/files/` directory