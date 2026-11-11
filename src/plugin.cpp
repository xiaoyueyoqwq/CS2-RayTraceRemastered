// RayTraceRemastered — Metamod:Source plugin exposing CRayTraceInterface002.
//
// ABI-compatible replacement for Slynx's RayTrace.so (deployed v1.0.15 @ e136f9a).
// Managed side (RayTraceImpl.dll) does:
//   Utilities.MetaFactory("CRayTraceInterface002")  -> vtable slots 2..5
// MetaFactory iterates each plugin's IMetamodListener::OnMetamodQuery — we
// register ourselves as a listener and answer that name (see docs/BINDING.md).

#include <ISmmPlugin.h>
#include <interfaces/interfaces.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include <dlfcn.h>

#include <nlohmann/json.hpp>
#include <mathlib/vector.h>
#include <gametrace.h>
#include <ray.h>
#include <entity2/entityinstance.h>
#include <entity2/entityidentity.h>

#include "common/sig_scan.h"
#include "CRayTraceInterface002.h"

PLUGIN_GLOBALVARS();

namespace
{
// UTIL_TraceShape(gtm, ray, start, end, filter, out) — verified vs 1.41.9.6
// (gamedata sig "UTIL_TraceShape", System V, 6 args; see docs/STATUS.md)
using UTILTraceShapeFn = bool (*)(void* gtm, Ray_t* ray, const Vector* start,
                                  const Vector* end, CTraceFilter* filter, CGameTrace* trace);

void* g_pGameTraceManager = nullptr;
UTILTraceShapeFn g_UtilTraceShape = nullptr;

void FillResult(const CGameTrace& tr, raytrace::TraceResult* res)
{
    res->endPos[0] = tr.m_vEndPos.x;
    res->endPos[1] = tr.m_vEndPos.y;
    res->endPos[2] = tr.m_vEndPos.z;
    res->hitEntity = tr.m_pEnt;
    res->fraction = tr.m_flFraction;
    res->allSolid = tr.m_bStartInSolid ? 1 : 0;
    res->normal[0] = tr.m_vHitNormal.x;
    res->normal[1] = tr.m_vHitNormal.y;
    res->normal[2] = tr.m_vHitNormal.z;
}

void ZeroResult(raytrace::TraceResult* res)
{
    if (res) std::memset(res, 0, sizeof(*res));
}

// Resolve "mov/lea reg, [rip+X]" ref inside a matched instruction:
// eff = match + insnLen + *(int32*)(match + dispOff)
void* RipTarget(void* match, int insnLen, int dispOff)
{
    auto base = reinterpret_cast<uintptr_t>(match);
    int32_t disp = *reinterpret_cast<int32_t*>(base + dispOff);
    return reinterpret_cast<void*>(base + insnLen + disp);
}

// libm-free trig (server libm lacks newer versioned symbols our toolchain emits).
// Minimax on [-PI/2, PI/2] after quadrant folding; |err| < ~2e-7 — plenty for a
// direction vector.
float SinLocal(float x)
{
    constexpr float kPi = 3.14159265358979323846f;
    // wrap into [-pi, pi]
    x = x - static_cast<float>(static_cast<int32_t>(x / kPi + (x >= 0 ? 0.5f : -0.5f))) * kPi;
    // fold quadrant: sin(x) for x in [-pi/2,pi/2]
    if (x > kPi * 0.5f) x = kPi - x;
    else if (x < -kPi * 0.5f) x = -kPi - x;
    const float x2 = x * x;
    return x * (((x2 * -2.4725516e-4f + 8.3321609e-3f) * x2 - 1.6666657e-1f) * x2 + 1.0f);
}
float CosLocal(float x) { return SinLocal(x + 1.57079632679489661923f); }
} // namespace

// libm-free exports: our build host's glibc is newer than the server's; providing
// these locally stops the .so from referencing versioned libm symbols it lacks.
// Accuracy ~1e-6/1e-7 — fine for mathlib/protobuf internal angle paths.
extern "C" float sqrtf(float x)
{
    float r;
    __asm__("sqrtss %1, %0" : "=x"(r) : "x"(x));
    return r;
}

extern "C" float atan2f(float y, float x)
{
    if (x == 0.0f) return y >= 0 ? 1.57079632679489661923f : -1.57079632679489661923f;
    const float z = y / x;
    const float z2 = z * z;
    // atan(z) via 11th-order poly on [-1,1] + half-plane wrap
    float a = z * (0.9999993329f + z2 * (-0.3332985605f + z2 * (0.1994653599f + z2 * (-0.1390853351f + z2 * 0.0964200442f))));
    if (x < 0.0f) a += (y >= 0.0f ? 3.14159265358979323846f : -3.14159265358979323846f);
    return a;
}

extern "C" float asinf(float x)
{
    if (x > 1.0f) x = 1.0f;
    if (x < -1.0f) x = -1.0f;
    return atan2f(x, sqrtf(1.0f - x * x));
}

extern "C" float acosf(float x)
{
    if (x > 1.0f) x = 1.0f;
    if (x < -1.0f) x = -1.0f;
    return atan2f(sqrtf(1.0f - x * x), x);
}

class RayTraceIface final : public CRayTraceInterface002
{
  public:
    bool TraceShape(Vector* start, QAngle* angles, CEntityInstance* ignore,
                    raytrace::TraceOptions* opts, raytrace::TraceResult* res) override
    {
        const float pr = angles->x * (3.14159265358979323846f / 180.0f);
        const float yr = angles->y * (3.14159265358979323846f / 180.0f);
        const float cp = CosLocal(pr);
        Vector fwd(cp * CosLocal(yr), cp * SinLocal(yr), -SinLocal(pr));
        Vector end(start->x + fwd.x * 8192.0f,
                   start->y + fwd.y * 8192.0f,
                   start->z + fwd.z * 8192.0f);
        return TraceEndShape(start, &end, ignore, opts, res);
    }

    bool TraceEndShape(Vector* start, Vector* end, CEntityInstance* ignore,
                       raytrace::TraceOptions* opts, raytrace::TraceResult* res) override
    {
        if (!g_UtilTraceShape || !g_pGameTraceManager || !res) { ZeroResult(res); return false; }
        Ray_t ray;
        CTraceFilter filter;
        ApplyOptions(&filter, ignore, opts);
        CGameTrace tr;
        const bool ok = g_UtilTraceShape(g_pGameTraceManager, &ray, start, end, &filter, &tr);
        FillResult(tr, res);
        return ok;
    }

    bool TraceHullShape(Vector* start, Vector* end, Vector* mins, Vector* maxs,
                        CEntityInstance* ignore, raytrace::TraceOptions* opts,
                        raytrace::TraceResult* res) override
    {
        if (!g_UtilTraceShape || !g_pGameTraceManager || !res) { ZeroResult(res); return false; }
        Ray_t ray(*mins, *maxs); // hull
        CTraceFilter filter;
        ApplyOptions(&filter, ignore, opts);
        CGameTrace tr;
        const bool ok = g_UtilTraceShape(g_pGameTraceManager, &ray, start, end, &filter, &tr);
        FillResult(tr, res);
        return ok;
    }

    bool TraceShapeEx(Vector* start, Vector* end, CTraceFilter* filter, Ray_t* ray,
                      raytrace::TraceResult* res) override
    {
        if (!g_UtilTraceShape || !g_pGameTraceManager || !res) { ZeroResult(res); return false; }
        CGameTrace tr;
        const bool ok = g_UtilTraceShape(g_pGameTraceManager, ray, start, end, filter, &tr);
        FillResult(tr, res);
        return ok;
    }

  private:
    void ApplyOptions(CTraceFilter* f, CEntityInstance* ignore, const raytrace::TraceOptions* opts)
    {
        if (opts)
        {
            f->m_nInteractsWith = opts->interactsWith;
            f->m_nInteractsExclude = opts->interactsExclude;
        }
        if (ignore && ignore->m_pEntity)
            f->m_nEntityIdsToIgnore[0] = static_cast<uint32>(ignore->m_pEntity->GetEntityIndex().Get());
        // TODO: opts->drawBeam debug rendering (orig likely draws a beam when set).
    }
};

static RayTraceIface s_rayTraceIface;

class RayTracePlugin final : public ISmmPlugin, public IMetamodListener
{
  public:
    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late) override
    {
        PLUGIN_SAVEVARS();

        const std::string gdPath = ComputeGamedataPath();
        if (gdPath.empty())
        {
            std::snprintf(error, maxlen, "RayTraceRemastered: cannot compute gamedata.json path");
            return false;
        }
        nlohmann::json gd;
        if (!raytrace::sig::LoadGamedata(gdPath.c_str(), gd))
        {
            std::snprintf(error, maxlen, "RayTraceRemastered: failed to load %s", gdPath.c_str());
            return false;
        }
        // NOTE: metamod ships a 300KB libserver.so shim under addons/ — the
        // scanner excludes addon paths so this basename hits the real game lib.
        raytrace::sig::ModuleInfo server = raytrace::sig::ModuleFromName("libserver.so");
        if (!server)
        {
            std::snprintf(error, maxlen, "RayTraceRemastered: libserver.so module not found");
            return false;
        }

        char e[128] = {};
        void* utilSig = raytrace::sig::ResolveSig(gd, server, "UTIL_TraceShape", e, sizeof e);
        if (!utilSig)
        {
            std::snprintf(error, maxlen, "RayTraceRemastered: UTIL_TraceShape: %s", e);
            return false;
        }
        void* gtmSig = raytrace::sig::ResolveSig(gd, server, "GetGameTraceManager", e, sizeof e);
        if (!gtmSig)
        {
            std::snprintf(error, maxlen, "RayTraceRemastered: GetGameTraceManager: %s", e);
            return false;
        }

        g_UtilTraceShape = reinterpret_cast<UTILTraceShapeFn>(utilSig);
        // GetGameTraceManager sig lands on "mov rdi, [rip+X]" (len 7, disp @+3)
        void* gtmStorage = RipTarget(gtmSig, 7, 3);
        g_pGameTraceManager = *reinterpret_cast<void**>(gtmStorage);
        if (!g_pGameTraceManager)
        {
            std::snprintf(error, maxlen, "RayTraceRemastered: GameTraceManager global is null");
            return false;
        }

        ismm->AddListener(this, this);
        Warning("[RayTraceRemastered] loaded: gtm=%p util=%p\n", g_pGameTraceManager, utilSig);
        return true;
    }

    bool Unload(char* /*error*/, size_t /*maxlen*/) override { return true; }

    void* OnMetamodQuery(const char* iface, int* ret) override
    {
        if (!iface) return nullptr;
        if (std::strcmp(iface, "CRayTraceInterface002") == 0)
        {
            if (ret) *ret = META_IFACE_OK;
            return static_cast<CRayTraceInterface002*>(&s_rayTraceIface);
        }
        return nullptr;
    }

    const char* GetAuthor() override { return "xiaoyueyoqwq (remaster)"; }
    const char* GetName() override { return "RayTrace"; }
    const char* GetDescription() override { return "CRayTraceInterface002 remaster"; }
    const char* GetURL() override { return ""; }
    const char* GetLicense() override { return "GPL-3.0"; }
    const char* GetVersion() override { return "v1.0.15-remaster"; }
    const char* GetDate() override { return __DATE__; }
    const char* GetLogTag() override { return "RAYTRACE"; }

  private:
    // addons/<name>/bin/<platform>/<lib> -> up 3 -> addons/<name>/gamedata.json
    std::string ComputeGamedataPath() const
    {
        Dl_info info{};
        auto addr = reinterpret_cast<void*>(const_cast<char*>(__FUNCTION__));
        if (!addr || dladdr(addr, &info) == 0 || !info.dli_fname) return {};
        std::string p(info.dli_fname);
        for (int i = 0; i < 3; ++i)
        {
            auto pos = p.find_last_of('/');
            if (pos == std::string::npos) return {};
            p.erase(pos);
        }
        return p + "/gamedata.json";
    }
};

RayTracePlugin g_RayTracePlugin;
PLUGIN_EXPOSE(RayTracePlugin, g_RayTracePlugin);
