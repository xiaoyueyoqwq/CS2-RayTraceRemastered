# CS2-RayTraceRemastered

[English](README.md) | 简体中文

面向 Counter-Strike 2 专用服务器的 [Metamod:Source](https://www.metamodsource.net/) 插件。这是对 Slynx `RayTrace` 插件（部署件 `v1.0.15 @ e136f9a`，上游源码不可寻）的开源、ABI 兼容重构。它通过 MetaMod 的 MetaFactory 机制注册同名 `CRayTraceInterface002` 接口，因此现有托管消费方（`RayTraceApi.dll` / `RayTraceImpl.dll`，以及 `raytrace:craytraceinterface` 插件 capability）无需任何改动即可继续工作。

已在 CS2 `1.41.9.6` + MetaMod API 18 生产环境验证——`meta list` 显示 `RayTrace (v1.0.15-remaster)`。

## 工作原理

- 加载时，插件根据自身 `.so` 路径定位 `gamedata.json`，在 `libserver.so` 中签名扫描 `UTIL_TraceShape` 与 `GetGameTraceManager`，再通过 RIP 相对寻址解引用拿到 `g_pGameTraceManager` 全局变量。
- 插件注册一个 `IMetamodListener`，在 `OnMetamodQuery("CRayTraceInterface002")` 时返回实现该接口的全局单例。
- 四个 trace 方法全部由托管侧 `TraceOptions` 构造原生 `Ray_t` / `CTraceFilter`，调用引擎函数 `UTIL_TraceShape(gtm, ray, start, end, filter, CGameTrace*)`，再把 `CGameTrace` 映射到托管侧 `TraceResult` 布局——全程不生成临时探针实体。
- `TraceOptions.interactsWith` / `interactsExclude` 写入 `CTraceFilter::m_nInteractsWith` / `m_nInteractsExclude`；`ignore` 实体（如有）填入 `m_nEntityIdsToIgnore[0]`。

## 接口契约

托管侧通过 `Utilities.MetaFactory("CRayTraceInterface002")` 取得接口，并按 vtable 槽位绑定：Linux 为 `2..5`，Windows 为 `1..4`（Itanium ABI 的虚析构占两个槽位）：

| 槽位（linux/win） | 方法 | 用途 |
|---|---|---|
| 2 / 1 | `TraceShape(start, angles, ignore, options, out result)` | 按方向的射线 trace，固定 8192 单位距离 |
| 3 / 2 | `TraceEndShape(start, end, ignore, options, out result)` | 两点间射线 trace |
| 4 / 3 | `TraceHullShape(start, end, mins, maxs, ignore, options, out result)` | Hull（盒体）trace |
| 5 / 4 | `TraceShapeEx(start, end, filter, ray, out result)` | 调用方自带 `CTraceFilter*` / `Ray_t*` |

`TraceOptions`（24 字节）与 `TraceResult`（44 字节）是托管侧 `ExplicitLayout` 结构体的逐字节镜像。完整 ABI 见 `docs/CONTRACT.md`；MetaFactory 绑定机制见 `docs/BINDING.md`。托管程序集的反编译参考源码在 `docs/*.decompiled.cs`。

## 部署

相对游戏目录的布局：

```
addons/RayTrace/
├── bin/linuxsteamrt64/RayTrace.so   # 本插件
└── gamedata.json                    # 签名数据库
```

- 在 `addons/metamod/metaplugins.ini` 中像普通 MetaMod 插件一样注册（指向 `addons/RayTrace/bin/linuxsteamrt64/RayTrace.so` 的路径）。
- `gamedata.json` 按 `.so` 所在位置解析（`addons/<name>/gamedata.json`），必须放在 addon 根目录。
- 只需要 `gamedata/gamedata.json` 里的两条签名（`UTIL_TraceShape`、`GetGameTraceManager`）——均在 14196 构建上验证唯一命中。多余条目（例如旧部署件携带的实体探针签名）会被忽略。
- 加载成功时控制台打印 `[RayTraceRemastered] loaded: gtm=0x… util=0x…`，`meta list` 显示 `RayTrace (v1.0.15-remaster) by xiaoyueyoqwq (remaster)`。

## 构建（仅 Linux）

依赖：

- `hl2sdk-cs2`（`thirdparty/` 下需含 protobuf 3.21.8）
- `metamod-source` 头文件（API 18；插件不使用 SourceHook/KHook 钩子——仅用 `OnMetamodQuery`）
- `protoc` 及 CS2 `.proto` 源文件（清单见 `CMakeLists.txt`）

```bash
export HL2SDKCS2=/path/to/hl2sdk-cs2 \
       MMSOURCE_DEV=/path/to/metamod-source \
       CSGO_PROTO=/path/to/hl2sdk-cs2 \
       PROTOC=/path/to/protoc
cmake -B build && cmake --build build -j
# → build/RayTrace.so
```

说明：

- `CMakeLists.txt` 未接入 Windows 构建（gamedata 已带 windows 签名，但只产出并部署过 Linux 版）。
- `src/plugin.cpp` 导出了精简的 `sqrtf` / `atan2f` / `asinf` / `acosf` / `sin` / `cos` 实现，使二进制能在 glibc 版本低于构建机的服务器上加载（不引用新版带版本号的 libm 符号）。

## 已知限制

- `TraceOptions.drawBeam` 会被接收但未实现（原版大概率是调试画线）。
- `TraceShape` 的 trace 距离硬编码为 8192 单位（沿用 CS2TraceRay 惯例；原版常量未知）。
- 签名失配（例如游戏更新导致 gamedata 失效）时，`Load` 直接失败并报控制台错误，不会以半绑定状态加载。

## 仓库结构

- `src/plugin.cpp` —— 插件入口、MetaFactory 监听器、trace 实现
- `src/CRayTraceInterface002.h` —— 接口与 POD 布局（必须与托管 ABI 逐字节一致）
- `src/common/sig_scan.*` —— 模块枚举、模式扫描、gamedata 加载器
- `gamedata/gamedata.json` —— 引擎签名
- `docs/` —— ABI 契约、绑定笔记、托管程序集反编译源码、开发/部署日志（`STATUS.md`）

## 许可证

GPL-3.0
