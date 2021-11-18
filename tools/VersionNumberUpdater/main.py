import re
import sys

assert len(sys.argv) == 2 and len(sys.argv[1]) > 0

versionNumbers = list(map(lambda s: int(s), sys.argv[1].split('.')))
versionNumbers.extend([0, 0, 0])

version = '%d.%d.%d.%d' % tuple(versionNumbers[0:4])
versionComma = version.replace('.', ',')

rootDir = '..\\..'


# Magpie
with open(rootDir + '\\Magpie\\Properties\\AssemblyInfo.cs', mode='r+', encoding='utf8') as f:
    src = f.read()

    src = re.sub(r'AssemblyVersion\([^*]*?\)', 'AssemblyVersion(\"' + version + '\")', src)
    src = re.sub(r'AssemblyFileVersion\([^*]*?\)', 'AssemblyFileVersion(\"' + version + '\")', src)

    f.seek(0)
    f.truncate()
    f.write(src)

# Magpie 的全局变量
with open(rootDir + '\\Magpie\\App.xaml.cs', mode='r+', encoding='utf8') as f:
    src = f.read()
    src = re.sub(r'APP_VERSION = new\(".*?"\)', 'APP_VERSION = new("' + version + '")', src)

    f.seek(0)
    f.truncate()
    f.write(src)

# Runtime
with open(rootDir + '\\Runtime\\Runtime.rc', mode='r+', encoding='utf8') as f:
    src = f.read()

    src = re.sub(r'FILEVERSION .*?\n', 'FILEVERSION ' + versionComma + '\n', src)
    src = re.sub(r'PRODUCTVERSION .*?\n', 'PRODUCTVERSION ' + versionComma + '\n', src)
    src = re.sub(r'"FileVersion",[ ]*?".*?"\n', '"FileVersion", "' + version + '"\n', src)
    src = re.sub(r'"ProductVersion",[ ]*?".*?"\n', '"ProductVersion", "' + version + '"\n', src)

    f.seek(0)
    f.truncate()
    f.write(src)
