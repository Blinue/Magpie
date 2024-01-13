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

os.chdir(os.path.dirname(__file__))

# 记录是否有项目的依赖需要编译
anyProjectToBuild = False

# 遍历存在 conanfile.txt 的项目
for project in os.listdir(".."):
    conanfilePath = f"..\\{project}\\conanfile.txt"
    try:
        with open(conanfilePath, "rb") as conanfile:
            hash = hashlib.sha256(conanfile.read()).hexdigest()
    except FileNotFoundError:
        continue

    hashFilePath = f"..\\..\\.conan\\{project}\\{platform}_{configuration}_hash.txt"
    try:
        with open(hashFilePath, "r") as hashFile:
            if hashFile.read() == hash:
                # 哈希未变化
                continue
    except:
        pass

    anyProjectToBuild = True

    # 编译依赖
    if platform == "x64":
        build_type = "x86_64"
    else:
        build_type = "armv8"

    # HybridCRT 要求静态链接 CRT
    p = subprocess.run(
        f"conan install {conanfilePath} -pr:a=conanprofile.txt --output-folder ..\\..\\.conan\\{project} --build=missing -s build_type={configuration} -s arch={build_type} --update"
    )
    if p.returncode != 0:
        raise Exception("conan install 失败")
    
    # 更新哈希文件
    with open(hashFilePath, "w") as hashFile:
        hashFile.write(hash)

if not anyProjectToBuild:
    print("Conan 依赖已是最新", flush=True)
