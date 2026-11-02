using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Collections.Generic;
using System.Diagnostics;
using System.Security.Cryptography;
using System.ComponentModel;
using System.Windows.Forms;
using Microsoft.Win32;

static class Program {
 const string ExpectedHash="PAYLOAD_HASH";
 static string Normalize(string p){
  if(String.IsNullOrWhiteSpace(p))return null;
  try{p=Path.GetFullPath(p.Trim('"')).TrimEnd(Path.DirectorySeparatorChar,Path.AltDirectorySeparatorChar);if(File.Exists(Path.Combine(p,"AfterFX.exe")))return p;var support=Path.Combine(p,"Support Files");if(File.Exists(Path.Combine(support,"AfterFX.exe")))return support;}catch{}
  return null;
 }
 internal static List<string> Detect(){
  var found=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
  Action<string> add=p=>{var n=Normalize(p);if(n!=null)found.Add(n);};
  foreach(var pf in new[]{Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86)}){
   string adobe=Path.Combine(pf,"Adobe");try{if(Directory.Exists(adobe))foreach(var d in Directory.GetDirectories(adobe,"Adobe After Effects*"))add(d);}catch{}
  }
  foreach(var view in new[]{RegistryView.Registry64,RegistryView.Registry32})try{
   using(var h=RegistryKey.OpenBaseKey(RegistryHive.LocalMachine,view))using(var root=h.OpenSubKey(@"SOFTWARE\Adobe\After Effects"))if(root!=null)foreach(var version in root.GetSubKeyNames())using(var k=root.OpenSubKey(version))foreach(var name in new[]{"InstallPath","InstallDir","Path"})add(k.GetValue(name) as string);
  }catch{}
  return found.OrderByDescending(p=>p,StringComparer.OrdinalIgnoreCase).ToList();
 }
 internal static byte[] Payload(){
  using(var stream=typeof(Program).Assembly.GetManifestResourceStream("payload.zip"))using(var memory=new MemoryStream()){
   stream.CopyTo(memory);var bytes=memory.ToArray();using(var sha=SHA256.Create())if(BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-","")!=ExpectedHash)throw new Exception("Installer payload checksum failed.");return bytes;
  }
 }
 [STAThread]static void Main(string[] args){
  try{
   if(args.Length==2&&args[0]=="--diagnose"){
    using(var memory=new MemoryStream(Payload()))using(var zip=new ZipArchive(memory,ZipArchiveMode.Read)){
     if(!zip.Entries.Any(e=>e.FullName=="install.ps1")||!zip.Entries.Any(e=>e.FullName=="Assets/ShaderGlass/Library/catalog-v1.json"))throw new Exception("Missing payload files");
     File.WriteAllLines(args[1],new[]{"PAYLOAD_OK","entries="+zip.Entries.Count}.Concat(Detect()));return;
    }
   }
   Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);Application.Run(new Setup());
  }catch(Exception e){MessageBox.Show(e.Message,"FYLM Plugins Setup",MessageBoxButtons.OK,MessageBoxIcon.Error);Environment.ExitCode=1;}
 }
 internal static string CheckPath(string p){var n=Normalize(p);if(n==null)throw new Exception("Select an AE folder containing Support Files/AfterFX.exe.");return n;}
}
class Setup:Form {
 ComboBox paths=new ComboBox();Button install=new Button();TextBox log=new TextBox();Button browse=new Button(),refresh=new Button();
 string T(string en,string zh,string ja){var lang=System.Globalization.CultureInfo.CurrentUICulture.TwoLetterISOLanguageName;return lang=="zh"?zh:lang=="ja"?ja:en;}
 public Setup(){
  Text="FYLM Plugins 0.14.0 Setup";Width=750;Height=410;MinimumSize=new System.Drawing.Size(620,360);StartPosition=FormStartPosition.CenterScreen;
  var grid=new TableLayoutPanel{Dock=DockStyle.Fill,Padding=new Padding(18),ColumnCount=3,RowCount=5};grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));grid.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));grid.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));grid.RowStyles.Add(new RowStyle(SizeType.AutoSize));grid.RowStyles.Add(new RowStyle(SizeType.AutoSize));grid.RowStyles.Add(new RowStyle(SizeType.AutoSize));grid.RowStyles.Add(new RowStyle(SizeType.Percent,100));grid.RowStyles.Add(new RowStyle(SizeType.AutoSize));Controls.Add(grid);
  var intro=new Label{AutoSize=true,Text=T("Select the After Effects version to install into. Close AE and aerender first.","选择要安装的 After Effects 版本，请先关闭 AE 和 aerender。","インストール先の AE を選択してください。AE と aerender を先に終了してください。"),Margin=new Padding(0,0,0,12)};grid.Controls.Add(intro,0,0);grid.SetColumnSpan(intro,3);
  paths.DropDownStyle=ComboBoxStyle.DropDown;paths.Dock=DockStyle.Fill;grid.Controls.Add(paths,0,1);
  refresh.Text=T("Detect","自动检测","自動検出");refresh.AutoSize=true;grid.Controls.Add(refresh,1,1);refresh.Click+=(s,e)=>Reload();
  browse.Text=T("Browse…","手动选择…","参照…");browse.AutoSize=true;grid.Controls.Add(browse,2,1);browse.Click+=(s,e)=>{using(var dialog=new FolderBrowserDialog()){dialog.Description=T("Select the AE version folder or its Support Files folder.","选择 AE 版本目录或其 Support Files 文件夹。","AE のフォルダーまたは Support Files を選択してください。");if(dialog.ShowDialog()==DialogResult.OK)try{paths.Text=Program.CheckPath(dialog.SelectedPath);}catch(Exception ex){MessageBox.Show(ex.Message,Text);}}};
  var help=new Label{AutoSize=true,Text=T("Installs Pixel Art, CRT VHS, ShaderGlass and the optional panel. Existing files are backed up.","安装 Pixel Art、CRT VHS、ShaderGlass 和辅助面板，原文件会自动备份。","Pixel Art、CRT VHS、ShaderGlass とパネルをインストールします。既存ファイルはバックアップされます。"),Margin=new Padding(0,12,0,12)};grid.Controls.Add(help,0,2);grid.SetColumnSpan(help,3);
  log.Multiline=true;log.ReadOnly=true;log.ScrollBars=ScrollBars.Vertical;log.Dock=DockStyle.Fill;grid.Controls.Add(log,0,3);grid.SetColumnSpan(log,3);
  install.Text=T("Install 0.14.0","安装 0.14.0","0.14.0 をインストール");install.AutoSize=true;grid.Controls.Add(install,2,4);install.Click+=(s,e)=>Install();Reload();
 }
 protected override void OnFormClosing(FormClosingEventArgs e){if(!install.Enabled){e.Cancel=true;return;}base.OnFormClosing(e);}
 void Reload(){paths.Items.Clear();foreach(var p in Program.Detect())paths.Items.Add(p);if(paths.Items.Count>0)paths.SelectedIndex=0;log.Text=paths.Items.Count+T(" AE installation(s) detected. You can browse or enter another path."," 个 AE 版本已检测到，也可手动选择或输入路径。"," 件の AE を検出しました。別の場所も指定できます。");}
 void Install(){
  string target;try{target=Program.CheckPath(paths.Text);if(Process.GetProcesses().Any(p=>p.ProcessName.StartsWith("AfterFX",StringComparison.OrdinalIgnoreCase)||p.ProcessName.StartsWith("aerender",StringComparison.OrdinalIgnoreCase)))throw new Exception(T("Save your project and close AE/aerender first.","请保存工程并关闭 AE/aerender 后安装。","プロジェクトを保存し、AE/aerender を終了してください。"));}catch(Exception e){MessageBox.Show(e.Message,Text);return;}
  install.Enabled=browse.Enabled=refresh.Enabled=paths.Enabled=false;log.Text=T("Preparing installation. Windows will request administrator access.","正在准备安装，Windows 将请求管理员授权。","準備中です。Windows の管理者権限を許可してください。");
  var worker=new BackgroundWorker();string folder=null;
  worker.DoWork+=(s,e)=>{
   folder=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"FYLM","Install-0.14.0-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(folder);
   using(var memory=new MemoryStream(Program.Payload()))using(var zip=new ZipArchive(memory,ZipArchiveMode.Read))foreach(var entry in zip.Entries){var output=Path.GetFullPath(Path.Combine(folder,entry.FullName));if(!output.StartsWith(folder+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase))throw new Exception("Invalid archive path");if(entry.Name.Length==0){Directory.CreateDirectory(output);continue;}Directory.CreateDirectory(Path.GetDirectoryName(output));entry.ExtractToFile(output);}
   var psi=new ProcessStartInfo("powershell.exe","-NoProfile -ExecutionPolicy Bypass -File \""+Path.Combine(folder,"install-wrapper.ps1")+"\" -AfterEffects \""+target+"\""){UseShellExecute=true,Verb="runas",WindowStyle=ProcessWindowStyle.Hidden};
   using(var p=Process.Start(psi)){p.WaitForExit();if(p.ExitCode!=0)throw new Exception(File.Exists(Path.Combine(folder,"install.log"))?File.ReadAllText(Path.Combine(folder,"install.log")):"Installation cancelled or failed.");}
   e.Result=File.ReadAllText(Path.Combine(folder,"install.log"));
  };
  worker.RunWorkerCompleted+=(s,e)=>{install.Enabled=browse.Enabled=refresh.Enabled=paths.Enabled=true;log.Text=(e.Error!=null?e.Error.Message:(string)e.Result)+Environment.NewLine+T("Log folder: ","日志目录：","ログ：")+folder;};worker.RunWorkerAsync();
 }
}
