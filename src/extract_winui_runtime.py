import sys
import os
import glob
import zipfile
import shutil

if len(sys.argv) != 2:
    raise Exception("请勿直接运行此脚本")

platform = sys.argv[1]
if not platform in ["x64", "ARM64"]:
    raise Exception("非法参数")

os.chdir(os.path.dirname(__file__) + "\\..\\packages")
packagesFolder = os.getcwd()

winuiPkg = max(glob.glob("Microsoft.UI.Xaml*"))

intDir = f"..\\obj\\{platform}\\WinUI"

if "prerelease" in winuiPkg:
    # 预览版本的 WinUI 无需解压
    shutil.rmtree(intDir, ignore_errors=True)
else:
    os.makedirs(intDir, exist_ok=True)
    os.chdir(intDir)

    def needExtract():
        try:
            with open("version.txt") as f:
                if f.read() != winuiPkg:
                    return True

            for path in [
                "Microsoft.UI.Xaml.dll",
                "Microsoft.UI.Xaml.pri",
                "Microsoft.UI.Xaml",
            ]:
                if not os.access(path, os.F_OK):
                    return True
        except:
            return True

        return False

    if needExtract():
        with zipfile.ZipFile(
            # 取最新的包
            max(
                glob.glob(
                    f"{packagesFolder}\\{winuiPkg}\\tools\\AppX\\{platform}\\Release\\Microsoft.UI.Xaml*.appx"
                )
            )
        ) as appx:
            # 收集要解压的文件
            members = ["Microsoft.UI.Xaml.dll", "resources.pri"]
            # 编译需要 Assets 文件夹，编译完成后会删除它
            for file in appx.namelist():
                if file.startswith("Microsoft.UI.Xaml/Assets"):
                    members.append(file)
            appx.extractall(members=members)

        # 将 resources.pri 重命名为 Microsoft.UI.Xaml.pri
        try:
            os.remove("Microsoft.UI.Xaml.pri")
        except:
            pass
        os.rename("resources.pri", "Microsoft.UI.Xaml.pri")

        with open("version.txt", mode="w") as f:
            f.write(winuiPkg)
