import sys
import os
import subprocess
import glob
import shutil
from xml.etree import ElementTree

majorVersion = None
try:
    # https://docs.github.com/en/actions/learn-github-actions/variables
    if os.environ["GITHUB_ACTIONS"].lower() == "true":
        # 不知为何在 Github Actions 中运行时默认编码为 ANSI，并且 print 需刷新流才能正常显示
        for stream in [sys.stdout, sys.stderr]:
            stream.reconfigure(encoding="utf-8")

        # 存在 MAJOR 环境变量则发布新版本
        majorVersion = os.environ["MAJOR"]
except:
    pass

platform = "x64"
if len(sys.argv) >= 2:
    platform = sys.argv[1]
    if not platform in ["x64", "ARM64"]:
        raise Exception("非法参数")

if majorVersion != None:
    import re

    minorVersion = os.environ["MINOR"]
    patchVersion = os.environ["PATCH"]
    tag = os.environ["TAG"]

#####################################################################
#
# 使用 vswhere 查找 msbuild
#
#####################################################################

programFilesX86Path = os.environ["ProgramFiles(x86)"]
vswherePath = programFilesX86Path + "\\Microsoft Visual Studio\\Installer\\vswhere.exe"
if not os.access(vswherePath, os.X_OK):
    raise Exception("未找到 vswhere")

p = subprocess.run(
    vswherePath
    + " -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe",
    capture_output=True,
)
msbuildPath = str(p.stdout, encoding="utf-8").splitlines()[0]
if not os.access(msbuildPath, os.X_OK):
    raise Exception("未找到 msbuild")

#####################################################################
#
# 编译
#
#####################################################################

os.chdir(os.path.dirname(__file__))

p = subprocess.run("git rev-parse --short HEAD", capture_output=True)
commitId = str(p.stdout, encoding="utf-8")[0:-1]

if majorVersion != None:
    version_props = f";MajorVersion={majorVersion};MinorVersion={minorVersion};PatchVersion={patchVersion};VersionTag={tag}"

    # 更新 RC 文件中的版本号
    version = f"{majorVersion}.{minorVersion}.{patchVersion}.0"
    version_comma = version.replace(".", ",")
    for project in os.listdir("src"):
        rcPath = f"src\\{project}\\{project}.rc"
        if not os.access(rcPath, os.R_OK | os.W_OK):
            continue

        with open(rcPath, mode="r+", encoding="utf-8") as f:
            src = f.read()

            src = re.sub(
                r"FILEVERSION .*?\n", "FILEVERSION " + version_comma + "\n", src
            )
            src = re.sub(
                r"PRODUCTVERSION .*?\n", "PRODUCTVERSION " + version_comma + "\n", src
            )
            src = re.sub(
                r'"FileVersion", *?".*?"\n', '"FileVersion", "' + version + '"\n', src
            )
            src = re.sub(
                r'"ProductVersion", *?".*?"\n',
                '"ProductVersion", "' + version + '"\n',
                src,
            )

            f.seek(0)
            f.truncate()
            f.write(src)
else:
    version_props = ""

p = subprocess.run(
    f'"{msbuildPath}" -restore -p:RestorePackagesConfig=true;Configuration=Release;Platform={platform};OutDir={os.getcwd()}\\publish\\{platform}\\;CommitId={commitId}{version_props} Magpie.sln'
)
if p.returncode != 0:
    raise Exception("编译失败")

#####################################################################
#
# 清理不需要的文件
#
#####################################################################

os.chdir("publish\\" + platform)


# 删除文件，忽略错误
def remove_file(file):
    try:
        os.remove(file)
    except:
        pass


for pattern in ["*.pdb", "*.lib", "*.exp"]:
    for file in glob.glob(pattern):
        remove_file(file)

print("清理完毕", flush=True)

#####################################################################
#
# 为 TouchHelper 签名
#
#####################################################################

if len(sys.argv) >= 5 and sys.argv[4] != "":
    # sys.argv[2] 保留为打包选项
    pfxPath = os.path.join("..\\..", sys.argv[3])
    pfxPassword = sys.argv[4]

    # 取最新的 Windows SDK
    windowsSdkDir = max(
        glob.glob(programFilesX86Path + "\\Windows Kits\\10\\bin\\10.*")
    )
    p = subprocess.run(
        f'"{windowsSdkDir}\\x64\\signtool.exe" sign /fd SHA256 /a /f "{pfxPath}" /p "{pfxPassword}" TouchHelper.exe'
    )
    if p.returncode != 0:
        raise Exception("签名失败")
