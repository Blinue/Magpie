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


def merge_sarif_files(sarifPaths, outputPath: pathlib.Path):
    mergedRun = {"results": [], "artifacts": []}
    # uri -> 索引
    artifactUris = {}

    for sarifPath in sarifPaths:
        with open(sarifPath, "r", encoding="utf-8") as file:
            sarif = json.load(file)

        for run in sarif.get("runs", []):
            if "tool" not in mergedRun:
                mergedRun["tool"] = run.get("tool")

            if "invocations" not in mergedRun:
                mergedRun["invocations"] = run.get("invocations")

            # 当前 run 的 artifact 索引映射到 mergedRun.artifact 索引
            localIndexMap = []

            for artifact in run.get("artifacts", []):
                uri = artifact["location"]["uri"]

                if uri in artifactUris:
                    localIndexMap.append(artifactUris[uri])
                else:
                    newIndex = len(artifactUris)
                    artifactUris[uri] = newIndex
                    localIndexMap.append(newIndex)
                
                    mergedRun["artifacts"].append(
                        {
                            **artifact,
                            # uri 需要转义，否则 Github 无法识别
                            "location": {"uri": urllib.parse.quote(uri, safe="/\\:")},
                        }
                    )

            # 更新 result 中的 artifact 索引
            for result in run.get("results", []):
                if "analysisTarget" in result:
                    result["analysisTarget"]["index"] = localIndexMap[result["analysisTarget"]["index"]]
                
                for location in result.get("locations", []):
                    location["physicalLocation"]["artifactLocation"]["index"] = localIndexMap[location["physicalLocation"]["artifactLocation"]["index"]]

                for codeFlow in result.get("codeFlows", []):
                    for threadFlow in codeFlow.get("threadFlows", []):
                        for location in threadFlow.get("locations", []):
                            location["location"]["physicalLocation"]["artifactLocation"]["index"] = localIndexMap[location["location"]["physicalLocation"]["artifactLocation"]["index"]]

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
