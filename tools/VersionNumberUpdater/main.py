from os import path
import glob
import re

versionNumber = "0.6.0.0"

rootDir = '..\\..'

csProjects = ["CursorHook", "Magpie"]
cppProjects = ["Runtime"]
cppProjects.extend(map(lambda d: path.basename(d), glob.iglob(rootDir + '\\MODULE_*')))

for csProject in csProjects:
    with open(rootDir + '\\' + csProject + '\\Properties\\AssemblyInfo.cs', mode='r+', encoding='utf8') as f:
        src = f.read()

        src = re.sub(r'AssemblyVersion\([^*]*?\)', 'AssemblyVersion(\"' + versionNumber + '\")', src)
        src = re.sub(r'AssemblyFileVersion\([^*]*?\)', 'AssemblyFileVersion(\"' + versionNumber + '\")', src)

        f.seek(0)
        f.truncate()
        f.write(src)

