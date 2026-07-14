// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TwoLeftStanding : ModuleRules
{
	public TwoLeftStanding(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
        });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"TwoLeftStanding",
			"TwoLeftStanding/Variant_Strategy",
			"TwoLeftStanding/Variant_Strategy/UI",
			"TwoLeftStanding/Variant_TwinStick",
			"TwoLeftStanding/Variant_TwinStick/AI",
			"TwoLeftStanding/Variant_TwinStick/Gameplay",
			"TwoLeftStanding/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
        PrivateDependencyModuleNames.Add("OnlineSubsystemSteam");
        PrivateDependencyModuleNames.Add("SteamSockets");
    }
}
