import ctypes, sys
from ctypes import windll, wintypes
import uuid
import subprocess
import os
import glob
import shutil
from xml.etree import ElementTree

sys.stdout.reconfigure(encoding='utf-8')

#####################################################################
#
# 使用 vswhere 查找 msbuild
#
#####################################################################

class FOLDERID:
    ProgramFilesX86 = uuid.UUID('{7C5A40EF-A0FB-4BFC-874A-C0F2E0B9FA8E}')

# 包装 SHGetKnownFolderPath，来自 https://gist.github.com/mkropat/7550097
def get_known_folder_path(folderid):
    class GUID(ctypes.Structure):
        _fields_ = [
            ("Data1", wintypes.DWORD),
            ("Data2", wintypes.WORD),
            ("Data3", wintypes.WORD),
            ("Data4", wintypes.BYTE * 8)
        ]

        def __init__(self, uuid_):
            ctypes.Structure.__init__(self)
            self.Data1, self.Data2, self.Data3, self.Data4[0], self.Data4[1], rest = uuid_.fields
            for i in range(2, 8):
                self.Data4[i] = rest>>(8 - i - 1)*8 & 0xff

    CoTaskMemFree = windll.ole32.CoTaskMemFree
    CoTaskMemFree.restype= None
    CoTaskMemFree.argtypes = [ctypes.c_void_p]

    SHGetKnownFolderPath = windll.shell32.SHGetKnownFolderPath
    SHGetKnownFolderPath.argtypes = [
        ctypes.POINTER(GUID), wintypes.DWORD, wintypes.HANDLE, ctypes.POINTER(ctypes.c_wchar_p)
    ] 

    fid = GUID(folderid) 
    pPath = ctypes.c_wchar_p()
    if SHGetKnownFolderPath(ctypes.byref(fid), 0, wintypes.HANDLE(0), ctypes.byref(pPath)) != 0:
        raise FileNotFoundError()
    path = pPath.value
    CoTaskMemFree(pPath)
    return path

programFilesX86Path = get_known_folder_path(FOLDERID.ProgramFilesX86);

vswherePath = programFilesX86Path + "\\Microsoft Visual Studio\\Installer\\vswhere.exe"
if not os.access(vswherePath, os.X_OK):
    raise Exception("未找到 vswhere")

process = subprocess.run(vswherePath + " -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe", capture_output=True)
msbuildPath = str(process.stdout, encoding="utf-8").splitlines()[0]
if not os.access(msbuildPath, os.X_OK):
    raise Exception("未找到 msbuild")

#####################################################################
#
# 编译
#
#####################################################################

os.chdir(os.path.dirname(__file__))

if os.system("\"" + msbuildPath + "\" /p:Configuration=Release;Platform=x64 src\\CONAN_INSTALL") != 0:
    raise Exception("编译 CONAN_INSTALL 失败")

if os.system("\"" + msbuildPath + "\" /p:Configuration=Release;Platform=x64;OutDir=..\\..\\publish\\ src\\Effects") != 0:
    raise Exception("编译 Effects 失败")

if os.system("\"" + msbuildPath + "\" /p:Configuration=Release;Platform=x64;OutDir=..\\..\\publish\\ src\\Magpie.Core") != 0:
    raise Exception("编译 Magpie.Core 失败")

if os.system("\"" + msbuildPath + "\" /p:Configuration=Release;Platform=x64;BuildProjectReferences=false;OutDir=..\\..\\publish\\ src\\Magpie") != 0:
    raise Exception("编译 Magpie 失败")

if os.system("\"" + msbuildPath + "\" /p:Configuration=Release;Platform=x64;OutDir=..\\..\\publish\\ src\\Updater") != 0:
    raise Exception("编译 Updater 失败")

print("编译完成", flush=True)

#####################################################################
#
# 清理不需要的文件
#
#####################################################################

os.chdir(os.getcwd() + "\\publish")

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

print("清理完毕", flush=True)

#####################################################################
#
# 修剪 resources.pri
# 参考自 https://github.com/microsoft/microsoft-ui-xaml/pull/4400
#
#####################################################################

windowsSdkDir = sorted(glob.glob(programFilesX86Path + "\\Windows Kits\\10\\bin\\10.*"))[-1];
makepriPath = windowsSdkDir + "\\x64\\makepri.exe"
if not os.access(makepriPath, os.X_OK):
    raise Exception("未找到 makepri")

if os.system("\"" + makepriPath + "\" dump /dt detailed /o") != 0:
    raise Exception("dump 失败")

xmlTree = ElementTree.parse("resources.pri.xml")

for resourceNode in xmlTree.getroot().findall("ResourceMap/ResourceMapSubtree/ResourceMapSubtree/ResourceMapSubtree/NamedResource"):
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
    print("""<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
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
</resources>""", file=f)

os.system("\"" + makepriPath + "\" new /pr . /cf priconfig.xml /in Magpie.App /o")

os.remove("resources.pri.xml")
os.remove("priconfig.xml")

print("已修剪 resources.pri", flush=True)
