import sys
import os
import subprocess
import argparse
import glob
import json
import pathlib
import io
import urllib.parse

try:
    # https://docs.github.com/en/actions/learn-github-actions/variables
    if os.environ["GITHUB_ACTIONS"].lower() == "true":
        # 不知为何在 Github Actions 中运行时默认编码为 ANSI，并且 print 需刷新流才能正常显示
        for stream in [sys.stdout, sys.stderr]:
            if isinstance(stream, io.TextIOWrapper):
                stream.reconfigure(encoding="utf-8")
except:
    pass

argParser = argparse.ArgumentParser()
argParser.add_argument(
    "--tool",
    choices=["Microsoft C++ Code Analysis", "clang-tidy"],
    default="Microsoft C++ Code Analysis",
)
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
    f'"{vswherePath}" -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe',
    capture_output=True,
)
msbuildPath = str(p.stdout, encoding="utf-8").splitlines()[0]
if not os.access(msbuildPath, os.X_OK):
    raise Exception("未找到 msbuild")

#####################################################################
#
# 编译和执行代码分析
#
#####################################################################

os.chdir(os.path.dirname(__file__) + "\\..")

p = subprocess.run(
    f'"{msbuildPath}" Magpie.slnx -m -t:Rebuild -restore -p:RestorePackagesConfig=true;Configuration=Release;Platform=x64;DisablePDB=true;RunCodeAnalysis=true;UseClangCL={args.tool == "clang-tidy"}'
)
if p.returncode != 0:
    raise Exception("编译失败")

#####################################################################
#
# 合并 SARIF 文件
#
#####################################################################


def merge_sarif_files(sarifPaths: list[str], outputPath: pathlib.Path):
    mergedRun = {"results": []}
    # 用于避免 result 重复，检查 ruleId、message、artifactLocation、startLine 和 startColumn
    seenResults: set[tuple[str, str, int, int, int]] = set()

    for sarifPath in sarifPaths:
        with open(sarifPath, "r", encoding="utf-8") as file:
            sarif = json.load(file)

        for run in sarif.get("runs", []):
            if "tool" not in mergedRun:
                mergedRun["tool"] = run.get("tool")

            if "invocations" not in mergedRun:
                mergedRun["invocations"] = run.get("invocations")

            # 当前 run 的 artifact 索引映射到 mergedRun.artifact 索引
            indexToUri: list[str] = []

            # 将 uri 内联，因此不需要 artifacts
            for artifact in run.get("artifacts", []):
                uri: str = artifact["location"]["uri"]

                # Github 对 URI 的要求：
                # * 大小写必须匹配
                # * POSIX 格式
                # * 需要转义特殊字符

                # 原始路径全是小写，需解析为真实路径
                realPath = pathlib.Path.from_uri(uri).resolve()
                uri = realPath.relative_to(os.getcwd()).as_posix()
                uri = urllib.parse.quote(uri)

                indexToUri.append(uri)

            # 更新 result 中的 artifact 索引
            for result in run.get("results", []):
                if "ruleId" not in result:
                    continue

                locations = result.get("locations", [])
                if len(locations) > 0:
                    physicalLocation = locations[0]["physicalLocation"]
                    artifactLocation = physicalLocation["artifactLocation"]["index"]
                    startLine = physicalLocation["region"]["startLine"]
                    startColumn = physicalLocation["region"]["startColumn"]
                else:
                    artifactLocation = -1
                    startLine = -1
                    startColumn = -1

                resultKey = (
                    result["ruleId"],
                    result.get("message", {}).get("text", ""),
                    artifactLocation,
                    startLine,
                    startColumn,
                )
                # 避免 result 重复
                if resultKey in seenResults:
                    continue
                seenResults.add(resultKey)

                if "analysisTarget" in result:
                    analysisTarget: dict = result["analysisTarget"]
                    analysisTarget["uri"] = indexToUri[analysisTarget["index"]]
                    analysisTarget.pop("index")

                for location in locations:
                    artifactLocation = location["physicalLocation"]["artifactLocation"]
                    artifactLocation["uri"] = indexToUri[artifactLocation["index"]]
                    artifactLocation.pop("index")

                for codeFlow in result.get("codeFlows", []):
                    for threadFlow in codeFlow.get("threadFlows", []):
                        for location in threadFlow.get("locations", []):
                            artifactLocation = location["location"]["physicalLocation"]["artifactLocation"]
                            artifactLocation["uri"] = indexToUri[artifactLocation["index"]]
                            artifactLocation.pop("index")

                mergedRun["results"].append(result)

    with open(outputPath, "w", encoding="utf-8") as file:
        json.dump(
            {
                "$schema": "https://json.schemastore.org/sarif-2.1.0.json",
                "version": "2.1.0",
                "runs": [mergedRun],
            },
            file,
            ensure_ascii=False,
            indent=2,
        )
        file.write("\n")


if args.tool == "Microsoft C++ Code Analysis":
    sarifPaths = sorted(glob.glob("obj\\x64\\Release\\**\\*.sarif", recursive=True))
    if len(sarifPaths) == 0:
        raise Exception("未找到 SARIF 文件")

    outputPath = pathlib.Path("code-analysis\\PREfast.sarif")
    outputPath.parent.mkdir(parents=True, exist_ok=True)

    merge_sarif_files(sarifPaths, outputPath)
