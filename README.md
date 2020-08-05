# Riru-Il2CppDumper
Riru版Il2CppDumper，在游戏运行时dump数据，用于绕过保护，加密以及混淆。

## 如何食用
1. 安装[Magisk](https://github.com/topjohnwu/Magisk)和[Riru](https://github.com/RikkaApps/Riru)
2. 下载源码
3. 编辑`game.h`, 修改游戏包名`GamePackageName`和Unity版本`UnityVersion`，`UnityVersion`的值请看文件内注释
4. 使用Android Studio运行gradle任务`:module:assembleRelease`编译
5. 在Magisk里安装模块
6. 启动游戏，会在`/data/data/GamePackageName/files/`目录下生成`dump.cs`

## TODO
- [x] 低版本使用反射进行dump
- [x] 高版本使用`il2cpp_image_get_class`进行dump
- [ ] 完善dump.cs输出
- [ ] 泛型相关输出
- [ ] 生成IDA脚本
- [ ] 生成头文件
