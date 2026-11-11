using System;
using System.Diagnostics;
using System.Numerics;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security;
using System.Security.Permissions;
using CounterStrikeSharp.API.Core;
using CounterStrikeSharp.API.Modules.Utils;

[assembly: CompilationRelaxations(8)]
[assembly: RuntimeCompatibility(WrapNonExceptionThrows = true)]
[assembly: Debuggable(DebuggableAttribute.DebuggingModes.IgnoreSymbolStoreSequencePoints)]
[assembly: TargetFramework(".NETCoreApp,Version=v10.0", FrameworkDisplayName = ".NET 10.0")]
[assembly: AssemblyCompany("RayTraceApi")]
[assembly: AssemblyConfiguration("Release")]
[assembly: AssemblyFileVersion("1.0.0.0")]
[assembly: AssemblyInformationalVersion("1.0.0+e136f9af2d2cecf431ace26a5f2d02c3ea8d6d9f")]
[assembly: AssemblyProduct("RayTraceApi")]
[assembly: AssemblyTitle("RayTraceApi")]
[assembly: SecurityPermission(SecurityAction.RequestMinimum, SkipVerification = true)]
[assembly: AssemblyVersion("1.0.0.0")]
[module: UnverifiableCode]
[module: RefSafetyRules(11)]
namespace RayTraceAPI;

[Flags]
public enum InteractionLayers : ulong
{
	None = 0uL,
	Solid = 1uL,
	Hitboxes = 2uL,
	Trigger = 4uL,
	Sky = 8uL,
	PlayerClip = 0x10uL,
	NPCClip = 0x20uL,
	BlockLOS = 0x40uL,
	BlockLight = 0x80uL,
	Ladder = 0x100uL,
	Pickup = 0x200uL,
	BlockSound = 0x400uL,
	NoDraw = 0x800uL,
	Window = 0x1000uL,
	PassBullets = 0x2000uL,
	WorldGeometry = 0x4000uL,
	Water = 0x8000uL,
	Slime = 0x10000uL,
	TouchAll = 0x20000uL,
	Player = 0x40000uL,
	NPC = 0x80000uL,
	Debris = 0x100000uL,
	Physics_Prop = 0x200000uL,
	NavIgnore = 0x400000uL,
	NavLocalIgnore = 0x800000uL,
	PostProcessingVolume = 0x1000000uL,
	UnusedLayer3 = 0x2000000uL,
	CarriedObject = 0x4000000uL,
	PushAway = 0x8000000uL,
	ServerEntityOnClient = 0x10000000uL,
	CarriedWeapon = 0x20000000uL,
	StaticLevel = 0x40000000uL,
	csgo_team1 = 0x80000000uL,
	csgo_team2 = 0x100000000uL,
	csgo_grenadeclip = 0x200000000uL,
	csgo_droneclip = 0x400000000uL,
	csgo_moveable = 0x800000000uL,
	csgo_opaque = 0x1000000000uL,
	csgo_monster = 0x2000000000uL,
	csgo_thrown_grenade = 0x8000000000uL,
	MASK_SHOT_PHYSICS = Solid | PlayerClip | Window | PassBullets | Player | NPC | Physics_Prop,
	MASK_SHOT_HITBOX = Hitboxes | Player | NPC,
	MASK_SHOT_FULL = MASK_SHOT_PHYSICS | Hitboxes,
	MASK_WORLD_ONLY = 0x3001uL,
	MASK_GRENADE = 0x203001uL,
	MASK_BRUSH_ONLY = 0x1001uL,
	MASK_PLAYER_MOVE = 0x3011uL,
	MASK_NPC_MOVE = MASK_WORLD_ONLY | NPCClip
}
[StructLayout(LayoutKind.Explicit, Size = 24)]
public struct TraceOptions
{
	[FieldOffset(0)]
	public ulong InteractsWith;

	[FieldOffset(8)]
	public ulong InteractsExclude;

	[FieldOffset(16)]
	public int DrawBeam;

	public TraceOptions()
	{
		InteractsWith = 2895889uL;
		InteractsExclude = 0uL;
		DrawBeam = 0;
	}

	public TraceOptions(InteractionLayers interactsWith, InteractionLayers interactsExclude = InteractionLayers.None, bool drawBeam = false)
	{
		InteractsWith = (ulong)interactsWith;
		InteractsExclude = (ulong)interactsExclude;
		DrawBeam = (drawBeam ? 1 : 0);
	}
}
[StructLayout(LayoutKind.Explicit, Size = 44)]
public struct TraceResult
{
	[FieldOffset(0)]
	public float EndPosX;

	[FieldOffset(4)]
	public float EndPosY;

	[FieldOffset(8)]
	public float EndPosZ;

	[FieldOffset(16)]
	public nint HitEntity;

	[FieldOffset(24)]
	public float Fraction;

	[FieldOffset(28)]
	public int AllSolid;

	[FieldOffset(32)]
	public float NormalX;

	[FieldOffset(36)]
	public float NormalY;

	[FieldOffset(40)]
	public float NormalZ;

	public Vector3 EndPos => new Vector3(EndPosX, EndPosY, EndPosZ);

	public Vector3 Normal => new Vector3(NormalX, NormalY, NormalZ);

	public bool DidHit => Fraction < 1f;

	public bool IsAllSolid => AllSolid != 0;
}
public interface CRayTraceInterface
{
	bool TraceShape(Vector start, QAngle angles, CEntityInstance? ignore, TraceOptions options, out TraceResult result);

	bool TraceEndShape(Vector start, Vector end, CEntityInstance? ignore, TraceOptions options, out TraceResult result);

	bool TraceHullShape(Vector start, Vector end, Vector mins, Vector maxs, CEntityInstance? ignore, TraceOptions options, out TraceResult result);

	bool TraceShapeEx(Vector start, Vector end, nint filter, nint ray, out TraceResult result);
}
