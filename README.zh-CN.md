# Zygisk-Il2CppDumper
Zygisk版Il2CppDumper，在游戏运行时dump il2cpp数据，可以绕过保护，加密以及混淆。

## 如何食用
1. 安装[Magisk](https://github.com/topjohnwu/Magisk) v24以上版本并开启Zygisk
2. 下载源码
3. 编辑`game.h`, 修改`GamePackageName`为游戏包名
4. 使用Android Studio运行gradle任务`:module:assembleRelease`编译，zip包会生成在`out`文件夹下
5. 在Magisk里安装模块
6. 启动游戏，会在`/data/data/GamePackageName/files/`目录下生成`dump.cs`
