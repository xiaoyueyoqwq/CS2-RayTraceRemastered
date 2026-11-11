# CS2-RayTraceRemastered

重构 Slynx 的 `RayTrace` Metamod 插件（部署件 `v1.0.15 @ e136f9a`，上游源码不可寻）——目标是做一个 **API-18 兼容、对 1.41.9.6 签名有效**的等价物，保持 `CRayTraceInterface` 契约不变，让现有 `RayTraceApi.dll` 消费方（bot 生态）无需改动。

## 已锁定的信息

- 完整托管契约：`docs/CONTRACT.md`（反编译自线上 `RayTraceApi.dll`）
- 引擎层 trace 实现参考：`~/project/CS2TraceRay`（SlynxCZ 公开的 C# 库，示范了用 `NativeAPI.FindSignature` 直调 `GameTraceManager`/`TraceShape`/`CTraceFilterVtable` 的路径——原生侧用同一批引擎函数）
- 部署 gamedata 4 条里 3 条仍命中新二进制，`AcceptInput` 断——上游 CSS gamedata 同名签名可直接抄

## 待确认项（写码前）

1. ~~接口发现机制~~ 已钉死：MetaFactory 注册名 `CRayTraceInterface002`，vtable 槽 2-5（详见 docs/BINDING.md）
2. **实体探针路径**：部署件引用了 `CreateEntityByName`/`DispatchSpawn`/`AcceptInput`/`AddEntityIOEvent`——大概率是 spawn 临时实体做 hull/trigger 碰撞探针；重写时要么复刻要么用 `GameTraceManager` 的 shape trace 替代（优先替代，少一条实体链）
3. **构建环境**：ambuild/cmake + hl2sdk-cs2（`.codex/deps` 有）+ MM ≥ API18 头文件

## 方向

写一个新 metamod 插件（C++），加载时注册同名 `CRayTraceInterface` 实现：

- `TraceShape` / `TraceEndShape` / `TraceHullShape` / `TraceShapeEx` → `GameTraceManager::TraceShape`（引擎原生，签名已验）
- 按 `docs/CONTRACT.md` 的 ABI 逐字节匹配 TraceOptions/TraceResult 内存布局
- gamedata 只需要它真正用到的签名；`AcceptInput` 若还需要，从上游 CSS gamedata 借

## 约束

- 不向任何上游送 PR；这是本地重构件
- 部署前先在空服验证 `meta list` 加载 + 消费插件 `botidentity:api`-style 取接口成功
