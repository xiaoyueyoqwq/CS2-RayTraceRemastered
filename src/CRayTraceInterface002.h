// CRayTraceInterface002 — ABI-compatible with deployed RayTraceApi.dll contract.
// Managed side binds vtable slots: linux 2..5 (two dtor slots), windows 1..4.
// Keep method ORDER exactly: TraceShape, TraceEndShape, TraceHullShape, TraceShapeEx.
#pragma once
#include <cstdint>

class Vector;
class QAngle;
class CEntityInstance;
class CTraceFilter;
struct Ray_t;

namespace raytrace
{
// Mirror of RayTraceAPI.TraceOptions (ExplicitLayout, 24 bytes)
struct TraceOptions
{
    uint64_t interactsWith;
    uint64_t interactsExclude;
    int32_t  drawBeam;
    int32_t  _pad; // explicit trailing pad for alignment safety (managed size=24, no pad field)
};
static_assert(sizeof(TraceOptions) == 24, "TraceOptions must be 24 bytes");

// Mirror of RayTraceAPI.TraceResult (ExplicitLayout, 44 bytes)
struct TraceResult
{
    float   endPos[3];
    void*   hitEntity;
    float   fraction;
    int32_t allSolid;
    float   normal[3];
};
static_assert(offsetof(TraceResult, hitEntity) == 16 &&
              offsetof(TraceResult, fraction) == 24 &&
              offsetof(TraceResult, allSolid) == 28 &&
              offsetof(TraceResult, normal) == 32,
              "TraceResult field offsets must match managed 44B layout");
} // namespace raytrace

class CRayTraceInterface002
{
public:
    virtual ~CRayTraceInterface002() {}
    // vtable slot 2 (linux) / 1 (win)
    virtual bool TraceShape(Vector* start, QAngle* angles, CEntityInstance* ignore,
                            raytrace::TraceOptions* opts, raytrace::TraceResult* res) = 0;
    // vtable slot 3 (linux) / 2 (win)
    virtual bool TraceEndShape(Vector* start, Vector* end, CEntityInstance* ignore,
                               raytrace::TraceOptions* opts, raytrace::TraceResult* res) = 0;
    // vtable slot 4 (linux) / 3 (win)
    virtual bool TraceHullShape(Vector* start, Vector* end, Vector* mins, Vector* maxs,
                                CEntityInstance* ignore, raytrace::TraceOptions* opts,
                                raytrace::TraceResult* res) = 0;
    // vtable slot 5 (linux) / 4 (win)
    virtual bool TraceShapeEx(Vector* start, Vector* end, CTraceFilter* filter, Ray_t* ray,
                              raytrace::TraceResult* res) = 0;
};
