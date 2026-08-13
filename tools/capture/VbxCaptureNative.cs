using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;

namespace VbxCaptureHarness
{
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct STARTUPINFO
    {
        public int cb;
        public string lpReserved;
        public string lpDesktop;
        public string lpTitle;
        public int dwX;
        public int dwY;
        public int dwXSize;
        public int dwYSize;
        public int dwXCountChars;
        public int dwYCountChars;
        public int dwFillAttribute;
        public int dwFlags;
        public short wShowWindow;
        public short cbReserved2;
        public IntPtr lpReserved2;
        public IntPtr hStdInput;
        public IntPtr hStdOutput;
        public IntPtr hStdError;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PROCESS_INFORMATION
    {
        public IntPtr hProcess;
        public IntPtr hThread;
        public int dwProcessId;
        public int dwThreadId;
    }

    public class WindowInfo
    {
        public long Handle;
        public int Pid;
        public string ClassName;
        public string Title;
        public int X, Y, Width, Height;
        public bool Visible;
    }

    public delegate bool EnumDesktopWindowsProc(IntPtr hWnd, IntPtr lParam);

    public static class Native
    {
        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr CreateDesktop(string lpszDesktop, IntPtr lpszDevice, IntPtr pDevmode, int dwFlags, uint dwDesiredAccess, IntPtr lpsa);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool CloseDesktop(IntPtr hDesktop);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool EnumDesktopWindows(IntPtr hDesktop, EnumDesktopWindowsProc lpfn, IntPtr lParam);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern int GetWindowTextLength(IntPtr hWnd);

        [DllImport("user32.dll")]
        public static extern bool IsWindowVisible(IntPtr hWnd);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out int lpdwProcessId);

        [DllImport("user32.dll")]
        public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

        [DllImport("user32.dll")]
        public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);

        [DllImport("user32.dll")]
        public static extern IntPtr GetDC(IntPtr hWnd);

        [DllImport("user32.dll")]
        public static extern int ReleaseDC(IntPtr hWnd, IntPtr hDC);

        [DllImport("gdi32.dll")]
        public static extern IntPtr CreateCompatibleDC(IntPtr hdc);

        [DllImport("gdi32.dll")]
        public static extern IntPtr CreateCompatibleBitmap(IntPtr hdc, int nWidth, int nHeight);

        [DllImport("gdi32.dll")]
        public static extern IntPtr SelectObject(IntPtr hdc, IntPtr hgdiobj);

        [DllImport("gdi32.dll")]
        public static extern bool DeleteObject(IntPtr hObject);

        [DllImport("gdi32.dll")]
        public static extern bool DeleteDC(IntPtr hdc);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern bool CreateProcess(
            string lpApplicationName,
            StringBuilder lpCommandLine,
            IntPtr lpProcessAttributes,
            IntPtr lpThreadAttributes,
            bool bInheritHandles,
            uint dwCreationFlags,
            IntPtr lpEnvironment,
            string lpCurrentDirectory,
            ref STARTUPINFO lpStartupInfo,
            out PROCESS_INFORMATION lpProcessInformation);

        [DllImport("kernel32.dll")]
        public static extern bool CloseHandle(IntPtr hObject);

        public const uint DESKTOP_ALL_ACCESS = 0x000F01FF;
        public const uint PW_RENDERFULLCONTENT = 0x00000002;
        public const int STARTF_USESHOWWINDOW = 0x00000001;
        public const short SW_SHOWNORMAL = 1;
        public const uint CREATE_NEW_PROCESS_GROUP = 0x00000200;

        public static IntPtr CreateHeadlessDesktop(string name)
        {
            IntPtr h = CreateDesktop(name, IntPtr.Zero, IntPtr.Zero, 0, DESKTOP_ALL_ACCESS, IntPtr.Zero);
            if (h == IntPtr.Zero)
                throw new InvalidOperationException("CreateDesktop failed, Win32Error=" + Marshal.GetLastWin32Error());
            return h;
        }

        public static int LaunchOnDesktop(string desktopFullName, string commandLine)
        {
            STARTUPINFO si = new STARTUPINFO();
            si.cb = Marshal.SizeOf(typeof(STARTUPINFO));
            si.lpDesktop = desktopFullName;
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_SHOWNORMAL;
            PROCESS_INFORMATION pi;
            StringBuilder cmd = new StringBuilder(commandLine, commandLine.Length + 64);
            bool ok = CreateProcess(null, cmd, IntPtr.Zero, IntPtr.Zero, false,
                CREATE_NEW_PROCESS_GROUP, IntPtr.Zero, null, ref si, out pi);
            if (!ok)
                throw new InvalidOperationException("CreateProcess failed, Win32Error=" + Marshal.GetLastWin32Error());
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return pi.dwProcessId;
        }

        public static List<WindowInfo> ListDesktopWindows(IntPtr hDesktop)
        {
            List<WindowInfo> results = new List<WindowInfo>();
            EnumDesktopWindowsProc proc = delegate(IntPtr hWnd, IntPtr lParam)
            {
                StringBuilder sbClass = new StringBuilder(256);
                GetClassName(hWnd, sbClass, sbClass.Capacity);
                int len = GetWindowTextLength(hWnd);
                StringBuilder sbTitle = new StringBuilder(Math.Max(len, 0) + 1);
                if (len > 0) GetWindowText(hWnd, sbTitle, sbTitle.Capacity);
                int pid;
                GetWindowThreadProcessId(hWnd, out pid);
                RECT r;
                GetWindowRect(hWnd, out r);
                WindowInfo wi = new WindowInfo();
                wi.Handle = hWnd.ToInt64();
                wi.Pid = pid;
                wi.ClassName = sbClass.ToString();
                wi.Title = sbTitle.ToString();
                wi.X = r.Left;
                wi.Y = r.Top;
                wi.Width = r.Right - r.Left;
                wi.Height = r.Bottom - r.Top;
                wi.Visible = IsWindowVisible(hWnd);
                results.Add(wi);
                return true;
            };
            EnumDesktopWindows(hDesktop, proc, IntPtr.Zero);
            GC.KeepAlive(proc);
            return results;
        }

        // Captures the window (client + non-client area) via PrintWindow into a
        // freshly created bitmap and saves it as PNG. Returns false (with
        // `error` set) on any Win32-level failure; the caller must still treat
        // a `true` result as merely "PrintWindow said it painted something" --
        // NOT as proof the pixels are legible. Use IsImageUniform for that.
        public static bool CaptureWindow(long handle, string outputPath, out int width, out int height, out string error)
        {
            width = 0; height = 0; error = null;
            IntPtr hWnd = new IntPtr(handle);
            RECT r;
            if (!GetWindowRect(hWnd, out r))
            {
                error = "GetWindowRect failed, Win32Error=" + Marshal.GetLastWin32Error();
                return false;
            }
            width = r.Right - r.Left;
            height = r.Bottom - r.Top;
            if (width <= 0 || height <= 0)
            {
                error = "window has non-positive size (" + width + "x" + height + ")";
                return false;
            }

            IntPtr hdcScreen = GetDC(IntPtr.Zero);
            IntPtr hdcMem = IntPtr.Zero;
            IntPtr hBitmap = IntPtr.Zero;
            bool printed = false;
            try
            {
                hdcMem = CreateCompatibleDC(hdcScreen);
                hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
                IntPtr hOld = SelectObject(hdcMem, hBitmap);
                printed = PrintWindow(hWnd, hdcMem, PW_RENDERFULLCONTENT);
                if (!printed)
                {
                    error = "PrintWindow returned FALSE, Win32Error=" + Marshal.GetLastWin32Error();
                }
                SelectObject(hdcMem, hOld);

                using (Bitmap bmp = Image.FromHbitmap(hBitmap))
                {
                    bmp.Save(outputPath, ImageFormat.Png);
                }
            }
            finally
            {
                if (hBitmap != IntPtr.Zero) DeleteObject(hBitmap);
                if (hdcMem != IntPtr.Zero) DeleteDC(hdcMem);
                if (hdcScreen != IntPtr.Zero) ReleaseDC(IntPtr.Zero, hdcScreen);
            }
            return printed;
        }

        // A freshly allocated GDI bitmap is uninitialised memory, which reads
        // back as solid black (or, with an unset alpha channel, as solid black
        // through a PNG viewer too). PrintWindow can return TRUE while having
        // painted nothing into it -- e.g. a window whose owner never handled
        // WM_PRINT/WM_PRINTCLIENT for the requested content. So this reads the
        // saved PNG back and samples it on a grid; if every sampled pixel is
        // the exact same colour, the image is reported uniform (i.e. suspect)
        // rather than trusted.
        public static bool IsImageUniform(string path, int gridSize, out int distinctSampleColors, out int totalSamples)
        {
            distinctSampleColors = 0;
            totalSamples = 0;
            using (Bitmap bmp = new Bitmap(path))
            {
                HashSet<int> seen = new HashSet<int>();
                int w = bmp.Width, h = bmp.Height;
                for (int gx = 0; gx < gridSize; gx++)
                {
                    for (int gy = 0; gy < gridSize; gy++)
                    {
                        int px = Math.Min(w - 1, (int)((gx + 0.5) * w / gridSize));
                        int py = Math.Min(h - 1, (int)((gy + 0.5) * h / gridSize));
                        Color c = bmp.GetPixel(px, py);
                        seen.Add(c.ToArgb());
                        totalSamples++;
                    }
                }
                distinctSampleColors = seen.Count;
            }
            return distinctSampleColors <= 1;
        }
    }
}
