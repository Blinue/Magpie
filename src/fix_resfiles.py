import sys
import os

if len(sys.argv) != 2:
    raise Exception("请勿直接运行此脚本")

with open(sys.argv[1], "r+") as f:
    lines = []
    for line in f.readlines():
        if not "\\packages\\Microsoft.UI.Xaml" in line or "prerelease" in line:
            lines.append(line)

    f.seek(os.SEEK_SET)
    f.truncate()
    f.writelines(lines)
