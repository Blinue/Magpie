import sys
import os
import subprocess
import glob
import shutil
from xml.etree import ElementTree

try:
    # https://docs.github.com/en/actions/learn-github-actions/variables
    if os.environ["GITHUB_ACTIONS"].lower() == "true":
        # 不知为何在 Github Actions 中运行时默认编码为 ANSI，并且 print 需刷新流才能正常显示
        for stream in [sys.stdout, sys.stderr]:
            stream.reconfigure(encoding="utf-8")
except:
    pass

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

p = subprocess.run(
    f'"{msbuildPath}" -restore -p:RestorePackagesConfig=true;Configuration=Release;Platform=x64;OutDir={os.getcwd()}\\publish\\ Magpie.sln'
)
if p.returncode != 0:
    raise Exception("编译失败")

#####################################################################
#
# 清理不需要的文件
#
#####################################################################

os.chdir("publish")


# 删除文件，忽略错误
def remove_file(file):
    try:
        os.remove(file)
    except:
        pass


# 删除文件夹，忽略错误
def remove_folder(folder):
    try:
        shutil.rmtree(folder)
    except:
        pass


remove_file("Microsoft.Web.WebView2.Core.dll")

for pattern in ["*.pdb", "*.lib", "*.exp", "*.winmd", "*.xml", "*.xbf", "dummy.*"]:
    for file in glob.glob(pattern):
        remove_file(file)

for file in glob.glob("*.pri"):
    if file != "resources.pri":
        remove_file(file)

for folder in ["Microsoft.UI.Xaml", "Magpie.App"]:
    remove_folder(folder)

print("清理完毕")

#####################################################################
#
# 修剪 resources.pri
# 参考自 https://github.com/microsoft/microsoft-ui-xaml/pull/4400
#
#####################################################################

# 取最新的 Windows SDK
windowsSdkDir = sorted(
    glob.glob(programFilesX86Path + "\\Windows Kits\\10\\bin\\10.*")
)[-1]
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
    for key in ["compact", "Compact", "v1", "rs2", "rs3", "rs4", "rs5"]:
        if key in name:
            # 将文件内容替换为一个空格（Base64 为 "IA=="）
            resourceNode.find("Candidate/Base64Value").text = "IA=="
            break

xmlTree.write("resources.pri.xml", encoding="utf-8")

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

print("已修剪 resources.pri")
