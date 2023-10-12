import sys
import os
import hashlib

if len(sys.argv) != 3:
    raise Exception("请勿直接运行此脚本")

platform = sys.argv[1]
configuration = sys.argv[2]

if not platform in ["x64", "ARM64"] or not configuration in ["Debug", "Release"]:
    raise Exception("非法参数")

with open("conanfile.txt", "rb") as conanfile:
    hash = hashlib.file_digest(conanfile, hashlib.sha256).hexdigest()

projectName = os.path.basename(os.getcwd())
hashFilePath = f"..\\..\\.conan\\{platform}\\{configuration}\\{projectName}\\hash.txt"

try:
    with open(hashFilePath, "r+") as hashFile:
        if hashFile.read(len(hash)) == hash:
            # 哈希未变化
            exit(0)

        # 更新哈希文件
        hashFile.seek(os.SEEK_SET)
        hashFile.truncate()
        print(hash, file=hashFile)
except FileNotFoundError:
    # hash.txt 不存在
    os.makedirs(os.path.dirname(hashFilePath))
    with open(hashFilePath, "w") as hashFile:
        print(hash, file=hashFile)

# 编译依赖
os.system(f"conan config set storage.path={os.getcwd()}\..\..\.conan\data")

if platform == "x64":
    build_type = "x86_64"
else:
    build_type = "armv8"

if configuration == "Debug":
    runtime = "MTd"
else:
    runtime = "MT"

os.system(
    f"conan install conanfile.txt --install-folder ..\..\.conan\{platform}\{configuration}\{projectName} --build=outdated -s build_type={configuration} -s arch={build_type} -s compiler.version=17 -s compiler.runtime={runtime} --update"
)
