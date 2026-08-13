using System;
using System.Text;
using System.Threading;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;

namespace VbxCaptureHarness
{
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }

    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X; public int Y; }

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

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr OpenDesktop(string lpszDesktop, int dwFlags, bool fInherit, uint dwDesiredAccess);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool SetThreadDesktop(IntPtr hDesktop);

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

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll")]
        public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

        public const uint SWP_NOZORDER = 0x0004;
        public const uint SWP_NOACTIVATE = 0x0010;
        public const uint SWP_NOMOVE = 0x0002;

        // Resizes the window in place (keeps its current top-left) via
        // SetWindowPos -- this is a normal window-management API, distinct
        // from SendInput, and does not touch the operator's real desktop
        // since it targets a specific off-screen HWND directly. Used to
        // reach narrow-width responsive layouts without needing real user
        // interaction (there is no "drag the edge" gesture to simulate
        // headlessly that Qt would recognise as a resize any more
        // reliably than this).
        public static bool ResizeWindow(long handle, int width, int height)
        {
            IntPtr hWnd = new IntPtr(handle);
            return SetWindowPos(hWnd, IntPtr.Zero, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
        }

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

        // ---------------------------------------------------------------
        // Background input: WM_* message constants and virtual-key codes
        // used to drive a window on the off-screen desktop WITHOUT ever
        // touching the operator's real cursor, keyboard focus, or
        // foreground window (SendInput/mouse_event/keybd_event all act on
        // the real INPUT DESKTOP; PostMessage/SendMessage instead target a
        // specific HWND's message queue directly, which is what lets this
        // stay fully off-screen).
        // ---------------------------------------------------------------
        public const uint WM_MOUSEMOVE = 0x0200;
        public const uint WM_LBUTTONDOWN = 0x0201;
        public const uint WM_LBUTTONUP = 0x0202;
        public const uint WM_KEYDOWN = 0x0100;
        public const uint WM_KEYUP = 0x0101;
        public const uint WM_CHAR = 0x0102;
        public const uint WM_SETFOCUS = 0x0007;
        public const int MK_LBUTTON = 0x0001;

        public const int VK_SHIFT = 0x10;
        public const int VK_CONTROL = 0x11;
        public const int VK_MENU = 0x12; // Alt
        public const int VK_ESCAPE = 0x1B;
        public const int VK_RETURN = 0x0D;
        public const int VK_TAB = 0x09;
        public const int VK_F1 = 0x70;

        // A window's HWND is only reliably usable by a process/thread that
        // is itself attached to that window's desktop -- observed directly
        // while building the driven-input path: a brand-new process
        // deserializing a previously-discovered HWND value and calling
        // GetWindowRect/GetClassName on it straight away got back
        // false/0 with no useful error, even though the SAME handle worked
        // fine from the ORIGINAL process that ran EnumDesktopWindows and
        // PrintWindow against it. OpenDesktop (by name) + SetThreadDesktop
        // gives THIS thread its own handle to, and attachment to, that
        // off-screen desktop, after which the same HWND values resolve
        // correctly. Returns the opened desktop handle so the caller can
        // close it when done (a short-lived per-call handle, separate from
        // the long-lived one the session-holder process keeps open).
        public static IntPtr AttachToDesktop(string desktopName, out bool threadAttached, out string warning)
        {
            threadAttached = false;
            warning = null;
            IntPtr h = OpenDesktop(desktopName, 0, false, DESKTOP_ALL_ACCESS);
            if (h == IntPtr.Zero)
                throw new InvalidOperationException("OpenDesktop('" + desktopName + "') failed, Win32Error=" + Marshal.GetLastWin32Error());
            // SetThreadDesktop fails with ERROR_BUSY (170) if the calling
            // thread already owns any window/hook on ITS current desktop --
            // which a hosting process (e.g. PowerShell's own console/COM
            // machinery) routinely does before this ever runs. That does
            // NOT mean OpenDesktop's handle is useless: merely holding an
            // open, access-granted handle to the target desktop is what
            // actually mattered for GetWindowRect/GetClassName/PrintWindow
            // to resolve the HWND correctly from a fresh process (this was
            // proven empirically while building this path) -- so a failed
            // SetThreadDesktop is reported as a warning, not a hard error.
            if (!SetThreadDesktop(h))
            {
                warning = "SetThreadDesktop failed, Win32Error=" + Marshal.GetLastWin32Error() + " (continuing with the OpenDesktop handle alone)";
            }
            else
            {
                threadAttached = true;
            }
            return h;
        }

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

        private static IntPtr MakeLParam(int x, int y)
        {
            return new IntPtr((y << 16) | (x & 0xFFFF));
        }

        // Returns the (X,Y) offset of the window's client-area origin from
        // its own window-rect origin -- i.e. how far a pixel coordinate
        // measured against a PrintWindow capture (which paints starting at
        // the WINDOW rect's top-left, including any non-client area) must
        // be shifted to land on the same point in CLIENT coordinates, which
        // is what WM_MOUSEMOVE/WM_LBUTTONDOWN/WM_LBUTTONUP lParam expects.
        // For this application's frameless Material title bar the offset is
        // expected to be (0,0) -- there is no OS-drawn caption/border to
        // account for -- but it is computed for real rather than assumed,
        // since a future dialog or a differently-styled window may not be
        // frameless.
        public static void GetClientOffset(long handle, out int offsetX, out int offsetY)
        {
            IntPtr hWnd = new IntPtr(handle);
            RECT wr;
            GetWindowRect(hWnd, out wr);
            POINT origin = new POINT { X = 0, Y = 0 };
            ClientToScreen(hWnd, ref origin);
            offsetX = origin.X - wr.Left;
            offsetY = origin.Y - wr.Top;
        }

        // Clicks a point expressed in the SAME pixel space as a PrintWindow
        // capture of this window (i.e. the coordinates an operator would
        // read directly off the saved PNG), translated internally to the
        // client coordinates the posted mouse messages require. Delivered
        // entirely via PostMessage to this specific HWND's queue -- never
        // SendInput/mouse_event, which would require this desktop to be the
        // real input desktop and would move the operator's actual cursor.
        // Qt's hover/press visual state (used by Material-style ripple and
        // list-item delegates) tracks real WM_MOUSEMOVE traffic -- a single
        // WM_MOUSEMOVE that teleports straight to the target, immediately
        // followed by WM_LBUTTONDOWN, was observed to be unreliable (some
        // clicks simply had no effect at all) while a click preceded by a
        // short walk of intermediate WM_MOUSEMOVE steps from a neutral
        // point, plus a held-down press before release, was reliable. This
        // mirrors what a real mouse actually generates -- many WM_MOUSEMOVE
        // messages arriving before the button transition, not one.
        public static void ClickWindowPoint(long handle, int imgX, int imgY, int settleMs)
        {
            IntPtr hWnd = new IntPtr(handle);
            int offsetX, offsetY;
            GetClientOffset(handle, out offsetX, out offsetY);
            int clientX = imgX - offsetX;
            int clientY = imgY - offsetY;

            PostMessage(hWnd, WM_SETFOCUS, IntPtr.Zero, IntPtr.Zero);
            Thread.Sleep(settleMs);

            // Walk from a neutral top-left-ish point toward the target over
            // several steps so intermediate hover state has a chance to
            // update, exactly as continuous real mouse movement would.
            int steps = 6;
            int startX = Math.Max(0, clientX - 40);
            int startY = Math.Max(0, clientY - 40);
            for (int i = 1; i <= steps; i++)
            {
                int ix = startX + (clientX - startX) * i / steps;
                int iy = startY + (clientY - startY) * i / steps;
                PostMessage(hWnd, WM_MOUSEMOVE, IntPtr.Zero, MakeLParam(ix, iy));
                Thread.Sleep(Math.Max(15, settleMs / 6));
            }

            IntPtr finalLParam = MakeLParam(clientX, clientY);
            // One more move exactly on target, then let it settle before
            // pressing -- gives hover/ripple state time to catch up.
            PostMessage(hWnd, WM_MOUSEMOVE, IntPtr.Zero, finalLParam);
            Thread.Sleep(settleMs);
            PostMessage(hWnd, WM_LBUTTONDOWN, new IntPtr(MK_LBUTTON), finalLParam);
            // Hold the press briefly -- some Material ripple/press-state
            // logic distinguishes a real press (which has non-zero
            // duration) from an instantaneous down+up pair.
            Thread.Sleep(Math.Max(settleMs, 150));
            PostMessage(hWnd, WM_MOUSEMOVE, new IntPtr(MK_LBUTTON), finalLParam);
            Thread.Sleep(Math.Max(30, settleMs / 4));
            PostMessage(hWnd, WM_LBUTTONUP, IntPtr.Zero, finalLParam);
            Thread.Sleep(settleMs);
        }

        // Presses and releases a full chord of virtual-key codes against
        // this HWND's message queue: every modifier DOWN first (in the
        // order given), then the main key DOWN/UP, then every modifier UP
        // in REVERSE order -- e.g. {VK_CONTROL, VK_SHIFT}, 'F' reproduces
        // Ctrl+Shift+F. Windows maintains GetKeyState()'s per-thread key
        // table from the messages that thread actually retrieves off its
        // OWN queue (not global hardware state, which is GetAsyncKeyState),
        // so posting the modifier-down messages before the main key and
        // giving the target's message loop time to pump each one is what
        // lets a window that reads GetKeyState() for modifiers see the
        // chord as genuinely held, even though nothing touched the real
        // keyboard.
        public static void SendKeyChord(long handle, int[] modifierVks, int mainVk, int settleMs)
        {
            IntPtr hWnd = new IntPtr(handle);
            PostMessage(hWnd, WM_SETFOCUS, IntPtr.Zero, IntPtr.Zero);
            Thread.Sleep(settleMs);
            foreach (int vk in modifierVks)
            {
                PostMessage(hWnd, WM_KEYDOWN, new IntPtr(vk), IntPtr.Zero);
                Thread.Sleep(settleMs);
            }
            PostMessage(hWnd, WM_KEYDOWN, new IntPtr(mainVk), IntPtr.Zero);
            Thread.Sleep(settleMs);
            PostMessage(hWnd, WM_KEYUP, new IntPtr(mainVk), IntPtr.Zero);
            Thread.Sleep(settleMs);
            for (int i = modifierVks.Length - 1; i >= 0; i--)
            {
                PostMessage(hWnd, WM_KEYUP, new IntPtr(modifierVks[i]), IntPtr.Zero);
                Thread.Sleep(settleMs);
            }
        }

        // Single unmodified key press (Escape, Enter, Tab, ...).
        public static void SendKeyPress(long handle, int vk, int settleMs)
        {
            IntPtr hWnd = new IntPtr(handle);
            PostMessage(hWnd, WM_KEYDOWN, new IntPtr(vk), IntPtr.Zero);
            Thread.Sleep(settleMs);
            PostMessage(hWnd, WM_KEYUP, new IntPtr(vk), IntPtr.Zero);
            Thread.Sleep(settleMs);
        }

        // Types literal text one WM_CHAR at a time.
        public static void SendText(long handle, string text, int settleMs)
        {
            IntPtr hWnd = new IntPtr(handle);
            foreach (char c in text)
            {
                PostMessage(hWnd, WM_CHAR, new IntPtr(c), IntPtr.Zero);
                Thread.Sleep(settleMs);
            }
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
