# 绑定机制（反编译自 RayTraceImpl.dll，build `e136f9a`）

## 托管侧（C# 插件 RayTraceImpl，已完整拿到行为）

```csharp
Handle = Utilities.MetaFactory("CRayTraceInterface002");   // 查 MM 接口注册表
if (Handle == 0) fail;
// VirtualFunction.Create<...>(Handle, vtableIndex) 按槽位绑：
//   Linux:   TraceShape=2, TraceEndShape=3, TraceHullShape=4, TraceShapeEx=5
//   Windows: TraceShape=1, TraceEndShape=2, TraceHullShape=3, TraceShapeEx=4
```

vtable 槽位差 1 = Itanium ABI 虚析构占两个槽位（complete+deleting），MSVC 占一个。
⇒ **原生接口 = 带虚析构的纯虚类，4 个方法按声明顺序排在析构后面**

## 插件注册流程（托管侧，已实现）

- `PluginCapability<CRayTraceInterface>("raytrace:craytraceinterface")` —— C# 插件间 capability 名（消费插件用它取）
- `Load()` 里 `Capabilities.RegisterPluginCapability(..., () => new CRayTrace())`
- `OnMetamodAllPluginsLoaded` 时 `NativeBridge.Initialize()` → `MetaFactory("CRayTraceInterface002")` → 失败打红字 `[RayTraceImpl] Native bridge initialization failed.`
- `CRayTrace` 各方法 = 转发到 native vtable 槽，`NativeBridge.Loaded=false` 时直接 `return false`

## 原生侧必须提供（remastered 实现目标）

```cpp
// 二进制布局 = 托管侧按槽位直调
struct TraceOptions { uint64_t interactsWith; uint64_t interactsExclude; int32_t drawBeam; }; // 24B
struct TraceResult  { float endPos[3]; void* hitEntity; float fraction; int32_t allSolid; float normal[3]; }; // 44B

class CRayTraceInterface002 {
public:
    virtual ~CRayTraceInterface002() {}                                              // slot 0-1 (linux)
    virtual bool TraceShape    (void* start, void* angles, void* ignore, TraceOptions* opts, TraceResult* res) = 0;                       // slot 2
    virtual bool TraceEndShape (void* start, void* end,   void* ignore, TraceOptions* opts, TraceResult* res) = 0;                       // slot 3
    virtual bool TraceHullShape(void* start, void* end, void* mins, void* maxs, void* ignore, TraceOptions* opts, TraceResult* res) = 0; // slot 4
    virtual bool TraceShapeEx  (void* start, void* end, void* filter, void* ray, TraceResult* res) = 0;                                  // slot 5
};
```

- 通过 **Metamod MetaFactory 机制**以名字 `CRayTraceInterface002` 注册一个全局单例（用与 BotIdentity/CSS 相同的 S2 原生接口注册通路——`ISmmAPI` 的 MetaFactory/RegisterInterface 系，写码时对照 `counterstrikesharp.so` 里 `MetaFactory` 的实现确认最终调用名）
- `start/end/mins/maxs/angles` = `Vector*`/`QAngle*`（hl2sdk `Vector`、`QAngle`，12B float×3）
- `ignore` = `CEntityInstance*`，可为空
- `filter`/`ray` = 原生 `CTraceFilter*`/`Ray_t*`（hl2sdk 类型）
- 返回 bool = 托管侧 `NativeBridge.Loaded` 前置；实现里失败也要把 `*res` 填成零并返回 false

## 引擎 trace 实现路径（不用逆）

`~/project/CS2TraceRay/CS2TraceRay/Class/TraceRay.cs` 示范了同一批引擎函数：
- `NativeAPI.FindSignature("TraceFunc")` → `TraceShapeDelegate(gtm, start, end, skip, mask, content, CGameTrace*)` Cdecl
- `"TraceShape"` → `TraceShapeRayFilterDelegate(gtm, Ray*, start, end, CTraceFilter*, CGameTrace*)` Cdecl
- `"CTraceFilterVtable"`、`"GameTraceManager"`（`GetAbsoluteAddress(addr,3,7)` 解引用后取 `*(IntPtr*)`）

原生侧从 `gamedata.json` 拿同样签名即可；`AcceptInput` 断的那条从上游 CSS gamedata 借。
