import sys
import os
import subprocess
import glob
import argparse

try:
    # https://docs.github.com/en/actions/learn-github-actions/variables
    if os.environ["GITHUB_ACTIONS"].lower() == "true":
        # 不知为何在 Github Actions 中运行时默认编码为 ANSI，并且 print 需刷新流才能正常显示
        for stream in [sys.stdout, sys.stderr]:
            stream.reconfigure(encoding="utf-8")
except:
    pass

argParser = argparse.ArgumentParser()
argParser.add_argument("--compiler", choices=["MSVC", "ClangCL"], default="MSVC")
argParser.add_argument("--platform", choices=["x64", "ARM64"], default="x64")
argParser.add_argument("--use-native-march", action="store_true")
argParser.add_argument("--version-major", type=int, default=0)
argParser.add_argument("--version-minor", type=int, default=0)
argParser.add_argument("--version-patch", type=int, default=0)
argParser.add_argument("--version-string", default="")
argParser.add_argument("--pfx-path", default="")
argParser.add_argument("--pfx-password", default="")
args = argParser.parse_args()

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

os.chdir(os.path.dirname(__file__) + "\\..")

p = subprocess.run("git rev-parse --short HEAD", capture_output=True)
commitId = str(p.stdout, encoding="utf-8")[0:-1]

versionNumProps = f";MajorVersion={args.version_major};MinorVersion={args.version_minor};PatchVersion={args.version_patch}"
versionStrProp = "" if args.version_string == "" else f";VersionString={args.version_string}"

p = subprocess.run(
    f'"{msbuildPath}" Magpie.slnx -m -t:Rebuild -restore -p:RestorePackagesConfig=true;Configuration=Release;Platform={args.platform};DisablePDB=true;UseClangCL={args.compiler == "ClangCL"};UseNativeMicroArch={args.use_native_march};OutDir={os.getcwd()}\\publish\\{args.platform}\\;CommitId={commitId}{versionNumProps}{versionStrProp}'
)
if p.returncode != 0:
    raise Exception("编译失败")

#####################################################################
#
# 清理不需要的文件
#
#####################################################################

os.chdir("publish\\" + args.platform)


# 删除文件，忽略错误
def remove_file(file):
    try:
        os.remove(file)
    except:
        pass


for file in glob.glob("*.lib"):
    remove_file(file)

print("清理完毕", flush=True)

#####################################################################
#
# 为 TouchHelper 签名
#
#####################################################################

if args.pfx_path != "":
    pfxPath = os.path.join("..\\..", args.pfx_path)

    # 取最新的 Windows SDK
    windowsSdkDir = max(
        glob.glob(programFilesX86Path + "\\Windows Kits\\10\\bin\\10.*")
    )
    passwordOption = "" if args.pfx_password == "" else f'/p "{args.pfx_password}"'
    p = subprocess.run(
        f'"{windowsSdkDir}\\x64\\signtool.exe" sign /fd SHA256 /a /f "{pfxPath}" {passwordOption} TouchHelper.exe'
    )
    if p.returncode != 0:
        raise Exception("签名失败")
