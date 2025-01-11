using System;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using System.Security.Cryptography.X509Certificates;

namespace RS2014_Mod_Installer
{
    public partial class GUI : Form
    {
        public GUI()
        {
            InitializeComponent();
        }

        private void UseModsButton_Click(object sender, EventArgs e)
        {
            string originalButtonText = UseModsButton.Text;
            UseModsButton.Text += "\n(Please wait as we get the mods setup).";
            // Can we find Rocksmith's install?
            if (Worker.WhereIsRocksmith() == string.Empty)
            {
                MessageBox.Show("It looks like your current Rocksmith2014 install folder cannot be found. Please tell us where it is located!", "Error: RSLocation Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                string newRSFolder = RSMods.Util.GenUtil.AskUserForRSFolder();
                if (newRSFolder == string.Empty)
                {
                    MessageBox.Show("We cannot detect where you have Rocksmith located. Please try reinstalling your game on Steam.", "Error: RSLocation Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    Environment.Exit(1);
                    return;
                }

                Worker.RSLocation = newRSFolder;
            }

            IsVoid(Worker.WhereIsRocksmith());

            // Get DLL from Installer
            if (DLLStuff.InjectDLL(Worker.WhereIsRocksmith()))
            {
                // Get GUI from Installer
                if (DLLStuff.InjectGUI(Worker.WhereIsRocksmith()))
                {
                    string rsModsPath = Path.Combine(Worker.WhereIsRocksmith(), "RSMods") + "\\RSMods.exe";
                    MessageBox.Show("This version of the installer allows you to take advantage of the new mod settings available by opening: " + rsModsPath, "New Mod Settings Available!", MessageBoxButtons.OK, MessageBoxIcon.Information);

                    Process.Start(rsModsPath);
                    CreateDesktopShortcut(rsModsPath);

                    this.Close();
                }
            }

            UseModsButton.Text = originalButtonText;
        }

        private void CreateDesktopShortcut(string rsModsPath)
        {
            DialogResult dialogResult = MessageBox.Show("Would you like to create RSMods shortcut on your desktop?", "Create shortcut", MessageBoxButtons.YesNo);
            if (dialogResult != DialogResult.Yes)
            {
                return;
            }

            try
            {
                string deskDir = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);

                using (StreamWriter writer = new StreamWriter(deskDir + @"\\RSMods.url", true))
                {
                    writer.WriteLine("[InternetShortcut]");
                    writer.WriteLine("URL=file:///" + rsModsPath);
                    writer.WriteLine("IconIndex=0");
                    string icon = rsModsPath.Replace('\\', '/');
                    writer.WriteLine("IconFile=" + icon);
                }
            }
            catch (IOException ex)
            {
                MessageBox.Show(ex.Message, "Error creating the shortcut");
            }
        }

        public static void IsVoid(string installLocation) // Anti-Piracy Check (False = Real, True = Pirated) || Modified from Beat Saber Mod Assistant
        {
            string reason = string.Empty;
            bool fakeSteamApi = true;
            try
            {
                X509Certificate2 cert = new X509Certificate2(X509Certificate.CreateFromSignedFile(Path.Combine(installLocation, "steam_api.dll")));

                if (cert.GetNameInfo(X509NameType.SimpleName, false) == "Valve" || cert.Verify())
                {
                    fakeSteamApi = false;
                }
                else
                {
                    reason += "Invalid steam_api.dll certificate.";
                }
            }
            catch { } // Fall-through = bad cert.

            bool areCrackIndicationsPresent = File.Exists(Path.Combine(installLocation, "IGG-GAMES.COM.url")) || File.Exists(Path.Combine(installLocation, "SmartSteamEmu.ini")) || File.Exists(Path.Combine(installLocation, "GAMESTORRENT.CO.url")) || File.Exists(Path.Combine(installLocation, "Codex.ini")) || File.Exists(Path.Combine(installLocation, "Skidrow.ini")) || File.Exists(Path.Combine(installLocation, "steamclient.dll"));

            if (areCrackIndicationsPresent)
            {
                reason += "\nParts of game crack are present in the folder.";
            }

            bool isExeInvalid = !ExeUtil.CheckExecutable(installLocation);

            if (isExeInvalid)
            {
                reason += "\nGame executable version doesn't appear to be correct.";
            }

            if (areCrackIndicationsPresent || fakeSteamApi || isExeInvalid)
            {
                MessageBox.Show($"Incompatible Rocksmith version detected! Only the Steam versions of RS are supported - make sure you are not using a pirated / stolen copy of Rocksmith 2014! {Environment.NewLine}Reason: {reason}", "Incompatible Rocksmith version", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Process.Start("https://store.steampowered.com/app/221680/");
                Environment.Exit(1);
                return;
            }
        }
    }
}
