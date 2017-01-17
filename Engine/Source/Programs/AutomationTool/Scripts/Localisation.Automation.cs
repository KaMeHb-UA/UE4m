﻿// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;
using EpicGames.Localization;

[Help("Updates the external localization data using the arguments provided.")]
[Help("UEProjectRoot", "Optional root-path to the project we're gathering for (defaults to CmdEnv.LocalRoot if unset).")]
[Help("UEProjectDirectory", "Sub-path to the project we're gathering for (relative to UEProjectRoot).")]
[Help("UEProjectName", "Optional name of the project we're gathering for (should match its .uproject file, eg QAGame).")]
[Help("LocalizationProjectNames", "Comma separated list of the projects to gather text from.")]
[Help("LocalizationBranch", "Optional suffix to use when uploading the new data to the localization provider.")]
[Help("LocalizationProvider", "Optional localization provide override (default is OneSky).")]
[Help("LocalizationSteps", "Optional comma separated list of localization steps to perform [Download, Gather, Import, Export, Compile, GenerateReports, Upload] (default is all). Only valid for projects using a modular config.")]
[Help("IncludePlugins", "Optional flag to include plugins from within the given UEProjectDirectory as part of the gather. This may optionally specify a comma separated list of the specific plugins to gather (otherwise all plugins will be gathered).")]
[Help("ExcludePlugins", "Optional comma separated list of plugins to exclude from the gather.")]
[Help("AdditionalCommandletArguments", "Optional arguments to pass to the gather process.")]
class Localise : BuildCommand
{
	private struct LocalizationBatch
	{
		public LocalizationBatch(string InUEProjectDirectory, string InRemoteFilenamePrefix, List<string> InLocalizationProjectNames)
		{
			UEProjectDirectory = InUEProjectDirectory;
			RemoteFilenamePrefix = InRemoteFilenamePrefix;
			LocalizationProjectNames = InLocalizationProjectNames;
		}

		public string UEProjectDirectory;
		public string RemoteFilenamePrefix;
		public List<string> LocalizationProjectNames;
	};

	public override void ExecuteBuild()
	{
		var UEProjectRoot = ParseParamValue("UEProjectRoot");
		if (UEProjectRoot == null)
		{
			UEProjectRoot = CmdEnv.LocalRoot;
		}

		var UEProjectDirectory = ParseParamValue("UEProjectDirectory");
		if (UEProjectDirectory == null)
		{
			throw new AutomationException("Missing required command line argument: 'UEProjectDirectory'");
		}

		var UEProjectName = ParseParamValue("UEProjectName");
		if (UEProjectName == null)
		{
			UEProjectName = "";
		}

		var LocalizationProjectNames = new List<string>();
		{
			var LocalizationProjectNamesStr = ParseParamValue("LocalizationProjectNames");
			if (LocalizationProjectNamesStr != null)
			{
				foreach (var ProjectName in LocalizationProjectNamesStr.Split(','))
				{
					LocalizationProjectNames.Add(ProjectName.Trim());
				}
			}
		}

		var LocalizationProviderName = ParseParamValue("LocalizationProvider");
		if (LocalizationProviderName == null)
		{
			LocalizationProviderName = "OneSky";
		}

		var LocalizationSteps = new List<string>();
		{
			var LocalizationStepsStr = ParseParamValue("LocalizationSteps");
			if (LocalizationStepsStr == null)
			{
				LocalizationSteps.AddRange(new string[] { "Download", "Gather", "Import", "Export", "Compile", "GenerateReports", "Upload" });
			}
			else
			{
				foreach (var StepName in LocalizationStepsStr.Split(','))
				{
					LocalizationSteps.Add(StepName.Trim());
				}
			}
			LocalizationSteps.Add("Monolithic"); // Always allow the monolithic scripts to run as we don't know which steps they do
		}

		var ShouldGatherPlugins = ParseParam("IncludePlugins");
		var IncludePlugins = new List<string>();
		var ExcludePlugins = new List<string>();
		if (ShouldGatherPlugins)
		{
			var IncludePluginsStr = ParseParamValue("IncludePlugins");
			if (IncludePluginsStr != null)
			{
				foreach (var PluginName in IncludePluginsStr.Split(','))
				{
					IncludePlugins.Add(PluginName.Trim());
				}
			}

			var ExcludePluginsStr = ParseParamValue("ExcludePlugins");
			if (ExcludePluginsStr != null)
			{
				foreach (var PluginName in ExcludePluginsStr.Split(','))
				{
					ExcludePlugins.Add(PluginName.Trim());
				}
			}
		}

		var AdditionalCommandletArguments = ParseParamValue("AdditionalCommandletArguments");
		if (AdditionalCommandletArguments == null)
		{
			AdditionalCommandletArguments = "";
		}

		var LocalizationBatches = new List<LocalizationBatch>();

		// Add the static set of localization projects as a batch
		if (LocalizationProjectNames.Count > 0)
		{
			LocalizationBatches.Add(new LocalizationBatch(UEProjectDirectory, "", LocalizationProjectNames));
		}

		// Build up any additional batches needed for plugins
		if (ShouldGatherPlugins)
		{
			var PluginsRootDirectory = CombinePaths(UEProjectRoot, UEProjectDirectory, "Plugins");
			IReadOnlyList<PluginInfo> AllPlugins = Plugins.ReadPluginsFromDirectory(new DirectoryReference(PluginsRootDirectory), UEProjectName.Length == 0 ? PluginLoadedFrom.Engine : PluginLoadedFrom.GameProject);

			// Add a batch for each plugin that meets our criteria
			foreach (var PluginInfo in AllPlugins)
			{
				bool ShouldIncludePlugin = (IncludePlugins.Count == 0 || IncludePlugins.Contains(PluginInfo.Name)) && !ExcludePlugins.Contains(PluginInfo.Name);
				if (ShouldIncludePlugin && PluginInfo.Descriptor.LocalizationTargets != null && PluginInfo.Descriptor.LocalizationTargets.Length > 0)
				{
					var RootRelativePluginPath = PluginInfo.Directory.MakeRelativeTo(new DirectoryReference(UEProjectRoot));
					RootRelativePluginPath = RootRelativePluginPath.Replace('\\', '/'); // Make sure we use / as these paths are used with P4

					var PluginTargetNames = new List<string>();
					foreach (var LocalizationTarget in PluginInfo.Descriptor.LocalizationTargets)
					{
						PluginTargetNames.Add(LocalizationTarget.Name);
					}

					LocalizationBatches.Add(new LocalizationBatch(RootRelativePluginPath, PluginInfo.Name, PluginTargetNames));
				}
			}
		}

		// Process each localization batch
		foreach (var LocalizationBatch in LocalizationBatches)
		{
			ProcessLocalizationProjects(LocalizationBatch, UEProjectRoot, UEProjectName, LocalizationProviderName, LocalizationSteps, AdditionalCommandletArguments);
		}
	}

	private void ProcessLocalizationProjects(LocalizationBatch LocalizationBatch, string UEProjectRoot, string UEProjectName, string LocalizationProviderName, List<string> LocalizationSteps, string AdditionalCommandletArguments)
	{
		var EditorExe = CombinePaths(CmdEnv.LocalRoot, @"Engine/Binaries/Win64/UE4Editor-Cmd.exe");
		var RootWorkingDirectory = CombinePaths(UEProjectRoot, LocalizationBatch.UEProjectDirectory);

		// Try and find our localization provider
		LocalizationProvider LocProvider = null;
		{
			LocalizationProvider.LocalizationProviderArgs LocProviderArgs;
			LocProviderArgs.RootWorkingDirectory = RootWorkingDirectory;
			LocProviderArgs.RemoteFilenamePrefix = LocalizationBatch.RemoteFilenamePrefix;
			LocProviderArgs.CommandUtils = this;
			LocProvider = LocalizationProvider.GetLocalizationProvider(LocalizationProviderName, LocProviderArgs);
		}

		// Make sure the Localization configs and content is up-to-date to ensure we don't get errors later on
		if (P4Enabled)
		{
			Log("Sync necessary content to head revision");
			P4.Sync(P4Env.BuildRootP4 + "/" + LocalizationBatch.UEProjectDirectory + "/Config/Localization/...");
			P4.Sync(P4Env.BuildRootP4 + "/" + LocalizationBatch.UEProjectDirectory + "/Content/Localization/...");
		}

		// Generate the info we need to gather for each project
		var ProjectInfos = new List<ProjectInfo>();
		foreach (var ProjectName in LocalizationBatch.LocalizationProjectNames)
		{
			ProjectInfos.Add(GenerateProjectInfo(RootWorkingDirectory, ProjectName, LocalizationSteps));
		}

		if (LocalizationSteps.Contains("Download") && LocProvider != null)
		{
			// Export all text from our localization provider
			foreach (var ProjectInfo in ProjectInfos)
			{
				LocProvider.DownloadProjectFromLocalizationProvider(ProjectInfo.ProjectName, ProjectInfo.ImportInfo);
			}
		}

		// Setup editor arguments for SCC.
		string EditorArguments = String.Empty;
		if (P4Enabled)
		{
			EditorArguments = String.Format("-SCCProvider={0} -P4Port={1} -P4User={2} -P4Client={3} -P4Passwd={4}", "Perforce", P4Env.P4Port, P4Env.User, P4Env.Client, P4.GetAuthenticationToken());
		}
		else
		{
			EditorArguments = String.Format("-SCCProvider={0}", "None");
		}
		EditorArguments += " -Unattended";

		// Setup commandlet arguments for SCC.
		string CommandletSCCArguments = String.Empty;
		if (P4Enabled) { CommandletSCCArguments += (String.IsNullOrEmpty(CommandletSCCArguments) ? "" : " ") + "-EnableSCC"; }
		if (!AllowSubmit) { CommandletSCCArguments += (String.IsNullOrEmpty(CommandletSCCArguments) ? "" : " ") + "-DisableSCCSubmit"; }

		// Execute commandlet for each config in each project.
		bool bLocCommandletFailed = false;
		foreach (var ProjectInfo in ProjectInfos)
		{
			foreach (var LocalizationStep in ProjectInfo.LocalizationSteps)
			{
				if (!LocalizationSteps.Contains(LocalizationStep.Name))
				{
					continue;
				}

				var CommandletArguments = String.Format("-config=\"{0}\"", LocalizationStep.LocalizationConfigFile) + (String.IsNullOrEmpty(CommandletSCCArguments) ? "" : " " + CommandletSCCArguments);

				if (!String.IsNullOrEmpty(AdditionalCommandletArguments))
				{
					CommandletArguments += " " + AdditionalCommandletArguments;
				}

				string Arguments = String.Format("{0} -run=GatherText {1} {2}", UEProjectName, EditorArguments, CommandletArguments);
				Log("Running localization commandlet: {0}", Arguments);
				var StartTime = DateTime.UtcNow;
				var RunResult = Run(EditorExe, Arguments, null, ERunOptions.Default | ERunOptions.NoLoggingOfRunCommand); // Disable logging of the run command as it will print the exit code which GUBP can pick up as an error (we do that ourselves below)
				var RunDuration = (DateTime.UtcNow - StartTime).TotalMilliseconds;
				Log("Localization commandlet finished in {0}s", RunDuration / 1000);

				if (RunResult.ExitCode != 0)
				{
					LogWarning("The localization commandlet exited with code {0} which likely indicates a crash. It ran with the following arguments: '{1}'", RunResult.ExitCode, Arguments);
					bLocCommandletFailed = true;
					break; // We failed a step, so don't process any other steps in this config chain
				}
			}
		}

		if (LocalizationSteps.Contains("Upload") && LocProvider != null)
		{
			if (bLocCommandletFailed)
			{
				LogWarning("Skipping upload to the localization provider due to an earlier commandlet failure.");
			}
			else
			{
				// Upload all text to our localization provider
				foreach (var ProjectInfo in ProjectInfos)
				{
					LocProvider.UploadProjectToLocalizationProvider(ProjectInfo.ProjectName, ProjectInfo.ExportInfo);
				}
			}
		}
	}

	private ProjectInfo GenerateProjectInfo(string RootWorkingDirectory, string ProjectName, List<string> LocalizationSteps)
	{
		var ProjectInfo = new ProjectInfo();

		ProjectInfo.ProjectName = ProjectName;
		ProjectInfo.LocalizationSteps = new List<ProjectStepInfo>();

		// Projects generated by the localization dashboard will use multiple config files that must be run in a specific order
		// Older projects (such as the Engine) would use a single config file containing all the steps
		// Work out which kind of project we're dealing with...
		var MonolithicConfigFile = CombinePaths(RootWorkingDirectory, String.Format(@"Config/Localization/{0}.ini", ProjectName));
		if (File.Exists(MonolithicConfigFile))
		{
			ProjectInfo.LocalizationSteps.Add(new ProjectStepInfo("Monolithic", MonolithicConfigFile));

			ProjectInfo.ImportInfo = GenerateProjectImportExportInfo(MonolithicConfigFile);
			ProjectInfo.ExportInfo = ProjectInfo.ImportInfo;
		}
		else
		{
			var FileSuffixes = new[] { 
				new { Suffix = "Gather", Required = LocalizationSteps.Contains("Gather") }, 
				new { Suffix = "Import", Required = LocalizationSteps.Contains("Import") || LocalizationSteps.Contains("Download") },	// Downloading needs the parsed ImportInfo
				new { Suffix = "Export", Required = LocalizationSteps.Contains("Gather") || LocalizationSteps.Contains("Upload")},		// Uploading needs the parsed ExportInfo
				new { Suffix = "Compile", Required = LocalizationSteps.Contains("Compile") }, 
				new { Suffix = "GenerateReports", Required = false } 
			};

			foreach (var FileSuffix in FileSuffixes)
			{
				var ModularConfigFile = CombinePaths(RootWorkingDirectory, String.Format(@"Config/Localization/{0}_{1}.ini", ProjectName, FileSuffix.Suffix));

				if (File.Exists(ModularConfigFile))
				{
					ProjectInfo.LocalizationSteps.Add(new ProjectStepInfo(FileSuffix.Suffix, ModularConfigFile));

					if (FileSuffix.Suffix == "Import")
					{
						ProjectInfo.ImportInfo = GenerateProjectImportExportInfo(ModularConfigFile);
					}
					else if (FileSuffix.Suffix == "Export")
					{
						ProjectInfo.ExportInfo = GenerateProjectImportExportInfo(ModularConfigFile);
					}
				}
				else if (FileSuffix.Required)
				{
					throw new AutomationException("Failed to find a required config file! '{0}'", ModularConfigFile);
				}
			}
		}

		return ProjectInfo;
	}

	private ProjectImportExportInfo GenerateProjectImportExportInfo(string LocalizationConfigFile)
	{
		var ProjectImportExportInfo = new ProjectImportExportInfo();

		var LocalizationConfig = new ConfigCacheIni(new FileReference(LocalizationConfigFile));

		if (!LocalizationConfig.GetString("CommonSettings", "DestinationPath", out ProjectImportExportInfo.DestinationPath))
		{
			throw new AutomationException("Failed to find a required config key! Section: 'CommonSettings', Key: 'DestinationPath', File: '{0}'", LocalizationConfigFile);
		}

		if (!LocalizationConfig.GetString("CommonSettings", "ManifestName", out ProjectImportExportInfo.ManifestName))
		{
			throw new AutomationException("Failed to find a required config key! Section: 'CommonSettings', Key: 'ManifestName', File: '{0}'", LocalizationConfigFile);
		}

		if (!LocalizationConfig.GetString("CommonSettings", "ArchiveName", out ProjectImportExportInfo.ArchiveName))
		{
			throw new AutomationException("Failed to find a required config key! Section: 'CommonSettings', Key: 'ArchiveName', File: '{0}'", LocalizationConfigFile);
		}

		if (!LocalizationConfig.GetString("CommonSettings", "PortableObjectName", out ProjectImportExportInfo.PortableObjectName))
		{
			throw new AutomationException("Failed to find a required config key! Section: 'CommonSettings', Key: 'PortableObjectName', File: '{0}'", LocalizationConfigFile);
		}

		if (!LocalizationConfig.GetString("CommonSettings", "NativeCulture", out ProjectImportExportInfo.NativeCulture))
		{
			throw new AutomationException("Failed to find a required config key! Section: 'CommonSettings', Key: 'NativeCulture', File: '{0}'", LocalizationConfigFile);
		}

		if (!LocalizationConfig.GetArray("CommonSettings", "CulturesToGenerate", out ProjectImportExportInfo.CulturesToGenerate))
		{
			throw new AutomationException("Failed to find a required config key! Section: 'CommonSettings', Key: 'CulturesToGenerate', File: '{0}'", LocalizationConfigFile);
		}

		if (!LocalizationConfig.GetBool("CommonSettings", "bUseCultureDirectory", out ProjectImportExportInfo.bUseCultureDirectory))
		{
			// bUseCultureDirectory is optional, default is true
			ProjectImportExportInfo.bUseCultureDirectory = true;
		}

		return ProjectImportExportInfo;
	}
}