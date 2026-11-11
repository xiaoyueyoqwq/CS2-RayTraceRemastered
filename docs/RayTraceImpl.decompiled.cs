using System;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security;
using System.Security.Permissions;
using CounterStrikeSharp.API;
using CounterStrikeSharp.API.Core;
using CounterStrikeSharp.API.Core.Capabilities;
using CounterStrikeSharp.API.Modules.Memory;
using CounterStrikeSharp.API.Modules.Utils;
using RayTraceAPI;

[assembly: CompilationRelaxations(8)]
[assembly: RuntimeCompatibility(WrapNonExceptionThrows = true)]
[assembly: Debuggable(DebuggableAttribute.DebuggingModes.IgnoreSymbolStoreSequencePoints)]
[assembly: TargetFramework(".NETCoreApp,Version=v10.0", FrameworkDisplayName = ".NET 10.0")]
[assembly: AssemblyCompany("RayTraceImpl")]
[assembly: AssemblyConfiguration("Release")]
[assembly: AssemblyFileVersion("1.0.0.0")]
[assembly: AssemblyInformationalVersion("1.0.0+e136f9af2d2cecf431ace26a5f2d02c3ea8d6d9f")]
[assembly: AssemblyProduct("RayTraceImpl")]
[assembly: AssemblyTitle("RayTraceImpl")]
[assembly: SecurityPermission(SecurityAction.RequestMinimum, SkipVerification = true)]
[assembly: AssemblyVersion("1.0.0.0")]
[module: UnverifiableCode]
[module: RefSafetyRules(11)]
namespace RayTraceImpl;

public class RayTraceImpl : BasePlugin
{
	public override string ModuleName => "RayTraceImpl";

	public override string ModuleVersion => "v1.0.0";

	public override string ModuleAuthor => "Slynx";

	internal static PluginCapability<CRayTraceInterface> RayTraceInterface { get; } = new PluginCapability<CRayTraceInterface>("raytrace:craytraceinterface");

	public override void Load(bool hotReload)
	{
		//IL_0031: Unknown result type (might be due to invalid IL or missing references)
		//IL_003b: Expected O, but got Unknown
		Capabilities.RegisterPluginCapability<CRayTraceInterface>(RayTraceInterface, (Func<CRayTraceInterface>)(() => new CRayTrace()));
		((BasePlugin)this).RegisterListener<OnMetamodAllPluginsLoaded>(new OnMetamodAllPluginsLoaded(OnMetamodAllPluginsLoaded));
	}

	public override void Unload(bool hotReload)
	{
		//IL_0008: Unknown result type (might be due to invalid IL or missing references)
		//IL_0012: Expected O, but got Unknown
		((BasePlugin)this).RemoveListener<OnMetamodAllPluginsLoaded>(new OnMetamodAllPluginsLoaded(OnMetamodAllPluginsLoaded));
	}

	private void OnMetamodAllPluginsLoaded()
	{
		if (!NativeBridge.Initialize())
		{
			Prints.ServerLog("[RayTraceImpl] Native bridge initialization failed.", ConsoleColor.Red);
		}
		else
		{
			Prints.ServerLog("[RayTraceImpl] Managed side initialized.", ConsoleColor.Green);
		}
	}
}
public class CRayTrace : CRayTraceInterface
{
	public unsafe bool TraceShape(Vector start, QAngle angles, CEntityInstance? ignore, TraceOptions options, out TraceResult result)
	{
		result = default(TraceResult);
		if (!NativeBridge.Loaded)
		{
			return false;
		}
		TraceResult traceResult = default(TraceResult);
		TraceOptions traceOptions = options;
		bool result2 = NativeBridge.TraceShape(NativeBridge.Handle, ((NativeObject)start).Handle, ((NativeObject)angles).Handle, (ignore != null) ? ((nint)((NativeEntity)ignore).Handle) : IntPtr.Zero, (nint)(&traceOptions), (nint)(&traceResult));
		result = traceResult;
		return result2;
	}

	public unsafe bool TraceEndShape(Vector start, Vector end, CEntityInstance? ignore, TraceOptions options, out TraceResult result)
	{
		result = default(TraceResult);
		if (!NativeBridge.Loaded)
		{
			return false;
		}
		TraceResult traceResult = default(TraceResult);
		TraceOptions traceOptions = options;
		bool result2 = NativeBridge.TraceEndShape(NativeBridge.Handle, ((NativeObject)start).Handle, ((NativeObject)end).Handle, (ignore != null) ? ((nint)((NativeEntity)ignore).Handle) : IntPtr.Zero, (nint)(&traceOptions), (nint)(&traceResult));
		result = traceResult;
		return result2;
	}

	public unsafe bool TraceHullShape(Vector start, Vector end, Vector mins, Vector maxs, CEntityInstance? ignore, TraceOptions options, out TraceResult result)
	{
		result = default(TraceResult);
		if (!NativeBridge.Loaded)
		{
			return false;
		}
		TraceResult traceResult = default(TraceResult);
		TraceOptions traceOptions = options;
		bool result2 = NativeBridge.TraceHullShape(NativeBridge.Handle, ((NativeObject)start).Handle, ((NativeObject)end).Handle, ((NativeObject)mins).Handle, ((NativeObject)maxs).Handle, (ignore != null) ? ((nint)((NativeEntity)ignore).Handle) : IntPtr.Zero, (nint)(&traceOptions), (nint)(&traceResult));
		result = traceResult;
		return result2;
	}

	public unsafe bool TraceShapeEx(Vector start, Vector end, nint filter, nint ray, out TraceResult result)
	{
		result = default(TraceResult);
		if (!NativeBridge.Loaded)
		{
			return false;
		}
		TraceResult traceResult = default(TraceResult);
		bool result2 = NativeBridge.TraceShapeEx(NativeBridge.Handle, ((NativeObject)start).Handle, ((NativeObject)end).Handle, filter, ray, (nint)(&traceResult));
		result = traceResult;
		return result2;
	}
}
public static class NativeBridge
{
	public static nint Handle;

	public static bool Loaded;

	public static Func<nint, nint, nint, nint, nint, nint, bool>? TraceShape;

	public static Func<nint, nint, nint, nint, nint, nint, bool>? TraceEndShape;

	public static Func<nint, nint, nint, nint, nint, nint, nint, nint, bool>? TraceHullShape;

	public static Func<nint, nint, nint, nint, nint, nint, bool>? TraceShapeEx;

	public static bool Initialize()
	{
		Handle = ((nint?)Utilities.MetaFactory("CRayTraceInterface002")).Value;
		if (Handle == 0)
		{
			return false;
		}
		Bind();
		Loaded = true;
		return true;
	}

	private static void Bind()
	{
		int num;
		int num2;
		int num3;
		int num4;
		if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
		{
			num = 1;
			num2 = 2;
			num3 = 3;
			num4 = 4;
		}
		else
		{
			num = 2;
			num2 = 3;
			num3 = 4;
			num4 = 5;
		}
		TraceShape = VirtualFunction.Create<nint, nint, nint, nint, nint, nint, bool>((IntPtr)Handle, num);
		TraceEndShape = VirtualFunction.Create<nint, nint, nint, nint, nint, nint, bool>((IntPtr)Handle, num2);
		TraceHullShape = VirtualFunction.Create<nint, nint, nint, nint, nint, nint, nint, nint, bool>((IntPtr)Handle, num3);
		TraceShapeEx = VirtualFunction.Create<nint, nint, nint, nint, nint, nint, bool>((IntPtr)Handle, num4);
	}
}
public static class Prints
{
	public static void ServerLog(string msg, ConsoleColor color = ConsoleColor.White)
	{
		Console.ForegroundColor = color;
		Console.WriteLine(msg);
		Console.ResetColor();
	}
}
