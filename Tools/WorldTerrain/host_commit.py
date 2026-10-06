"""Read Windows system commit headroom without launching another process."""
import ctypes
from ctypes import wintypes
import json

class PerformanceInformation(ctypes.Structure):
    _fields_ = [('cb', wintypes.DWORD)] + [(name, ctypes.c_size_t) for name in (
        'CommitTotal', 'CommitLimit', 'CommitPeak', 'PhysicalTotal',
        'PhysicalAvailable', 'SystemCache', 'KernelTotal', 'KernelPaged',
        'KernelNonpaged', 'PageSize')] + [(name, wintypes.DWORD) for name in (
        'HandleCount', 'ProcessCount', 'ThreadCount')]

def system_commit_sample():
    api = ctypes.WinDLL('psapi', use_last_error=True)
    api.GetPerformanceInfo.argtypes = [ctypes.POINTER(PerformanceInformation), wintypes.DWORD]
    api.GetPerformanceInfo.restype = wintypes.BOOL
    data = PerformanceInformation()
    data.cb = ctypes.sizeof(data)
    if not api.GetPerformanceInfo(ctypes.byref(data), data.cb):
        raise ctypes.WinError(ctypes.get_last_error())
    scale = data.PageSize/1048576
    return dict(commit_mib=round(data.CommitTotal*scale, 2),
        limit_mib=round(data.CommitLimit*scale, 2),
        available_commit_mib=round((data.CommitLimit-data.CommitTotal)*scale, 2),
        available_physical_mib=round(data.PhysicalAvailable*scale, 2))

if __name__ == '__main__':
    sample = system_commit_sample()
    assert 0 < sample['commit_mib'] <= sample['limit_mib']
    assert sample['available_commit_mib'] >= 0
    print(json.dumps(sample))
