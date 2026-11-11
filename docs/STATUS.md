# STATUS — RayTraceRemastered (2026-11-11 凌晨，v1 已上线)

## ✅ 已完成 & 已验证

- **部署上线**：`addons/RayTrace/bin/linuxsteamrt64/RayTrace.so` 替换为 remaster 版，
  `meta list` 显示 `[02] RayTrace (v1.0.15-remaster) by xiaoyueyoqwq (remaster)`
- **绑定链路全通**：`[RayTraceImpl] Managed side initialized.`——`MetaFactory("CRayTraceInterface002")`
  返回接口，托管侧 4 个 `VirtualFunction.Create` 槽位绑定成功
- **无崩溃**：启动后 0 segfault，server 稳定
- 备份原件在 `/home/steam/recovery/pre-raytrace-remaster-20261111-*/`

## 实现路径（最终生效版）

- MetaFactory 机制：`ISmmPlugin + IMetamodListener`，`OnMetamodQuery("CRayTraceInterface002")` 返回接口单例
- 引擎调用：`UTIL_TraceShape(gtm, Ray_t*, start, end, CTraceFilter*, CGameTrace*)`（签名来自 CS2_VibeSignatures 14196 库，唯一命中）
- `g_pGameTraceManager`：`GetGameTraceManager` sig → `mov rdi,[rip+X]` → `*(insn+7+disp32@+3)` 解析
- filter/ray/trace 用 hl2sdk 真实类型（`gametrace.h`/`ray.h`），不需要 CTraceFilterVtable/TraceFunc 签名
- glibc 兼容：自写 `sqrtf/atan2f/asinf/acosf/sin/cos`（服务器 glibc 2.39 < 构建机 2.43）
- `libserver.so` 解析必须排除 `addons/` 路径（metamod 自带 300KB 同名垫片）

## 部署包构成

- `addons/RayTrace/bin/linuxsteamrt64/RayTrace.so` = 本项目 `build/RayTrace.so`
- `addons/RayTrace/gamedata.json` = 旧 4 条实体签名 + 2 条新引擎签名（merged，11 entries）

## 待办/未验证项

1. **TraceShape 的 8192 距离常量**——原实现未知，按 CS2TraceRay 库惯例用 8192；若消费侧传远距 trace 结果偏差需调
2. **drawBeam** —— `TraceOptions.drawBeam` 未实现（原版可能是调试画线，低优先）
3. **运行时 trace 正确性**——绑定通了但还没实测命中结果；等 bot 生态实际调用时观察（或写测试插件打一条 trace）
4. **API18 重建**——迁移那天换 MM≥18 头文件重编即可（代码不用改）

## 环境（重建用）

```bash
DEPS=/home/xiaoyueyoqwq/project/CS2-Server/.codex/deps
export HL2SDKCS2=$DEPS/hl2sdk-cs2 MMSOURCE_DEV=$DEPS/metamod-source \
  CSGO_PROTO=$DEPS/hl2sdk-cs2 PROTOC=$DEPS/protoc/bin/protoc
cmake -B build && cmake --build build -j8
```

## API18 迁移发现（2026-11-11 补）

- **新 master（metamod-source-18 @ ea513d8, 11/9）里 `core/sourcehook/` 整个删了**——没有 sourcehook.h，没有 SH_* 宏。API18 = SourceHook 移除 + 强制 KHook（`core/metamod_khook.h`，`KHook::SetupHook`/`SetupVirtualHook`）
- 所以 BotIdentity/CSS 的"重编"实际是 **SH → KHook 代码迁移**（不是换头文件）
- RayTraceRemastered **没用 hook**，纯 `OnMetamodQuery` 注册——API18 侧只需换 SDK 源列表（sourcehook.cpp 那堆没了，换 khook 配套）+ 用新头文件编
- worktree 备好：`/home/xiaoyueyoqwq/project/CS2-Server/.codex/deps/metamod-source-18`（分离 HEAD ea513d8）

## 剩余迁移工程量修正

| 件 | API18 出路 | 修正成本 |
|---|---|---|
| BotController/MAM | 已有 API18 二进制（staged） | 0 |
| RayTraceRemastered | 换 SDK_SOURCES + 新头文件重编 | **低** |
| BotIdentity | SourceHook → KHook 源码迁移 + 重编 | **中** |
| CounterStrikeSharp | 上游 v1.0.379 已是 API18（丢 PR1314）或 vLocal + PR1314 + khook 重编 | **高** |

## API18 切换完成（2026-11-11 02:20）

- 线上 MM1490 + 五件全起：[01]BotIdentity 0.1.33(khook) [02]RayTrace-remaster [03]CSS vLocal+PR1314 [04]BotController 0.9.0 [05]MAM 1.7.3
- BotIdentity api18 构建要点：KHOOK_STANDALONE 不要定义（插件走 metamod 的 __exported__khook/GetDetourInterface）；PROTO_SRCS 只给头不编译（.pb.cc 会拖 descriptor_table 符号）
- 备份：/home/steam/recovery/pre-api18-cutover-<ts>/
