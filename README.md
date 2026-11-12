# CS2-RayTraceRemastered

English | [简体中文](README.zh-CN.md)

A [Metamod:Source](https://www.metamodsource.net/) plugin for Counter-Strike 2 dedicated servers. It is a source-available, ABI-compatible rebuild of Slynx's `RayTrace` plugin (deployed build `v1.0.15 @ e136f9a`, upstream source unavailable). It registers the same `CRayTraceInterface002` interface through MetaMod's MetaFactory mechanism, so existing managed consumers (`RayTraceApi.dll` / `RayTraceImpl.dll`, and the `raytrace:craytraceinterface` plugin capability) keep working without modification.

Verified in production on CS2 `1.41.9.6` with MetaMod API 18 — `meta list` shows `RayTrace (v1.0.15-remaster)`.

## How it works

- On load, the plugin locates `gamedata.json` relative to its own `.so` path, signature-scans `libserver.so` for `UTIL_TraceShape` and `GetGameTraceManager`, then resolves the `g_pGameTraceManager` global via a RIP-relative dereference.
- It registers an `IMetamodListener` and answers `OnMetamodQuery("CRayTraceInterface002")` with a global singleton implementing the interface.
- All four trace methods build a native `Ray_t` / `CTraceFilter` from the managed `TraceOptions`, call the engine's `UTIL_TraceShape(gtm, ray, start, end, filter, CGameTrace*)`, and map `CGameTrace` onto the managed `TraceResult` layout — no temporary probe entities are spawned.
- `TraceOptions.interactsWith` / `interactsExclude` are written to `CTraceFilter::m_nInteractsWith` / `m_nInteractsExclude`; the `ignore` entity (if any) is placed into `m_nEntityIdsToIgnore[0]`.

## Interface contract

The managed side resolves `Utilities.MetaFactory("CRayTraceInterface002")` and binds vtable slots `2..5` on Linux (`1..4` on Windows — the Itanium ABI uses two slots for the virtual destructor):

| Slot (linux/win) | Method | Purpose |
|---|---|---|
| 2 / 1 | `TraceShape(start, angles, ignore, options, out result)` | Directional ray trace, fixed 8192-unit range |
| 3 / 2 | `TraceEndShape(start, end, ignore, options, out result)` | Ray trace between two points |
| 4 / 3 | `TraceHullShape(start, end, mins, maxs, ignore, options, out result)` | Hull (box) trace |
| 5 / 4 | `TraceShapeEx(start, end, filter, ray, out result)` | Caller-supplied `CTraceFilter*` / `Ray_t*` |

`TraceOptions` (24 bytes) and `TraceResult` (44 bytes) are byte-compatible mirrors of the managed `ExplicitLayout` structs. The full ABI is documented in `docs/CONTRACT.md`; the MetaFactory binding mechanism is documented in `docs/BINDING.md`. Decompiled reference sources of the managed assemblies are in `docs/*.decompiled.cs`.

## Deployment

Layout relative to the game directory:

```
addons/RayTrace/
├── bin/linuxsteamrt64/RayTrace.so   # this plugin
└── gamedata.json                    # signature database
```

- Register the plugin in `addons/metamod/metaplugins.ini` like any other MetaMod plugin (path to `addons/RayTrace/bin/linuxsteamrt64/RayTrace.so`).
- `gamedata.json` is resolved relative to the `.so` location (`addons/<name>/gamedata.json`), so it must sit in the addon root.
- Only the two signatures shipped in `gamedata/gamedata.json` (`UTIL_TraceShape`, `GetGameTraceManager`) are required — both verified unique-match on build 14196. Extra entries (e.g. the legacy entity-probe signatures carried by older deployments) are ignored.
- On successful load the console prints `[RayTraceRemastered] loaded: gtm=0x… util=0x…` and `meta list` shows `RayTrace (v1.0.15-remaster) by xiaoyueyoqwq (remaster)`.

## Building (Linux only)

Requirements:

- `hl2sdk-cs2` (with protobuf 3.21.8 under `thirdparty/`)
- `metamod-source` headers (API 18; the plugin uses no SourceHook/KHook hooks — only `OnMetamodQuery`)
- `protoc` plus the CS2 `.proto` sources (listed in `CMakeLists.txt`)

```bash
export HL2SDKCS2=/path/to/hl2sdk-cs2 \
       MMSOURCE_DEV=/path/to/metamod-source \
       CSGO_PROTO=/path/to/hl2sdk-cs2 \
       PROTOC=/path/to/protoc
cmake -B build && cmake --build build -j
# → build/RayTrace.so
```

Notes:

- Windows build is not wired up in `CMakeLists.txt` (the gamedata already carries windows signatures, but only the Linux build has been produced and deployed).
- `src/plugin.cpp` exports minimal `sqrtf` / `atan2f` / `asinf` / `acosf` / `sin` / `cos` implementations so the binary loads on servers whose glibc is older than the build host's (no references to newer versioned libm symbols).

## Known limitations

- `TraceOptions.drawBeam` is accepted but not implemented (the original presumably rendered a debug beam).
- `TraceShape`'s trace distance is hardcoded to 8192 units (following the CS2TraceRay convention; the original constant is unknown).
- On signature mismatch (e.g. after a game update breaks the gamedata) `Load` fails with a console error instead of loading half-bound.

## Repository layout

- `src/plugin.cpp` — plugin entry point, MetaFactory listener, trace implementations
- `src/CRayTraceInterface002.h` — interface and POD layouts (must match the managed ABI byte-for-byte)
- `src/common/sig_scan.*` — module enumeration, pattern scanning, gamedata loader
- `gamedata/gamedata.json` — engine signatures
- `docs/` — ABI contract, binding notes, decompiled managed sources, development/deployment log (`STATUS.md`)

## License

GPL-3.0
