namespace MonitoringClient
{
    partial class ControlLineMulti
    {
        /// <summary> 
        /// 필수 디자이너 변수입니다.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// 사용 중인 모든 리소스를 정리합니다.
        /// </summary>
        /// <param name="disposing">관리되는 리소스를 삭제해야 하면 true이고, 그렇지 않으면 false입니다.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region 구성 요소 디자이너에서 생성한 코드

        /// <summary> 
        /// 디자이너 지원에 필요한 메서드입니다. 
        /// 이 메서드의 내용을 코드 편집기로 수정하지 마세요.
        /// </summary>
        private void InitializeComponent()
        {
            panelResize = new Panel();
            panelTitle = new Panel();
            labelTitle = new Label();
            formsPlot = new ScottPlot.FormsPlot();
            panelTitle.SuspendLayout();
            SuspendLayout();
            // 
            // panelResize
            // 
            panelResize.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            panelResize.BackColor = SystemColors.Control;
            panelResize.Location = new Point(324, 186);
            panelResize.Name = "panelResize";
            panelResize.Size = new Size(9, 9);
            panelResize.TabIndex = 0;
            panelResize.MouseDown += panelResize_MouseDown;
            panelResize.MouseMove += panelResize_MouseMove;
            // 
            // panelTitle
            // 
            panelTitle.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            panelTitle.BackColor = SystemColors.WindowFrame;
            panelTitle.Controls.Add(labelTitle);
            panelTitle.Location = new Point(0, 0);
            panelTitle.Name = "panelTitle";
            panelTitle.Size = new Size(332, 26);
            panelTitle.TabIndex = 1;
            panelTitle.MouseDown += panelTitle_MouseDown;
            panelTitle.MouseMove += panelTitle_MouseMove;
            // 
            // labelTitle
            // 
            labelTitle.AutoSize = true;
            labelTitle.Font = new Font("맑은 고딕", 14.25F, FontStyle.Regular, GraphicsUnit.Point);
            labelTitle.ForeColor = SystemColors.Control;
            labelTitle.Location = new Point(3, 0);
            labelTitle.Name = "labelTitle";
            labelTitle.Size = new Size(46, 25);
            labelTitle.TabIndex = 0;
            labelTitle.Text = "title";
            // 
            // formsPlot
            // 
            formsPlot.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            formsPlot.Location = new Point(-14, 27);
            formsPlot.Margin = new Padding(4, 3, 4, 3);
            formsPlot.Name = "formsPlot";
            formsPlot.Size = new Size(347, 167);
            formsPlot.TabIndex = 2;
            // 
            // ControlLineMulti
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            BackColor = SystemColors.AppWorkspace;
            Controls.Add(panelTitle);
            Controls.Add(panelResize);
            Controls.Add(formsPlot);
            Name = "ControlLineMulti";
            Size = new Size(332, 194);
            panelTitle.ResumeLayout(false);
            panelTitle.PerformLayout();
            ResumeLayout(false);
        }

        #endregion

        private Panel panelResize;
        private Panel panelTitle;
        private Label labelTitle;
        private ScottPlot.FormsPlot formsPlot;
    }
}
