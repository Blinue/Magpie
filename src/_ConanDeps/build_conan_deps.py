import sys
import os
import hashlib
import subprocess

if len(sys.argv) != 3:
    raise Exception("请勿直接运行此脚本")

platform = sys.argv[1]
configuration = sys.argv[2]

if not platform in ["x64", "ARM64"] or not configuration in ["Debug", "Release"]:
    raise Exception("非法参数")

subprocess.run(f"conan config set storage.path={os.getcwd()}\\..\\..\\.conan\\data")

# 遍历存在 conanfile.txt 的项目
for project in os.listdir(".."):
    conanfilePath = f"..\\{project}\\conanfile.txt"
    try:
        with open(conanfilePath, "rb") as conanfile:
            hash = hashlib.file_digest(conanfile, hashlib.sha256).hexdigest()
    except:
        continue

    hashFilePath = f"..\\..\\.conan\\{platform}\\{configuration}\\{project}\\hash.txt"
    try:
        with open(hashFilePath, "r") as hashFile:
            if hashFile.read(len(hash)) == hash:
                # 哈希未变化
                continue
    except:
        pass

    # 编译依赖
    if platform == "x64":
        build_type = "x86_64"
    else:
        build_type = "armv8"

    if configuration == "Debug":
        runtime = "MTd"
    else:
        runtime = "MT"

    p = subprocess.run(
        f"conan install {conanfilePath} --install-folder ..\\..\\.conan\\{platform}\\{configuration}\\{project} --build=outdated -s build_type={configuration} -s arch={build_type} -s compiler.version=17 -s compiler.runtime={runtime} --update"
    )
    if p.returncode != 0:
        raise Exception("conan install 失败")
    
    # 更新哈希文件
    with open(hashFilePath, "w") as hashFile:
        print(hash, file=hashFile)
