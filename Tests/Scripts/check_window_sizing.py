"""Check native viewer sizing across attached monitors on Windows 10 or later.

Usage: python Tests/Scripts/check_window_sizing.py path/to/OIViewer.exe
Requires an interactive desktop and the default 1/2/3/4 key bindings.
Only the viewer process launched by this script is resized and terminated.
"""

import argparse
import sys
import ctypes as c
from ctypes import wintypes as w
import json
import math
from pathlib import Path
import subprocess
import time

if sys.platform != 'win32':
    raise SystemExit('This native sizing check requires Windows')
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('executable', type=Path)
executable = parser.parse_args().executable.resolve(strict=True)
u = c.WinDLL('user32', use_last_error=True)
dwm = c.WinDLL('dwmapi')
u.SetProcessDpiAwarenessContext(c.c_void_p(-4))

class Monitor(c.Structure):
    _fields_ = [('size', w.DWORD), ('screen', w.RECT), ('work', w.RECT), ('flags', w.DWORD)]
wc = c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
mc = c.WINFUNCTYPE(w.BOOL, w.HMONITOR, w.HDC, c.POINTER(w.RECT), w.LPARAM)
u.EnumWindows.argtypes = [wc, w.LPARAM]
u.EnumDisplayMonitors.argtypes = [w.HDC, c.POINTER(w.RECT), mc, w.LPARAM]
u.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]
u.IsWindowVisible.argtypes = [w.HWND]
u.GetClientRect.argtypes = [w.HWND, c.POINTER(w.RECT)]
u.GetWindowRect.argtypes = [w.HWND, c.POINTER(w.RECT)]
u.GetMonitorInfoW.argtypes = [w.HMONITOR, c.POINTER(Monitor)]
u.GetDpiForWindow.argtypes = [w.HWND]
u.GetDpiForWindow.restype = w.UINT
u.SetWindowPos.argtypes = [w.HWND, w.HWND, c.c_int, c.c_int, c.c_int, c.c_int, w.UINT]
u.ShowWindow.argtypes = [w.HWND, c.c_int]
u.SendMessageTimeoutW.argtypes = [w.HWND, w.UINT, w.WPARAM, w.LPARAM, w.UINT, w.UINT, c.POINTER(c.c_size_t)]
u.SendMessageTimeoutW.restype = w.LPARAM
u.IsZoomed.argtypes = [w.HWND]
u.MonitorFromWindow.argtypes = [w.HWND, w.DWORD]
u.MonitorFromWindow.restype = w.HMONITOR
dwm.DwmGetWindowAttribute.argtypes = [w.HWND, w.DWORD, c.c_void_p, w.DWORD]
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
SW_RESTORE = 9
MONITOR_DEFAULTTONEAREST = 2
DWMWA_EXTENDED_FRAME_BOUNDS = 9
SWP_NOZORDER = 0x0004
SWP_NOACTIVATE = 0x0010
KEY_UP_LPARAM = 0xC0000001
monitors = []

@mc
def enum_monitor(h, dc, r, d):
    info = Monitor()
    info.size = c.sizeof(info)
    assert u.GetMonitorInfoW(h, c.byref(info))
    monitors.append((h, info))
    return True
assert u.EnumDisplayMonitors(None, None, enum_monitor, 0) and monitors

def find_window(pid):
    windows = []

    @wc
    def visit(h, d):
        owner = w.DWORD()
        u.GetWindowThreadProcessId(h, c.byref(owner))
        if owner.value == pid and u.IsWindowVisible(h):
            windows.append(h)
        return True
    u.EnumWindows(visit, 0)
    return windows[0] if windows else None

def rect(r):
    return [r.left, r.top, r.right, r.bottom]

def key(h, k):
    result = c.c_size_t()
    for message, data in ((WM_KEYDOWN, 1), (WM_KEYUP, KEY_UP_LPARAM)):
        assert u.SendMessageTimeoutW(h, message, ord(k), data, 2, 5000, c.byref(result)), k
    time.sleep(0.1)
p = subprocess.Popen([str(executable)], cwd=executable.parent)
try:
    h = None
    for _ in range(100):
        h = find_window(p.pid)
        if h:
            break
        assert p.poll() is None
        time.sleep(0.1)
    assert h
    time.sleep(1)
    for index, (monitor, info) in enumerate(monitors):
        u.ShowWindow(h, SW_RESTORE)
        assert u.SetWindowPos(h, None, info.work.left + 100, info.work.top + 100, 800, 600, SWP_NOZORDER | SWP_NOACTIVATE)
        time.sleep(0.6)
        for k in ['3', '3', '2', '1', '4', '3']:
            key(h, k)
            frame = w.RECT()
            client = w.RECT()
            outer = w.RECT()
            assert dwm.DwmGetWindowAttribute(h, DWMWA_EXTENDED_FRAME_BOUNDS, c.byref(frame), c.sizeof(frame)) == 0
            assert u.GetClientRect(h, c.byref(client))
            assert u.GetWindowRect(h, c.byref(outer))
            record = {
                'monitor': index,
                'key': k,
                'dpi': u.GetDpiForWindow(h),
                'work': rect(info.work),
                'frame': rect(frame),
                'outer': rect(outer),
                'client': rect(client),
                'maximized': bool(u.IsZoomed(h)),
                'same_monitor': u.MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST) == monitor,
            }
            if k in ['1', '2']:
                fraction = 0.25 if k == '1' else 0.5
                expected_width = (info.work.right - info.work.left) * fraction
                expected_height = (info.work.bottom - info.work.top) * fraction
                pixel_tolerance = math.ceil(record['dpi'] / 96)
                assert abs(client.right - expected_width) <= pixel_tolerance, record
                assert abs(client.bottom - expected_height) <= pixel_tolerance, record
                assert not record['maximized'] and record['same_monitor'], record
            elif k == '3':
                assert all((abs(a - b) <= 2 for a, b in zip(record['frame'], record['work']))), record
                assert record['maximized'] and record['same_monitor'], record
            else:
                assert all(abs(a - b) <= 2 for a, b in zip(record['frame'], rect(info.screen))), record
                assert record['same_monitor'], record
            print(json.dumps(record), flush=True)
    if len(monitors) > 1:
        for index, (source_monitor, source_info) in enumerate(monitors):
            source_sizes = (
                (800, 600),
                (round((source_info.work.right - source_info.work.left) * 0.8),
                 round((source_info.work.bottom - source_info.work.top) * 0.8)),
            )
            for source_width, source_height in source_sizes:
                for start_maximized in (False, True):
                    u.ShowWindow(h, SW_RESTORE)
                    assert u.SetWindowPos(
                        h, None, source_info.work.left + 100, source_info.work.top + 100,
                        source_width, source_height, SWP_NOZORDER | SWP_NOACTIVATE,
                    )
                    time.sleep(0.6)
                    original_client = w.RECT()
                    assert u.GetClientRect(h, c.byref(original_client))
                    original_scale = u.GetDpiForWindow(h) / 96
                    original_logical = (round(original_client.right / original_scale), round(original_client.bottom / original_scale))
                    if start_maximized:
                        key(h, '3')
                    key(h, '4')
                    target_monitor, target_info = monitors[(index + 1) % len(monitors)]
                    screen = target_info.screen
                    assert u.SetWindowPos(
                        h, None, screen.left, screen.top, screen.right - screen.left,
                        screen.bottom - screen.top, SWP_NOZORDER | SWP_NOACTIVATE,
                    )
                    time.sleep(0.6)
                    assert u.MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST) == target_monitor
                    key(h, '3')
                    frame = w.RECT()
                    assert dwm.DwmGetWindowAttribute(h, DWMWA_EXTENDED_FRAME_BOUNDS, c.byref(frame), c.sizeof(frame)) == 0
                    record = {
                        'case': 'fullscreen moved between monitors',
                        'source_monitor': index,
                        'started_maximized': start_maximized,
                        'source_size': [source_width, source_height],
                        'frame': rect(frame),
                        'target_work': rect(target_info.work),
                    }
                    assert u.IsZoomed(h), record
                    assert u.MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST) == target_monitor, record
                    assert all(abs(a - b) <= 2 for a, b in zip(rect(frame), rect(target_info.work))), record
                    print(json.dumps(record), flush=True)
                    u.ShowWindow(h, SW_RESTORE)
                    time.sleep(0.6)
                    client = w.RECT()
                    outer = w.RECT()
                    assert u.GetClientRect(h, c.byref(client))
                    assert u.GetWindowRect(h, c.byref(outer))
                    scale = u.GetDpiForWindow(h) / 96
                    max_width = math.floor((target_info.work.right - target_info.work.left -
                                            (outer.right - outer.left - client.right)) / scale)
                    max_height = math.floor((target_info.work.bottom - target_info.work.top -
                                             (outer.bottom - outer.top - client.bottom)) / scale)
                    record.update(original_logical=original_logical, restored_logical=[client.right / scale, client.bottom / scale], max_logical=[max_width, max_height])
                    assert abs(client.right / scale - min(original_logical[0], max_width)) <= 1, record
                    assert abs(client.bottom / scale - min(original_logical[1], max_height)) <= 1, record
                    assert u.MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST) == target_monitor, record
finally:
    if p.poll() is None:
        p.terminate()
        p.wait(timeout=10)
