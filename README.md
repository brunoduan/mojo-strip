# mojo-embedder
该项目允许在非chromium build环境来编译和使用Mojo。

愿景：非chromium应用能够使用mojo作为多个进程之间的通信机制。

## 概述
Mojo是Chromium团队开发的多进程、多线程通信模块，mojo embedder提供了最底层的通信API。

目前mojo只能在基于类chromium的项目中使用，比如chromium, fuchsia, android等，要在第三方应用中使用mojo有点难度。因为mojo绑定了很多chromium依赖，主要来自base和build工具链的直接和间接依赖，mojo甚至依赖libchrome。

该项目精简了mojo对于chromium的依赖，允许在非chromium build环境来编译mojo embedder, chromium基准版本为72.0.3579.0。

## 编译
编译依赖chromium depot_tools，目前只在ubuntu，编译target为android版本。

```
gn gen --args='target_os="android" target_cpu="arm" is_component_build=true' out/Debug
autoninja -C out/Debug samples_shell_apk
```

## TODO
- [x] 将mojo, chromium base, chromium build从chromium中剥离出来
- [x] 实现samples_shell_apk工程，支持打出MySamplesShell.apk
- [x] 精简content层，实现单进程初始化流程，包括master, slaver, utility等多个线程启动初始化
- [x] 增加service_manager
- [x] 增加service manifest
- [x] 单进程中跑通echo service
- [ ] 多进程中跑通echo service
- [ ] 验证非父子进程是否能够使用mojo进行通信
- [ ] Sandbox支持
- [ ] 权限管理
- [ ] 尝试将网络组件独立成service
- [ ] 其他通用services
