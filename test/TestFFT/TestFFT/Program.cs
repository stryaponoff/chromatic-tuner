using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Numerics;using System.Text;
namespace TestFFT
{
    class Program
    {        public const double SinglePi = Math.PI;
        public const double DoublePi = 2 * Math.PI;

        public static Dictionary<double, double> GetJoinedSpectrum(
            IList<Complex> spectrum0, IList<Complex> spectrum1,
            double shiftsPerFrame, double sampleRate)
        {
            var frameSize = spectrum0.Count;
            var frameTime = frameSize / sampleRate;
            var shiftTime = frameTime / shiftsPerFrame;
            var binToFrequency = sampleRate / frameSize;
            var dictionary = new Dictionary<double, double>();

            for (var bin = 0; bin < frameSize; bin++)
            {
                var omegaExpected = DoublePi * (bin * binToFrequency); // ω=2πf
                var omegaActual = (spectrum1[bin].Phase - spectrum0[bin].Phase) / shiftTime; // ω=∂φ/∂t
                var omegaDelta = Align(omegaActual - omegaExpected, DoublePi); // Δω=(∂ω + π)%2π - π
                var binDelta = omegaDelta / (DoublePi * binToFrequency);
                var frequencyActual = (bin + binDelta) * binToFrequency;
                var magnitude = spectrum1[bin].Magnitude + spectrum0[bin].Magnitude;
                dictionary.Add(frequencyActual, magnitude * (0.5 + Math.Abs(binDelta)));
            }

            return dictionary;
        }

        public static double Align(double angle, double period)
        {
            var qpd = (int)(angle / period);
            if (qpd >= 0) qpd += qpd & 1;
            else qpd -= qpd & 1;
            angle -= period * qpd;
            return angle;
        }

        public static Complex[] fft(Complex[] frame, bool direct)
        {
            if (frame.Length == 1) return frame;
            var frameHalfSize = frame.Length >> 1; // frame.Length/2
            var frameFullSize = frame.Length;

            var frameOdd = new Complex[frameHalfSize];
            var frameEven = new Complex[frameHalfSize];
            for (var i = 0; i < frameHalfSize; i++)
            {
                var j = i << 1; // i = 2*j;
                frameOdd[i] = frame[j + 1];
                frameEven[i] = frame[j];
            }

            var spectrumOdd = fft(frameOdd, direct);
            var spectrumEven = fft(frameEven, direct);

            var arg = direct ? -DoublePi / frameFullSize : DoublePi / frameFullSize;
            var omegaPowBase = new Complex(Math.Cos(arg), Math.Sin(arg));
            var omega = Complex.One;
            var spectrum = new Complex[frameFullSize];

            for (var j = 0; j < frameHalfSize; j++)
            {
                spectrum[j] = spectrumEven[j] + omega * spectrumOdd[j];
                spectrum[j + frameHalfSize] = spectrumEven[j] - omega * spectrumOdd[j];
                omega *= omegaPowBase;
            }

            return spectrum;
        }
        static Complex[] SinGenerate(double time, double samplingFreq, double offset, double amplitude, double signalFreq)
        {
            Complex[] points = new Complex[Convert.ToInt32(time / (1.0 / samplingFreq))];
            double step = 1 / samplingFreq;
            double t = 0;            int i = 0;
            while (t < time - step)
            {
                double v = offset + amplitude * Math.Sin(2 * Math.PI * signalFreq * t);
                points[i] = new Complex(v, 0);
                t += step;                i++;
            }
            return points;
        }

        static void Main(string[] args)
        {

            Complex[] signalList = SinGenerate(1, 44100, 0, 3, 200);            Complex[] frame1 = new Complex[512];            Complex[] frame2 = new Complex[512];            int offset = 512 / 16;            for (int i = 0; i < 512; i++)            {                frame1[i] = signalList[i];            }            for (int i = 0; i < 512; i++)            {                frame2[i] = signalList[i + offset];            }            var spectrum0 = fft(frame1, true);            var spectrum1 = fft(frame2, true);            for (var i = 0; i < 512; i++)            {                spectrum0[i] /= 512;                spectrum1[i] /= 512;            }            var spectrum = GetJoinedSpectrum(spectrum0, spectrum1, 16, 44100);            string s = "";            for(int i = 0; i < 512; i++)            {            }            foreach (var item in spectrum)            {                s += $"{item.Key} \t {item.Value} \n";            }            File.Delete("spectre.txt");
            File.WriteAllText("spectre.txt", s, Encoding.ASCII);
        }
    }
}
