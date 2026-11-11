# RayTrace API 契约（反编译自服务器部署的 RayTraceApi.dll）

来源：`addons/counterstrikesharp/shared/RayTraceApi/RayTraceApi.dll`
构建版本：`1.0.0+e136f9af2d2cecf431ace26a5f2d02c3ea8d6d9f`（e136f9a 不在任何公开仓库）
命名空间：`RayTraceAPI`

## 接口

```csharp
public interface CRayTraceInterface
{
    bool TraceShape    (Vector start, QAngle angles, CEntityInstance? ignore, TraceOptions options, out TraceResult result);
    bool TraceEndShape (Vector start, Vector end,     CEntityInstance? ignore, TraceOptions options, out TraceResult result);
    bool TraceHullShape(Vector start, Vector end, Vector mins, Vector maxs, CEntityInstance? ignore, TraceOptions options, out TraceResult result);
    bool TraceShapeEx  (Vector start, Vector end, nint filter, nint ray, out TraceResult result);
}
```

## TraceOptions（ExplicitLayout, Size=24）

| Offset | Field | Type | Default |
|---|---|---|---|
| 0 | InteractsWith | ulong | 0x2C3091 (2895889) |
| 8 | InteractsExclude | ulong | 0 |
| 16 | DrawBeam | int | 0 |

## TraceResult（ExplicitLayout, Size=44）

| Offset | Field | Type |
|---|---|---|
| 0 | EndPosX/Y/Z | float ×3 |
| 16 | HitEntity | nint（CEntityInstance*） |
| 24 | Fraction | float（DidHit = Fraction < 1） |
| 28 | AllSolid | int |
| 32 | NormalX/Y/Z | float ×3 |

## InteractionLayers（ulong flags）

完整枚举见反编译源 `RayTraceApi.decompiled.cs`（共 ~48 项），关键掩码：

- `MASK_SHOT_PHYSICS = Solid|PlayerClip|Window|PassBullets|Player|NPC|Physics_Prop`
- `MASK_SHOT_HITBOX = Hitboxes|Player|NPC`
- `MASK_SHOT_FULL   = MASK_SHOT_PHYSICS|Hitboxes`
- `MASK_WORLD_ONLY = 0x3001`
- `MASK_GRENADE    = 0x203001`
- `MASK_BRUSH_ONLY = 0x1001`
- `MASK_PLAYER_MOVE= 0x3011`
- `MASK_NPC_MOVE   = MASK_WORLD_ONLY|NPCClip`

## 原生端需要匹配的 ABI

- 注册名（consumer 侧取接口的名字）：`RayTraceAPI`（dll 内字符串，接口注册键）
- 调用约定：托管侧 `UnmanagedFunctionPointer(CallingConvention.Cdecl)` 或直接 capability 对象——**待 RayTraceImpl.dll 反编译确认**
- 结构体按上面 ExplicitLayout 逐字节复刻（C++ 端 POD 布局必须一致）
- `CEntityInstance? ignore`：托管 nullable → native `CEntityInstance*`，允许 NULL

## 部署 gamedata 现状（addons/RayTrace/gamedata.json）

| 签名 | 新二进制命中 |
|---|---|
| UTIL_CreateEntityByName | ✅ 1 |
| CBaseEntity_DispatchSpawn | ✅ 1 |
| CEntityInstance_AcceptInput | ❌ 0（坏——上游 CSS gamedata 同名签名已验证可用，直接抄） |
| CEntitySystem_AddEntityIOEvent | ✅ 1 |

说明部署件除纯 trace 外还有**生成/驱动临时实体**的路径（探针实体），重写时必须保留该路径或等价物。
