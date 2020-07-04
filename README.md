# Riru-Il2CppDumper
Riru版Il2CppDumper，在游戏运行时dump数据，用于绕过保护，加密以及混淆。

## 如何食用
1. 安装[Magisk](https://github.com/topjohnwu/Magisk)和[Riru](https://github.com/RikkaApps/Riru)
2. 下载源码
3. 编辑`game.h`, 修改游戏包名`GamePackageName`和Unity版本`UnityVersion`，`UnityVersion`的值请看文件内注释
4. 编译
5. 在Magisk里安装模块
6. 启动游戏，会在`/data/data/GamePackageName/files/`目录下生成`dump.cs`

## TODO
- [ ] 强化搜索
- [ ] 完善dump.cs输出
- [ ] 2018.3.0f2(24.1)及以上版本使用`il2cpp_image_get_class`
- [ ] 泛型相关输出
- [ ] 生成IDA脚本，头文件
