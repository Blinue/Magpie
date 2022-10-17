"""
用于更新版本号
使用方式: python BumpVersion.py 1.0.0
可选的可以指定字符串版本号: python BumpVersion.py 1.0.100 1.1.0-preview1
"""

import sys
import re

assert len(sys.argv) in [2, 3] and len(sys.argv[1]) > 0

versionNumbers = list(map(lambda s: int(s), sys.argv[1].split(".")))
versionNumbers.extend([0, 0, 0])

version = "%d.%d.%d.%d" % tuple(versionNumbers[0:4])
versionComma = version.replace(".", ",")

if len(sys.argv) == 3 and len(sys.argv[2]) > 0:
    versionStr = sys.argv[2]
else:
    versionStr = "%d.%d.%d" % tuple(versionNumbers[0:3])

srcDir = "..\\src\\"

# 更新 RC 文件
for project in ["Magpie", "Magpie.Core", "Magpie.UI"]:
    with open("{}{}\\{}.rc".format(srcDir, project, project), mode="r+", encoding="utf8") as f:
        src = f.read()

        src = re.sub(r"FILEVERSION .*?\n", "FILEVERSION " + versionComma + "\n", src)
        src = re.sub(r"PRODUCTVERSION .*?\n", "PRODUCTVERSION " + versionComma + "\n", src)
        src = re.sub(r'"FileVersion",[ ]*?".*?"\n', '"FileVersion", "' + version + '"\n', src)
        src = re.sub(r'"ProductVersion",[ ]*?".*?"\n', '"ProductVersion", "' + version + '"\n', src)

        f.seek(0)
        f.truncate()
        f.write(src)

# 更新 Version.h
with open(srcDir + "Shared\\Version.h", mode="r+", encoding="utf8") as f:
    src = f.read()
    src = re.sub(r'MAGPIE_VERSION ".*?"\n', 'MAGPIE_VERSION "' + versionStr + '"\n', src)

    f.seek(0)
    f.truncate()
    f.write(src)
