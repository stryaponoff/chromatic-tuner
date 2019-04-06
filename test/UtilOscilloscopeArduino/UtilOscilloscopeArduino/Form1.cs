using System;using System.Collections.Generic;using System.ComponentModel;using System.Data;using System.Drawing;using System.Linq;using System.Text;using System.Windows.Forms;using System.IO.Ports;namespace UtilOscilloscopeArduino{    public partial class Form1 : Form    {        SerialPort sp;        Point[] data = new Point[500];        Graphics g;        int i = 0;        public Form1()        {            InitializeComponent();            string[] portNames = SerialPort.GetPortNames();            foreach(var item in portNames)            {                comboBox1.Items.Add(item);            }        }        private void Form1_Load(object sender, EventArgs e)        {        }        private void button2_Click(object sender, EventArgs e)        {            comboBox1.Items.Clear();            string[] portNames = SerialPort.GetPortNames();            foreach (var item in portNames)            {                comboBox1.Items.Add(item);            }        }        private void button1_Click(object sender, EventArgs e)        {            sp = new SerialPort(comboBox1.SelectedItem.ToString(), 115200, Parity.None);            sp.DataReceived += Sp_DataReceived;            sp.Open();        }        private void Sp_DataReceived(object sender, SerialDataReceivedEventArgs e)        {                       string raw = sp.ReadLine();            label1.Text = raw;        }        private void button3_Click(object sender, EventArgs e)        {            sp.Close();        }    }}