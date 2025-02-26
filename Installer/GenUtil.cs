using Microsoft.Win32;
using RS2014_Mod_Installer;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace RSMods.Util
{
    public static class GenUtil
    {
        public static bool IsDirectoryEmpty(string path)
        {
            return !Directory.EnumerateFileSystemEntries(path).Any();
        }

        public static void ExtractEmbeddedResource(string outputDir, Assembly resourceAssembly, string resourceLocation, string[] files)
        {
            if (!Directory.Exists(outputDir))
                Directory.CreateDirectory(outputDir);

            string resourcePath;
            foreach (string file in files)
            {
                resourcePath = Path.Combine(outputDir, file);

                Stream stream = resourceAssembly.GetManifestResourceStream(string.Format("{0}.{1}", resourceLocation, file));

                if (stream == null)
                    return;

                using (FileStream fileStream = new FileStream(resourcePath, FileMode.Create))
                    stream.CopyTo(fileStream);
            }
        }

        private static bool IsRSFolder(this string folderPath)
        {
            if (!Directory.Exists(folderPath))
                return false;

            string dlcFolderPath = Path.Combine(folderPath, "dlc");
            string cachePsarcPath = Path.Combine(folderPath, "cache.psarc");

            if (!Directory.Exists(dlcFolderPath) || IsDirectoryEmpty(dlcFolderPath) || !File.Exists(cachePsarcPath))
                return false;

            return true;
        }

        private static string GetStringValueFromRegistry(string keyName, string valueName)
        {
            try
            {
                var retValue = (string)Registry.GetValue(keyName, valueName, "");
                return retValue ?? string.Empty;
            }
            catch (Exception)
            {
                return string.Empty;
            }
        }

        private static List<string> GetCustomSteamappsFolders(string mainSteamPath)
        {
            string libRegex = "(^\\t\"[1-9]\").*(\".*\")";
            var libDirs = new List<string>();

            string steamappsFolder = Path.Combine(mainSteamPath, "steamapps");
            string libVdf = Path.Combine(steamappsFolder, "libraryfolders.vdf");

            if (!File.Exists(libVdf))
                return new List<string>();

            foreach (string l in File.ReadAllLines(libVdf))
            {
                var reg = Regex.Match(l, libRegex);

                if (reg.Success)
                {
                    string dir = reg.Groups[2].Value;

                    if (dir != string.Empty)
                    {
                        string ndir = dir.Trim('\"');
                        libDirs.Add(ndir);
                    }
                }
            }

            if (libDirs.Count == 0)
                return new List<string>();

            return libDirs;
        }


        private static string GetCustomRSFolder(string mainSteamPath)
        {
            var customSteamappsFolders = GetCustomSteamappsFolders(mainSteamPath);
            string rsFolderPath = string.Empty;

            if (customSteamappsFolders == null || customSteamappsFolders.Count == 0)
                return string.Empty;

            foreach (var customSteamappsFolder in customSteamappsFolders)
            {
                string dirPath = Path.Combine(customSteamappsFolder, "steamapps", "appmanifest_221680.acf");

                if (!File.Exists(dirPath))
                    continue;

                string finalPath = Path.GetDirectoryName(dirPath);
                if (string.IsNullOrEmpty(finalPath))
                    continue;

                rsFolderPath = Path.Combine(finalPath, "common", "Rocksmith2014");

                if (rsFolderPath.IsRSFolder())
                    return rsFolderPath;
            }
           
            return string.Empty;
        }

        public static string GetSteamDirectory()
        {
            const string steamRegPath = @"HKEY_CURRENT_USER\SOFTWARE\Valve\Steam"; //IIRC it isn't the same on X86 machines, but do we really need to support those?

            return GetStringValueFromRegistry(steamRegPath, "SteamPath").Replace('/', '\\');
        }

        public static List<Tuple<string, string>> InstallRegKeys { get; set; } = new List<Tuple<string, string>>()
        {
            new Tuple<string, string>(@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Ubisoft\Rocksmith2014", "installdir"),
            new Tuple<string, string>(@"HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 221680", "InstallLocation"),
            new Tuple<string, string>(@"HKEY_LOCAL_MACHINE\SOFTWARE\Ubisoft\Rocksmith2014", "InstallLocation"),
            new Tuple<string, string>(@"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 221680", "InstallLocation")
        };

        public static string GetRSDirectory()
        {
            try
            {
                var rs2RootDir = String.Empty;
                var steamRootPath = GetSteamDirectory();

                if (!string.IsNullOrEmpty(steamRootPath))
                {
                    rs2RootDir = Path.Combine(steamRootPath, "steamapps\\common\\Rocksmith2014");

                    if (!Directory.Exists(rs2RootDir)) // RS-Folder doesn't exist
                    {
                        // Go through each possible registry location
                        foreach (var installRegKey in InstallRegKeys)
                        {
                            var path = GetStringValueFromRegistry(installRegKey.Item1, installRegKey.Item2);

                            if (!string.IsNullOrEmpty(path) && Directory.Exists(path))
                            {
                                rs2RootDir = path;
                                break;
                            }
                        }

                        if (string.IsNullOrEmpty(rs2RootDir))
                        {
                            rs2RootDir = GetCustomRSFolder(steamRootPath); // Grab custom Steam library paths from .vdf file
                        }

                        if (string.IsNullOrEmpty(rs2RootDir) || !rs2RootDir.IsRSFolder()) // If neither that's OK, ask the user to point the GUI to the correct location
                        {
                            MessageBox.Show("We were unable to detect your Rocksmith 2014 folder, please select it manually!", "Your help is required!");
                            return AskUserForRSFolder();
                        }
                    }
                    else // RS-Folder does exist
                    {
                        if (!File.Exists(Path.Combine(rs2RootDir, "cache.psarc"))) // If cache.psarc doesn't exist (old install / steam left-overs)
                        {
                            MessageBox.Show("cache.psarc not found in the folder. Please tell us where the Rocksmith 2014 folder is located.", "Error: cache.psarc not found");
                            rs2RootDir = AskUserForRSFolder();
                            
                            if (rs2RootDir?.Length == 0)
                            {
                                MessageBox.Show("We were unable to detect your Rocksmith 2014 folder, and you didn't give us a valid RS Folder.", "Closing Application");
                                Application.Exit();
                                return string.Empty;
                            }
                        }
                    }
                }

                return rs2RootDir;
            }
            catch (Exception ex)
            {
                MessageBox.Show("<Warning> GetRSDirectory failed, " + ex.Message);
            }

            return string.Empty;
        }

        public static string AskUserForRSFolder()
        {
            FolderPicker dialog = new FolderPicker();

            IntPtr ownerHandle = Application.OpenForms.Count > 0 ? Application.OpenForms[0].Handle : IntPtr.Zero;
            if (dialog.ShowDialog(ownerHandle) == true)
            {
                string rsFolder = dialog.ResultPath;

                if (!string.IsNullOrEmpty(rsFolder) && rsFolder.IsRSFolder())
                    return rsFolder;
            }

            return string.Empty;
        }
    }
}