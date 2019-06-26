# mojo-embedder
该项目允许在非chromium build环境来编译Mojo embedder

Mojo是Chromium团队开发的多进程／多线程通信模块，mojo embedder提供了最底层的通信API。

目前mojo只能在基于类chromium的项目中使用，比如chromium, fuchsia, android等，要在第三方应用中使用mojo有点难度。因为mojo绑定了很多chromium依赖，主要来自base和build工具链的直接和间接依赖，mojo甚至依赖libchrome。

该项目精简了mojo对于chromium的依赖，允许在非chromium build环境来编译mojo embedder, chromium基准版本为72.0.3579.0。后面再计划允许在第三方应用中编译mojo。

## 编译
编译依赖chromium depot_tools，目前只在ubuntu，编译target为android版本。

gn gen --args='target_os="android" target_cpu="arm" is_component_build=true' out/Debug
autoninja -C out/Debug mojo/core/embedder
