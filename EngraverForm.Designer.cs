namespace EngraverGui
{
    partial class EngraverForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(EngraverForm));
            this.folderBrowserDialog = new System.Windows.Forms.FolderBrowserDialog();
            this.openFileDialog = new System.Windows.Forms.OpenFileDialog();
            this.btn_start = new System.Windows.Forms.Button();
            this.statusStrip = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.pbar = new System.Windows.Forms.ToolStripProgressBar();
            this.statusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolStripStatusLabel2 = new System.Windows.Forms.ToolStripStatusLabel();
            this.pbar2 = new System.Windows.Forms.ToolStripProgressBar();
            this.statusLabel2 = new System.Windows.Forms.ToolStripStatusLabel();
            this.StatusLabel3 = new System.Windows.Forms.ToolStripStatusLabel();
            this.StatusLabel4 = new System.Windows.Forms.ToolStripStatusLabel();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.resumeFileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.aboutToolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.aboutToolStripMenuItem2 = new System.Windows.Forms.ToolStripMenuItem();
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.plotsize = new System.Windows.Forms.Label();
            this.units = new System.Windows.Forms.ComboBox();
            this.label3 = new System.Windows.Forms.Label();
            this.plotStatus2 = new System.Windows.Forms.TextBox();
            this.ntpValue = new System.Windows.Forms.RadioButton();
            this.ntpmax = new System.Windows.Forms.RadioButton();
            this.numericID = new System.Windows.Forms.TextBox();
            this.lbl_ID = new System.Windows.Forms.Label();
            this.button1 = new System.Windows.Forms.Button();
            this.plotname = new System.Windows.Forms.Label();
            this.startnonce = new System.Windows.Forms.NumericUpDown();
            this.label12 = new System.Windows.Forms.Label();
            this.btn_OutputFolder = new System.Windows.Forms.Button();
            this.space2 = new System.Windows.Forms.Label();
            this.lbl_space = new System.Windows.Forms.Label();
            this.nonces = new System.Windows.Forms.NumericUpDown();
            this.outputFolder = new System.Windows.Forms.TextBox();
            this.lbl_target = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.label4 = new System.Windows.Forms.Label();
            this.lowprio = new System.Windows.Forms.CheckBox();
            this.asyncio = new System.Windows.Forms.CheckBox();
            this.directio = new System.Windows.Forms.CheckBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.memlimit = new System.Windows.Forms.CheckBox();
            this.threadlimit = new System.Windows.Forms.CheckBox();
            this.lbl_CPURAM = new System.Windows.Forms.Label();
            this.lbl_RAM2 = new System.Windows.Forms.Label();
            this.mem = new System.Windows.Forms.NumericUpDown();
            this.threads = new System.Windows.Forms.NumericUpDown();
            this.toolTips = new System.Windows.Forms.ToolTip(this.components);
            this.statusStrip.SuspendLayout();
            this.menuStrip1.SuspendLayout();
            this.tabControl1.SuspendLayout();
            this.tabPage1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.startnonce)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.nonces)).BeginInit();
            this.tabPage2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.mem)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.threads)).BeginInit();
            this.SuspendLayout();
            // 
            // openFileDialog
            // 
            this.openFileDialog.Filter = "Burst Plot files|*_*_*_*.*;*_*_*.*";
            // 
            // btn_start
            // 
            this.btn_start.Location = new System.Drawing.Point(220, 187);
            this.btn_start.Name = "btn_start";
            this.btn_start.Size = new System.Drawing.Size(153, 30);
            this.btn_start.TabIndex = 18;
            this.btn_start.Text = "Start Plotting";
            this.btn_start.UseVisualStyleBackColor = true;
            this.btn_start.Click += new System.EventHandler(this.start_Click);
            // 
            // statusStrip
            // 
            this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel1,
            this.pbar,
            this.statusLabel1,
            this.toolStripStatusLabel2,
            this.pbar2,
            this.statusLabel2,
            this.StatusLabel3,
            this.StatusLabel4});
            this.statusStrip.Location = new System.Drawing.Point(0, 449);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Size = new System.Drawing.Size(624, 22);
            this.statusStrip.TabIndex = 2;
            this.statusStrip.Text = "statusStrip";
            // 
            // toolStripStatusLabel1
            // 
            this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            this.toolStripStatusLabel1.Size = new System.Drawing.Size(44, 17);
            this.toolStripStatusLabel1.Text = "Hasher";
            // 
            // pbar
            // 
            this.pbar.Name = "pbar";
            this.pbar.Size = new System.Drawing.Size(100, 16);
            this.pbar.ToolTipText = "Hasher Progress";
            // 
            // statusLabel1
            // 
            this.statusLabel1.Name = "statusLabel1";
            this.statusLabel1.Size = new System.Drawing.Size(34, 17);
            this.statusLabel1.Text = "(idle)";
            // 
            // toolStripStatusLabel2
            // 
            this.toolStripStatusLabel2.Name = "toolStripStatusLabel2";
            this.toolStripStatusLabel2.Size = new System.Drawing.Size(39, 17);
            this.toolStripStatusLabel2.Text = "Writer";
            // 
            // pbar2
            // 
            this.pbar2.Name = "pbar2";
            this.pbar2.Size = new System.Drawing.Size(100, 16);
            this.pbar2.ToolTipText = "Writer Progress";
            // 
            // statusLabel2
            // 
            this.statusLabel2.Name = "statusLabel2";
            this.statusLabel2.Size = new System.Drawing.Size(34, 17);
            this.statusLabel2.Text = "(idle)";
            // 
            // StatusLabel3
            // 
            this.StatusLabel3.Name = "StatusLabel3";
            this.StatusLabel3.Size = new System.Drawing.Size(19, 17);
            this.StatusLabel3.Text = "    ";
            // 
            // StatusLabel4
            // 
            this.StatusLabel4.Name = "StatusLabel4";
            this.StatusLabel4.Size = new System.Drawing.Size(19, 17);
            this.StatusLabel4.Text = "    ";
            // 
            // menuStrip1
            // 
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.helpToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(624, 24);
            this.menuStrip1.TabIndex = 0;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.resumeFileToolStripMenuItem,
            this.toolStripSeparator1,
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.fileToolStripMenuItem.Text = "&File";
            // 
            // resumeFileToolStripMenuItem
            // 
            this.resumeFileToolStripMenuItem.Name = "resumeFileToolStripMenuItem";
            this.resumeFileToolStripMenuItem.Size = new System.Drawing.Size(146, 22);
            this.resumeFileToolStripMenuItem.Text = "&Resume File...";
            this.resumeFileToolStripMenuItem.Click += new System.EventHandler(this.resumeFileToolStripMenuItem_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(143, 6);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(146, 22);
            this.exitToolStripMenuItem.Text = "&Exit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            // 
            // helpToolStripMenuItem
            // 
            this.helpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.aboutToolStripMenuItem1,
            this.toolStripSeparator2,
            this.aboutToolStripMenuItem2});
            this.helpToolStripMenuItem.Name = "helpToolStripMenuItem";
            this.helpToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.helpToolStripMenuItem.Text = "&Help";
            // 
            // aboutToolStripMenuItem1
            // 
            this.aboutToolStripMenuItem1.Name = "aboutToolStripMenuItem1";
            this.aboutToolStripMenuItem1.Size = new System.Drawing.Size(107, 22);
            this.aboutToolStripMenuItem1.Text = "&Help";
            this.aboutToolStripMenuItem1.Click += new System.EventHandler(this.aboutToolStripMenuItem1_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(104, 6);
            // 
            // aboutToolStripMenuItem2
            // 
            this.aboutToolStripMenuItem2.Name = "aboutToolStripMenuItem2";
            this.aboutToolStripMenuItem2.Size = new System.Drawing.Size(107, 22);
            this.aboutToolStripMenuItem2.Text = "&About";
            this.aboutToolStripMenuItem2.Click += new System.EventHandler(this.aboutToolStripMenuItem2_Click);
            // 
            // tabControl1
            // 
            this.tabControl1.Controls.Add(this.tabPage1);
            this.tabControl1.Controls.Add(this.tabPage2);
            this.tabControl1.Location = new System.Drawing.Point(12, 27);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(601, 417);
            this.tabControl1.TabIndex = 1;
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.plotsize);
            this.tabPage1.Controls.Add(this.units);
            this.tabPage1.Controls.Add(this.label3);
            this.tabPage1.Controls.Add(this.plotStatus2);
            this.tabPage1.Controls.Add(this.ntpValue);
            this.tabPage1.Controls.Add(this.ntpmax);
            this.tabPage1.Controls.Add(this.numericID);
            this.tabPage1.Controls.Add(this.btn_start);
            this.tabPage1.Controls.Add(this.lbl_ID);
            this.tabPage1.Controls.Add(this.button1);
            this.tabPage1.Controls.Add(this.plotname);
            this.tabPage1.Controls.Add(this.startnonce);
            this.tabPage1.Controls.Add(this.label12);
            this.tabPage1.Controls.Add(this.btn_OutputFolder);
            this.tabPage1.Controls.Add(this.space2);
            this.tabPage1.Controls.Add(this.lbl_space);
            this.tabPage1.Controls.Add(this.nonces);
            this.tabPage1.Controls.Add(this.outputFolder);
            this.tabPage1.Controls.Add(this.lbl_target);
            this.tabPage1.Controls.Add(this.label8);
            this.tabPage1.Controls.Add(this.label5);
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(593, 391);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "Basic Settings";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // plotsize
            // 
            this.plotsize.AutoSize = true;
            this.plotsize.Location = new System.Drawing.Point(448, 132);
            this.plotsize.Name = "plotsize";
            this.plotsize.Size = new System.Drawing.Size(55, 13);
            this.plotsize.TabIndex = 15;
            this.plotsize.Text = "(available)";
            // 
            // units
            // 
            this.units.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.units.FormattingEnabled = true;
            this.units.Items.AddRange(new object[] {
            "Nonces",
            "MiB",
            "GiB",
            "TiB"});
            this.units.Location = new System.Drawing.Point(372, 129);
            this.units.Name = "units";
            this.units.Size = new System.Drawing.Size(60, 21);
            this.units.TabIndex = 14;
            this.units.Tag = "";
            this.units.SelectedIndexChanged += new System.EventHandler(this.units_SelectedIndexChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(6, 217);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(39, 13);
            this.label3.TabIndex = 19;
            this.label3.Text = "Output";
            // 
            // plotStatus2
            // 
            this.plotStatus2.Font = new System.Drawing.Font("Lucida Console", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.plotStatus2.Location = new System.Drawing.Point(6, 233);
            this.plotStatus2.Multiline = true;
            this.plotStatus2.Name = "plotStatus2";
            this.plotStatus2.ReadOnly = true;
            this.plotStatus2.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.plotStatus2.Size = new System.Drawing.Size(581, 149);
            this.plotStatus2.TabIndex = 20;
            // 
            // ntpValue
            // 
            this.ntpValue.AutoSize = true;
            this.ntpValue.Location = new System.Drawing.Point(188, 130);
            this.ntpValue.Name = "ntpValue";
            this.ntpValue.Size = new System.Drawing.Size(52, 17);
            this.ntpValue.TabIndex = 12;
            this.ntpValue.Text = "Value";
            this.toolTips.SetToolTip(this.ntpValue, "specify plot file size");
            this.ntpValue.UseVisualStyleBackColor = true;
            // 
            // ntpmax
            // 
            this.ntpmax.AutoSize = true;
            this.ntpmax.Checked = true;
            this.ntpmax.Location = new System.Drawing.Point(102, 130);
            this.ntpmax.Name = "ntpmax";
            this.ntpmax.Size = new System.Drawing.Size(69, 17);
            this.ntpmax.TabIndex = 11;
            this.ntpmax.TabStop = true;
            this.ntpmax.Text = "Maximum";
            this.toolTips.SetToolTip(this.ntpmax, "plot all available space");
            this.ntpmax.UseVisualStyleBackColor = true;
            this.ntpmax.CheckedChanged += new System.EventHandler(this.ntpmax_CheckedChanged);
            // 
            // numericID
            // 
            this.numericID.Location = new System.Drawing.Point(102, 12);
            this.numericID.Name = "numericID";
            this.numericID.Size = new System.Drawing.Size(295, 20);
            this.numericID.TabIndex = 1;
            this.toolTips.SetToolTip(this.numericID, "your numeric Burst ID");
            this.numericID.TextChanged += new System.EventHandler(this.numericID_TextChanged);
            // 
            // lbl_ID
            // 
            this.lbl_ID.AutoSize = true;
            this.lbl_ID.Location = new System.Drawing.Point(6, 15);
            this.lbl_ID.Name = "lbl_ID";
            this.lbl_ID.Size = new System.Drawing.Size(60, 13);
            this.lbl_ID.TabIndex = 0;
            this.lbl_ID.Text = "Numeric ID";
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(246, 100);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(120, 20);
            this.button1.TabIndex = 9;
            this.button1.Text = "auto from last file...";
            this.toolTips.SetToolTip(this.button1, "determine start nonce from last plot file");
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.btn_auto_Click);
            // 
            // plotname
            // 
            this.plotname.AutoSize = true;
            this.plotname.Location = new System.Drawing.Point(99, 161);
            this.plotname.Name = "plotname";
            this.plotname.Size = new System.Drawing.Size(55, 13);
            this.plotname.TabIndex = 17;
            this.plotname.Text = "(available)";
            // 
            // startnonce
            // 
            this.startnonce.Increment = new decimal(new int[] {
            1000000,
            0,
            0,
            0});
            this.startnonce.Location = new System.Drawing.Point(102, 100);
            this.startnonce.Maximum = new decimal(new int[] {
            -1,
            -1,
            0,
            0});
            this.startnonce.Name = "startnonce";
            this.startnonce.Size = new System.Drawing.Size(138, 20);
            this.startnonce.TabIndex = 8;
            this.startnonce.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.startnonce.ThousandsSeparator = true;
            this.toolTips.SetToolTip(this.startnonce, "where you want to start plotting");
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Location = new System.Drawing.Point(6, 161);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(66, 13);
            this.label12.TabIndex = 16;
            this.label12.Text = "Plot Preview";
            // 
            // btn_OutputFolder
            // 
            this.btn_OutputFolder.Location = new System.Drawing.Point(403, 39);
            this.btn_OutputFolder.Name = "btn_OutputFolder";
            this.btn_OutputFolder.Size = new System.Drawing.Size(66, 20);
            this.btn_OutputFolder.TabIndex = 4;
            this.btn_OutputFolder.Text = "Browse...";
            this.toolTips.SetToolTip(this.btn_OutputFolder, "Locate output folder...");
            this.btn_OutputFolder.UseVisualStyleBackColor = true;
            this.btn_OutputFolder.Click += new System.EventHandler(this.Btn_OutputFolder_Click);
            // 
            // space2
            // 
            this.space2.AutoSize = true;
            this.space2.Location = new System.Drawing.Point(99, 71);
            this.space2.Name = "space2";
            this.space2.Size = new System.Drawing.Size(52, 13);
            this.space2.TabIndex = 6;
            this.space2.Text = "               ";
            // 
            // lbl_space
            // 
            this.lbl_space.AutoSize = true;
            this.lbl_space.Location = new System.Drawing.Point(6, 71);
            this.lbl_space.Name = "lbl_space";
            this.lbl_space.Size = new System.Drawing.Size(53, 13);
            this.lbl_space.TabIndex = 5;
            this.lbl_space.Text = "Drive Info";
            // 
            // nonces
            // 
            this.nonces.Increment = new decimal(new int[] {
            1000000,
            0,
            0,
            0});
            this.nonces.Location = new System.Drawing.Point(246, 130);
            this.nonces.Maximum = new decimal(new int[] {
            -1,
            -1,
            0,
            0});
            this.nonces.Name = "nonces";
            this.nonces.Size = new System.Drawing.Size(120, 20);
            this.nonces.TabIndex = 13;
            this.nonces.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.nonces.ThousandsSeparator = true;
            this.toolTips.SetToolTip(this.nonces, "size you want to plot");
            this.nonces.ValueChanged += new System.EventHandler(this.ntp_ValueChanged);
            this.nonces.Enter += new System.EventHandler(this.ntp_Enter);
            // 
            // outputFolder
            // 
            this.outputFolder.Location = new System.Drawing.Point(102, 39);
            this.outputFolder.Name = "outputFolder";
            this.outputFolder.Size = new System.Drawing.Size(295, 20);
            this.outputFolder.TabIndex = 3;
            this.toolTips.SetToolTip(this.outputFolder, "target path for plotfile");
            this.outputFolder.TextChanged += new System.EventHandler(this.output_TextChanged);
            // 
            // lbl_target
            // 
            this.lbl_target.AutoSize = true;
            this.lbl_target.Location = new System.Drawing.Point(6, 42);
            this.lbl_target.Name = "lbl_target";
            this.lbl_target.Size = new System.Drawing.Size(71, 13);
            this.lbl_target.TabIndex = 2;
            this.lbl_target.Text = "Output Folder";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(6, 132);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(59, 13);
            this.label8.TabIndex = 10;
            this.label8.Text = "Size to plot";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(6, 102);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(64, 13);
            this.label5.TabIndex = 7;
            this.label5.Text = "Start Nonce";
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.label4);
            this.tabPage2.Controls.Add(this.lowprio);
            this.tabPage2.Controls.Add(this.asyncio);
            this.tabPage2.Controls.Add(this.directio);
            this.tabPage2.Controls.Add(this.label2);
            this.tabPage2.Controls.Add(this.label1);
            this.tabPage2.Controls.Add(this.memlimit);
            this.tabPage2.Controls.Add(this.threadlimit);
            this.tabPage2.Controls.Add(this.lbl_CPURAM);
            this.tabPage2.Controls.Add(this.lbl_RAM2);
            this.tabPage2.Controls.Add(this.mem);
            this.tabPage2.Controls.Add(this.threads);
            this.tabPage2.Location = new System.Drawing.Point(4, 22);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(593, 391);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "Advanced Settings";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(222, 15);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(42, 13);
            this.label4.TabIndex = 3;
            this.label4.Text = "threads";
            // 
            // lowprio
            // 
            this.lowprio.AutoSize = true;
            this.lowprio.Location = new System.Drawing.Point(63, 37);
            this.lowprio.Name = "lowprio";
            this.lowprio.Size = new System.Drawing.Size(79, 17);
            this.lowprio.TabIndex = 4;
            this.lowprio.Text = "Low priority";
            this.toolTips.SetToolTip(this.lowprio, "run engraver as low priority process");
            this.lowprio.UseVisualStyleBackColor = true;
            this.lowprio.CheckedChanged += new System.EventHandler(this.lowprio_CheckedChanged);
            // 
            // asyncio
            // 
            this.asyncio.AutoSize = true;
            this.asyncio.Checked = true;
            this.asyncio.CheckState = System.Windows.Forms.CheckState.Checked;
            this.asyncio.Location = new System.Drawing.Point(152, 102);
            this.asyncio.Name = "asyncio";
            this.asyncio.Size = new System.Drawing.Size(74, 17);
            this.asyncio.TabIndex = 11;
            this.asyncio.Text = "Async I/O";
            this.toolTips.SetToolTip(this.asyncio, "enable async i/o, i.e. hash and write simultaneously");
            this.asyncio.UseVisualStyleBackColor = true;
            this.asyncio.CheckedChanged += new System.EventHandler(this.asyncio_CheckedChanged);
            // 
            // directio
            // 
            this.directio.AutoSize = true;
            this.directio.Checked = true;
            this.directio.CheckState = System.Windows.Forms.CheckState.Checked;
            this.directio.Location = new System.Drawing.Point(63, 102);
            this.directio.Name = "directio";
            this.directio.Size = new System.Drawing.Size(73, 17);
            this.directio.TabIndex = 10;
            this.directio.Text = "Direct I/O";
            this.toolTips.SetToolTip(this.directio, "enable direct i/o - direct disk writes without buffering");
            this.directio.UseVisualStyleBackColor = true;
            this.directio.CheckedChanged += new System.EventHandler(this.directio_CheckedChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(15, 103);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(23, 13);
            this.label2.TabIndex = 9;
            this.label2.Text = "I/O";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(15, 71);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(31, 13);
            this.label1.TabIndex = 5;
            this.label1.Text = "RAM";
            // 
            // memlimit
            // 
            this.memlimit.AutoSize = true;
            this.memlimit.Location = new System.Drawing.Point(63, 70);
            this.memlimit.Name = "memlimit";
            this.memlimit.Size = new System.Drawing.Size(83, 17);
            this.memlimit.TabIndex = 6;
            this.memlimit.Text = "Memory limit";
            this.toolTips.SetToolTip(this.memlimit, "enable memory limit");
            this.memlimit.UseVisualStyleBackColor = true;
            this.memlimit.CheckedChanged += new System.EventHandler(this.memlimit_CheckedChanged);
            // 
            // threadlimit
            // 
            this.threadlimit.AutoSize = true;
            this.threadlimit.Location = new System.Drawing.Point(63, 14);
            this.threadlimit.Name = "threadlimit";
            this.threadlimit.Size = new System.Drawing.Size(80, 17);
            this.threadlimit.TabIndex = 1;
            this.threadlimit.Text = "Thread limit";
            this.toolTips.SetToolTip(this.threadlimit, "enable thread limit");
            this.threadlimit.UseVisualStyleBackColor = true;
            this.threadlimit.CheckedChanged += new System.EventHandler(this.threadlimit_CheckedChanged);
            // 
            // lbl_CPURAM
            // 
            this.lbl_CPURAM.AutoSize = true;
            this.lbl_CPURAM.Location = new System.Drawing.Point(15, 15);
            this.lbl_CPURAM.Name = "lbl_CPURAM";
            this.lbl_CPURAM.Size = new System.Drawing.Size(29, 13);
            this.lbl_CPURAM.TabIndex = 0;
            this.lbl_CPURAM.Text = "CPU";
            // 
            // lbl_RAM2
            // 
            this.lbl_RAM2.AutoSize = true;
            this.lbl_RAM2.Location = new System.Drawing.Point(222, 71);
            this.lbl_RAM2.Name = "lbl_RAM2";
            this.lbl_RAM2.Size = new System.Drawing.Size(25, 13);
            this.lbl_RAM2.TabIndex = 8;
            this.lbl_RAM2.Text = "MiB";
            // 
            // mem
            // 
            this.mem.Enabled = false;
            this.mem.Location = new System.Drawing.Point(152, 69);
            this.mem.Maximum = new decimal(new int[] {
            1024000,
            0,
            0,
            0});
            this.mem.Name = "mem";
            this.mem.Size = new System.Drawing.Size(64, 20);
            this.mem.TabIndex = 7;
            this.mem.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.toolTips.SetToolTip(this.mem, "set memory limit");
            this.mem.Value = new decimal(new int[] {
            4096,
            0,
            0,
            0});
            this.mem.ValueChanged += new System.EventHandler(this.ram_ValueChanged);
            // 
            // threads
            // 
            this.threads.Enabled = false;
            this.threads.Location = new System.Drawing.Point(152, 13);
            this.threads.Name = "threads";
            this.threads.Size = new System.Drawing.Size(64, 20);
            this.threads.TabIndex = 2;
            this.threads.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.toolTips.SetToolTip(this.threads, "set thread limit");
            this.threads.Value = new decimal(new int[] {
            4,
            0,
            0,
            0});
            this.threads.ValueChanged += new System.EventHandler(this.threads_ValueChanged);
            // 
            // EngraverForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(624, 471);
            this.Controls.Add(this.tabControl1);
            this.Controls.Add(this.statusStrip);
            this.Controls.Add(this.menuStrip1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MaximumSize = new System.Drawing.Size(640, 510);
            this.MinimumSize = new System.Drawing.Size(640, 510);
            this.Name = "EngraverForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Engraver GUI v.2.0.4";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.EngraverForm_FormClosing);
            this.Load += new System.EventHandler(this.EngraverForm_Load);
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.tabControl1.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.tabPage1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.startnonce)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.nonces)).EndInit();
            this.tabPage2.ResumeLayout(false);
            this.tabPage2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.mem)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.threads)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.FolderBrowserDialog folderBrowserDialog;
        private System.Windows.Forms.OpenFileDialog openFileDialog;
        private System.Windows.Forms.Button btn_start;
        private System.Windows.Forms.StatusStrip statusStrip;
        private System.Windows.Forms.ToolStripStatusLabel statusLabel2;
        private System.Windows.Forms.ToolStripProgressBar pbar;
        private System.Windows.Forms.ToolStripStatusLabel statusLabel1;
        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem resumeFileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem2;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.TabControl tabControl1;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.RadioButton ntpValue;
        private System.Windows.Forms.RadioButton ntpmax;
        private System.Windows.Forms.TextBox numericID;
        private System.Windows.Forms.Label lbl_ID;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Label plotname;
        private System.Windows.Forms.NumericUpDown startnonce;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.Button btn_OutputFolder;
        private System.Windows.Forms.Label space2;
        private System.Windows.Forms.Label lbl_space;
        private System.Windows.Forms.NumericUpDown nonces;
        private System.Windows.Forms.TextBox outputFolder;
        private System.Windows.Forms.Label lbl_target;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TabPage tabPage2;
        private System.Windows.Forms.CheckBox lowprio;
        private System.Windows.Forms.CheckBox asyncio;
        private System.Windows.Forms.CheckBox directio;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.CheckBox memlimit;
        private System.Windows.Forms.CheckBox threadlimit;
        private System.Windows.Forms.Label lbl_CPURAM;
        private System.Windows.Forms.Label lbl_RAM2;
        private System.Windows.Forms.NumericUpDown mem;
        private System.Windows.Forms.NumericUpDown threads;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TextBox plotStatus2;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.ComboBox units;
        private System.Windows.Forms.ToolTip toolTips;
        private System.Windows.Forms.Label plotsize;
        private System.Windows.Forms.ToolStripStatusLabel StatusLabel3;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel2;
        private System.Windows.Forms.ToolStripProgressBar pbar2;
        private System.Windows.Forms.ToolStripStatusLabel StatusLabel4;
    }
}

