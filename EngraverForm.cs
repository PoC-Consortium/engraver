using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Windows.Forms;
using System.Text.RegularExpressions;
using System.Runtime.InteropServices;

namespace EngraverGui
{
    public partial class EngraverForm : Form
    {
        AutoResetEvent[] autoEvents;
        Process p1;
        int LoGSEC = 4096;
        decimal nonces_to_plot;
        String features = "";

        [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern int GetDiskFreeSpace(string lpRootPathName, out int lpSectorsPerCluster, out int lpBytesPerSector, out int lpNumberOfFreeClusters, out int lpTotalNumberOfClusters);

        struct plotfile
        {
            public ulong id;
            public ulong start;
            public ulong nonces;
            public ulong stagger;
        }

        public EngraverForm()
        {
            InitializeComponent();
        }

        // plotter progress bar
        void TaskProgress(int progress)
        {
            if (statusStrip.InvokeRequired)
            {
                statusStrip.Invoke(new MethodInvoker(() => { TaskProgress(progress); }));
                return;
            }
            else
            {
                pbar.Value = progress;
            }
        }

        // plotter task status
        void TaskStatus(string text)
        {
            if (statusStrip.InvokeRequired)
            {
                statusStrip.Invoke(new MethodInvoker(() => { TaskStatus(text); }));
                return;
            }
            else
            {
                statusLabel1.Text = text;
            }
        }

        // plotter progress bar
        void TaskProgress2(int progress)
        {
            if (statusStrip.InvokeRequired)
            {
                statusStrip.Invoke(new MethodInvoker(() => { TaskProgress2(progress); }));
                return;
            }
            else
            {
                pbar2.Value = progress;
            }
        }

        // plotter task status
        void TaskStatus3(string text)
        {
            if (statusStrip.InvokeRequired)
            {
                statusStrip.Invoke(new MethodInvoker(() => { TaskStatus3(text); }));
                return;
            }
            else
            {
                StatusLabel3.Text = text;
            }
        }

        // plotter task status
        void TaskStatus2(string text)
        {
            if (statusStrip.InvokeRequired)
            {
                statusStrip.Invoke(new MethodInvoker(() => { TaskStatus2(text); }));
                return;
            }
            else
            {
                statusLabel2.Text = text;
            }
        }
        // reset start button
        void ResetButton()
        {
            if (statusStrip.InvokeRequired)
            {
                btn_start.Invoke(new MethodInvoker(() => { ResetButton(); }));
                return;
            }
            else
            {
                btn_start.Text = "Start Plotting";
            }
        }

        // plotter standard output
        void Process_OutputDataReceived(object sender, DataReceivedEventArgs e)
        {
            if (!(sender is Process p))
                return;
            if (plotStatus2.InvokeRequired)
            {
                plotStatus2.Invoke(new MethodInvoker(() => { Process_OutputDataReceived(sender, e); }));
                return;
            }
            else
            {
                if (e.Data != null)
                {
                    if (!e.Data.StartsWith("Hash") && !e.Data.StartsWith("Writ") && !e.Data.StartsWith("0 /") && e.Data != "")
                        plotStatus2.AppendText(e.Data + "\r\n");
                    if (e.Data.StartsWith("Hash"))
                    {
                        String[] test = e.Data.Split('‚');
                        String status = test[test.Length - 1].Trim();
                        TaskStatus(status.Replace("MB", "MiB"));
                        test = status.Split(' ');
                        if (!test[0].StartsWith("Hash"))
                        {
                            int x = int.Parse(test[0].Substring(0, test[0].Length - 1).Split('.')[0]);
                            TaskProgress(x);
                        }
                    }

                    if (e.Data.StartsWith("Writ"))
                    {
                        String[] test = e.Data.Split('‚');
                        String status = test[test.Length - 1].Trim();
                        TaskStatus2(status.Replace("MB", "MiB"));
                        test = status.Split(' ');
                        if (!test[0].StartsWith("Writ"))
                        {
                            int x = int.Parse(test[0].Substring(0, test[0].Length - 1).Split('.')[0]);
                            TaskProgress2(x);
                        }
                    }
                }
            }
        }

        // plotter error output
        void Process_ErrorDataReceived(object sender, DataReceivedEventArgs e)
        {
            Process p = sender as Process;
            if (p == null)
                return;

            if (statusStrip.InvokeRequired)
            {
                statusStrip.Invoke(new MethodInvoker(() => { Process_ErrorDataReceived(sender, e); }));
                return;
            }
            else
            {
                if (e.Data != null)
                {
                    StatusLabel3.Text = e.Data;
                }
            }
        }

        // update plot size label
        private void DisplayPlotSize()
        {
            switch (units.SelectedItem.ToString())
            {
                case "Nonces":
                    plotname.Text = numericID.Text + "_" + startnonce.Value.ToString() + "_" + nonces.Value.ToString();
                    plotsize.Text = "(" + PrettyBytes((ulong)nonces.Value * (2 << 17)) + ")";
                    break;
                case "MiB":
                    plotname.Text = numericID.Text + "_" + startnonce.Value.ToString() + "_" + (nonces.Value * 4).ToString();
                    plotsize.Text = "(" + string.Format("{0:n0}", (ulong)nonces.Value * 4) + " nonces)";
                    break;
                case "GiB":
                    plotname.Text = numericID.Text + "_" + startnonce.Value.ToString() + "_" + (nonces.Value * 4 * 1024).ToString();
                    plotsize.Text = "(" + string.Format("{0:n0}", (ulong)nonces.Value * 4 * 1024) + " nonces)";
                    break;
                case "TiB":
                    plotname.Text = numericID.Text + "_" + startnonce.Value.ToString() + "_" + (nonces.Value * 4 * 1024 * 1024).ToString();
                    plotsize.Text = "(" + string.Format("{0:n0}", (ulong)nonces.Value * 4 * 1024 * 1024) + " nonces)";
                    break;
            }              
        }

        // update target drive info label
        private void UpdateDriveInfo()
        {
            // available space
            if (Directory.Exists(outputFolder.Text))
            {
                DriveInfo drive = new DriveInfo(outputFolder.Text);
                DriveInfo a = new DriveInfo(drive.Name);
                space2.Text = PrettyBytes((ulong)a.AvailableFreeSpace) + " (" + (a.AvailableFreeSpace * 0.99999 / (2 << 17)).ToString("#,##0") + " Nonces)";
                LoGSEC = getSectorSize(outputFolder.Text);
                space2.Text += ", Logical Sector Size: " + LoGSEC.ToString();
            }
            else
            {
                space2.Text = "unknown directory";
            }
        }

        // update nonces to plot label
        private void UpdateNoncesToPlot()
        {
            if (ntpmax.Checked && Directory.Exists(outputFolder.Text))
            {
                DriveInfo drive = new DriveInfo(outputFolder.Text);
                DriveInfo a = new DriveInfo(drive.Name);
                nonces.Value = (decimal)((double)(a.AvailableFreeSpace / (2 << 17)) * 0.99999);
                Properties.Settings.Default.nonces = nonces.Value;
                Properties.Settings.Default.Save();
            }
        }

        // pretty print bytes
        private string PrettyBytes(ulong bytes)
        {
            string result;
            if (bytes < 1024) { result = Math.Round((double)bytes, 1).ToString() + "B"; }
            else if (bytes < 1024 * 1024) { result = Math.Round((double)bytes / 1024, 1).ToString() + "KiB"; }
            else if (bytes < 1024 * 1024 * 1024) { result = Math.Round((double)bytes / 1024 / 1024, 1).ToString() + "MiB"; }
            else if (bytes < 1024L * 1024 * 1024 * 1024) { result = Math.Round((double)bytes / 1024 / 1024 / 1024, 1).ToString() + "GiB"; }
            else { result = Math.Round((double)bytes / 1024 / 1024 / 1024 / 1024, 1).ToString() + "TiB"; }
            return result;
        }

        // load form and user settings
        private void EngraverForm_Load(object sender, EventArgs e)
        {
            //check for exe
            if (File.Exists(Environment.CurrentDirectory + "\\engraver.exe"))
            {
                LoadSettings();
                UpdateDriveInfo();
                UpdateNoncesToPlot();
            }
            else
            {
                MessageBox.Show("Can't find engraver.exe. Shutting down...", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Application.Exit();
            }
        }

        // locate plot output directory
        private void Btn_OutputFolder_Click(object sender, EventArgs e)
        {
            if (folderBrowserDialog.ShowDialog() == DialogResult.OK)
            {
                outputFolder.Text = folderBrowserDialog.SelectedPath;
            }
        }

        // load user settings
        private void LoadSettings()
        {
            decimal backup = Properties.Settings.Default.nonces;
            directio.Checked = Properties.Settings.Default.ddio;
            asyncio.Checked = Properties.Settings.Default.daio;
            units.SelectedIndex = Properties.Settings.Default.unit;
            if (Properties.Settings.Default.ID != "") { numericID.Text = Properties.Settings.Default.ID; }
            outputFolder.Text = Properties.Settings.Default.path;
            ntpValue.Checked = !Properties.Settings.Default.maxnonces;
            ntpmax.Checked = Properties.Settings.Default.maxnonces;
            Console.WriteLine(Properties.Settings.Default.nonces.ToString());
            nonces.Value = backup;
            threads.Value = Properties.Settings.Default.threads;
            mem.Value = Properties.Settings.Default.mem;
            threadlimit.Checked = Properties.Settings.Default.threadlimit;
            lowprio.Checked = Properties.Settings.Default.lowprio;
            memlimit.Checked = Properties.Settings.Default.memlimit;
        }

        // start plotting
        private void start_Click(object sender, EventArgs e)
        {
            if (btn_start.Text == "Start Plotting")
            {
                btn_start.Text = "Stop Plotting";
                plotStatus2.Text = "";

                switch (units.SelectedItem.ToString())
                {
                    case "Nonces":
                        nonces_to_plot = nonces.Value;
                        break;
                    case "MiB":
                        nonces_to_plot = nonces.Value * 4;
                        break;
                    case "GiB":
                        nonces_to_plot = nonces.Value * 4 * 1024;
                        break;
                    case "TiB":
                        nonces_to_plot = nonces.Value * 4 * 1024 * 1024;
                        break;
                }
                features = "";
                if (threadlimit.Checked) features += " -c " + threads.Value.ToString();
                if (memlimit.Checked) features += " -m " + mem.Value.ToString() + "MiB";
                if (!directio.Checked) features += " -d";
                if (!asyncio.Checked) features += " -a";
                if (lowprio.Checked) features += " -l";


                // start control thread
                new Thread(() =>
                {
                    Thread.CurrentThread.IsBackground = true;
                    Control();
                }).Start();

            }
            else
            {
                if (MessageBox.Show("Plotting in progress, are you sure you want to stop?", "Stop Plotting", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    try
                    {
                        p1.Kill();
                    } catch
                    {

                    }
                    ResetButton();
                }
            }
        }

        // control thread
        private void Control()
        {
            autoEvents = new AutoResetEvent[]
            {
                    new AutoResetEvent(false),
            };
            ThreadPool.QueueUserWorkItem(new WaitCallback(U1));
            WaitHandle.WaitAll(autoEvents);
        }

        // plotter process
        public void U1(object stateInfo)
        {
            // reset Status
            TaskStatus3("Starting plotter...");
            try
            {
                using (p1 = new Process())
                {
                    // set start info
                    p1.StartInfo = new ProcessStartInfo("engraver.exe", "-i " + numericID.Text + " -s " + startnonce.Value.ToString() + " -n " + nonces_to_plot.ToString() + " -p " + outputFolder.Text + features )
                    {
                        WindowStyle = ProcessWindowStyle.Hidden,
                        //Arguments = "/A",
                        //RedirectStandardInput = true,
                        RedirectStandardOutput = true,
                        UseShellExecute = false,
                        WorkingDirectory = Environment.CurrentDirectory,
                        CreateNoWindow = true // silent
                    };

                    // event handlers for output & error
                    p1.EnableRaisingEvents = true;
                    p1.OutputDataReceived += Process_OutputDataReceived;
                    p1.ErrorDataReceived += Process_ErrorDataReceived;
                    p1.Exited += new EventHandler(p1_threadExit);
                    // start process
                    TaskStatus3("");
                    p1.Start();
                    p1.BeginOutputReadLine();
                    p1.WaitForExit();
                    autoEvents[0].Set();

                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
            }
        }

        public void p1_threadExit(object sender, System.EventArgs e)
        {
            ResetButton();
            TaskStatus3("Plotting ended.");
        }

        // below this line all GUI handling
        private void output_TextChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.path = outputFolder.Text;
            Properties.Settings.Default.Save();
            UpdateDriveInfo();
            UpdateNoncesToPlot();
        }

        private void numericID_TextChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.ID = numericID.Text;
            Properties.Settings.Default.Save();
        }

        private void threads_ValueChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.threads = (int)threads.Value;
            Properties.Settings.Default.Save();
        }

        private void ram_ValueChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.mem = (int)mem.Value;
            Properties.Settings.Default.Save();
        }

        private void ntpmax_CheckedChanged(object sender, EventArgs e)
        {
            UpdateNoncesToPlot();
            Properties.Settings.Default.maxnonces = ntpmax.Checked;
            Properties.Settings.Default.Save();
        }

        private void ntp_ValueChanged(object sender, EventArgs e)
        {
            if (!ntpmax.Checked)
            {
                if (units.SelectedItem.ToString() == "Nonces")
                {
                    int nonces_per_sector = Math.Max(1, LoGSEC / 64);
                    ulong rounded = (ulong)nonces.Value / (ulong)nonces_per_sector * (ulong)nonces_per_sector;
                    if (rounded == (ulong)nonces.Value || !directio.Checked)
                    {
                        Properties.Settings.Default.nonces = nonces.Value;
                        Properties.Settings.Default.Save();
                    }
                    else
                    {
                        nonces.Value = (decimal)rounded;
                        MessageBox.Show("Number of nonces has been rounded down to sector size.\n This allows for fast direct i/o", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                }
                else if (units.SelectedItem.ToString() == "MiB")
                {
                    int mib_per_sector = Math.Max(1, LoGSEC / 64 / 4);
                    ulong rounded = (ulong)nonces.Value / (ulong)mib_per_sector * (ulong)mib_per_sector;
                    if (rounded == (ulong)nonces.Value || !directio.Checked)
                    {
                        Properties.Settings.Default.nonces = nonces.Value;
                        Properties.Settings.Default.Save();
                    }
                    else
                    {
                        nonces.Value = (decimal)rounded;
                        MessageBox.Show("MiBs have been rounded down to sector size.\n This allows for fast direct i/o", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                }
            }
            DisplayPlotSize();
        }

        private void ntp_Enter(object sender, EventArgs e)
        {
            ntpValue.Checked = true;
        }

        private void btn_auto_Click(object sender, EventArgs e)
        {
            openFileDialog.Filter = "Burst Plot files|*_*_*_*.*;*_*_*.*";
            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                if (isPlotFileName(openFileDialog.FileName))
                {
                    plotfile temp = parsePlotFileName(openFileDialog.FileName);
                    startnonce.Value = temp.start + temp.nonces;
                }
            }
        }

        private bool isPlotFileName(string filename)
        {
            Regex poc1 = new Regex(@"(.)*(\\)+\d+(_)\d+(_)\d+(_)\d+$");
            Regex poc2 = new Regex(@"(.)*(\\)+\d+(_)\d+(_)\d+$");

            if (poc1.IsMatch(filename) || poc2.IsMatch(filename))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        private bool isPoC2PlotFileName(string filename)
        {
            Regex poc2 = new Regex(@"(.)*(\\)+\d+(_)\d+(_)\d+$");

            if (poc2.IsMatch(filename))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        private plotfile parsePlotFileName(string filename)
        {
            string[] temp = filename.Split('\\');
            string[] pfn = temp[temp.GetLength(0) - 1].Split('_');
            plotfile result;
            result.id = Convert.ToUInt64(pfn[0]);
            result.start = Convert.ToUInt64(pfn[1]);
            result.nonces = Convert.ToUInt64(pfn[2]);
            if (pfn.Length == 4)
            {
                result.stagger = Convert.ToUInt64(pfn[3]);
            }
            else
            {
                result.stagger = result.nonces;
            }
            return result;
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            // check if plotting is active
            if (btn_start.Text == "Start Plotting")
            {
                Application.Exit();
            }
            else
            {
                if (MessageBox.Show("Plotting in progress, are you sure you want to exit?", "Stop Plotting", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    try
                    {
                        p1.Kill();
                    } catch
                    {

                    }
                    Application.Exit();
                }
            }

        }

        private void resumeFileToolStripMenuItem_Click(object sender, EventArgs e)
        {
            openFileDialog.Filter = "Burst PoC2 Plot files|*_*_*.*";
            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                if (isPoC2PlotFileName(openFileDialog.FileName))
                {
                    plotfile temp = parsePlotFileName(openFileDialog.FileName);
                    numericID.Text = temp.id.ToString();
                    startnonce.Value = temp.start;
                    nonces.Value = (decimal)temp.nonces;
                    outputFolder.Text = Path.GetDirectoryName(openFileDialog.FileName);
                }
            }
        }

        private void units_SelectedIndexChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.unit = units.SelectedIndex;
            Properties.Settings.Default.Save();
            switch (units.SelectedItem.ToString())
            {
                case "Nonces":
                    nonces.Increment = 1000;
                    break;
                case "MiB":
                    nonces.Increment = 100;
                    break;
                case "GiB":
                    nonces.Increment = 100;
                    break;
                case "TiB":
                    nonces.Increment = 1;
                    break;
            }
            ntp_ValueChanged(null, null);
        }

        private void threadlimit_CheckedChanged(object sender, EventArgs e)
        {
            if (threadlimit.Checked)
            {
                threads.Enabled = true;
            } else
            {
                threads.Enabled = false;
            }
            Properties.Settings.Default.threadlimit = threadlimit.Checked;
            Properties.Settings.Default.Save();
        }

        private void lowprio_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.lowprio = lowprio.Checked;
            Properties.Settings.Default.Save();
        }

        private void memlimit_CheckedChanged(object sender, EventArgs e)
        {
            if (memlimit.Checked)
            {
                mem.Enabled = true;
            }
            else
            {
                mem.Enabled = false;
            }
            Properties.Settings.Default.memlimit = memlimit.Checked;
            Properties.Settings.Default.Save();
        }

        private void directio_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.ddio = directio.Checked;
            Properties.Settings.Default.Save();
        }

        private void asyncio_CheckedChanged(object sender, EventArgs e)
        {
            Properties.Settings.Default.daio = asyncio.Checked;
            Properties.Settings.Default.Save();
        }

        private void aboutToolStripMenuItem1_Click(object sender, EventArgs e)
        {
            System.Diagnostics.Process.Start("https://www.github.com/PoC-Consortium/engraver/wiki");
        }

        private void aboutToolStripMenuItem2_Click(object sender, EventArgs e)
        {
            System.Diagnostics.Process.Start("https://www.github.com/PoC-Consortium/engraver/blob/master/README.md");
        }

        private int getSectorSize(String directory)
        {
            int SectorsPerCluster;
            int BytesPerSector = 4096;
            int NumberOfFreeClusters;
            int TotalNumberOfClusters;
            try
            {

                FileInfo file = new FileInfo(directory);
                DriveInfo drive = new DriveInfo(file.Directory.Root.FullName);
                GetDiskFreeSpace(drive.Name, out SectorsPerCluster, out BytesPerSector, out NumberOfFreeClusters, out TotalNumberOfClusters);
            }
            catch (Exception e)
            {
            }
            return BytesPerSector;
        }
    }
}
