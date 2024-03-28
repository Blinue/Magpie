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
if len(sys.argv) == 2:
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
commit_id = str(p.stdout, encoding="utf-8")[0:-1]

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
    f'"{msbuildPath}" -restore -p:RestorePackagesConfig=true;Configuration=Release;Platform={platform};OutDir={os.getcwd()}\\publish\\{platform}\\;CommitId={commit_id}{version_props} Magpie.sln'
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


shutil.rmtree("Microsoft.UI.Xaml", ignore_errors=True)

for pattern in ["*.pdb", "*.lib", "*.exp", "*.winmd", "*.xml", "*.xbf"]:
    for file in glob.glob(pattern):
        remove_file(file)

for file in glob.glob("*.pri"):
    if file != "resources.pri":
        remove_file(file)

remove_file("Microsoft.Web.WebView2.Core.dll")

print("清理完毕", flush=True)

#####################################################################
#
# 修剪 resources.pri
# 参考自 https://github.com/microsoft/microsoft-ui-xaml/pull/4400
#
#####################################################################

# 取最新的 Windows SDK
windowsSdkDir = max(glob.glob(programFilesX86Path + "\\Windows Kits\\10\\bin\\10.*"))
makepriPath = windowsSdkDir + "\\x64\\makepri.exe"
if not os.access(makepriPath, os.X_OK):
    raise Exception("未找到 makepri")

# 将 resources.pri 的内容导出为 xml
p = subprocess.run(f'"{makepriPath}" dump /dt detailed /o')
if p.returncode != 0:
    raise Exception("dump 失败")

xmlTree = ElementTree.parse("resources.pri.xml")

# 在 xml 中删除冗余资源
for resourceNode in xmlTree.getroot().findall(
    "ResourceMap/ResourceMapSubtree/ResourceMapSubtree/ResourceMapSubtree/NamedResource"
):
    name = resourceNode.get("name")

    if not name.endswith(".xbf"):
        continue

    # 我们仅需 19h1 和 21h1 的资源，分别用于 Win10 和 Win11
    # 小写 compact 仅存在于预发行版 WinUI 的资源中
    for key in ["compact", "Compact", "v1", "rs2", "rs3", "rs4", "rs5"]:
        if key in name:
            # 将文件内容替换为一个空格（Base64 为 "IA=="）
            resourceNode.find("Candidate/Base64Value").text = "IA=="
            break

xmlTree.write("resources.pri.xml", encoding="utf-8")
xmlTree = None

with open("priconfig.xml", "w", encoding="utf-8") as f:
    print(
        """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<resources targetOsVersion="10.0.0" majorVersion="1">
  <packaging>
    <autoResourcePackage qualifier="Scale" />
    <autoResourcePackage qualifier="DXFeatureLevel" />
  </packaging>

  <index startIndexAt="resources.pri.xml" root="">
    <default>
      <qualifier name="Language" value="en-US" />
      <qualifier name="Contrast" value="standard" />
      <qualifier name="Scale" value="200" />
      <qualifier name="HomeRegion" value="001" />
      <qualifier name="TargetSize" value="256" />
      <qualifier name="LayoutDirection" value="LTR" />
      <qualifier name="DXFeatureLevel" value="DX9" />
      <qualifier name="Configuration" value="" />
      <qualifier name="AlternateForm" value="" />
      <qualifier name="Platform" value="UAP" />
    </default>
    <indexer-config type="priinfo" emitStrings="true" emitPaths="true" emitEmbeddedData="true" />
  </index>
</resources>""",
        file=f,
    )

# 将 xml 重新封装成 pri
p = subprocess.run(f'"{makepriPath}" new /pr . /cf priconfig.xml /in Magpie.App /o')
if p.returncode != 0:
    raise Exception("makepri 失败")

os.remove("resources.pri.xml")
os.remove("priconfig.xml")

print("已修剪 resources.pri", flush=True)
