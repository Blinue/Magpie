import sys
import os
import subprocess
import argparse
import glob
import json
import pathlib
import io
import re
from typing import Any
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
# 生成 SARIF 文件
#
#####################################################################


def make_uri(path: pathlib.Path) -> str:
    # Github 对 URI 的要求：
    # * 大小写必须匹配
    # * POSIX 格式
    # * 需要转义特殊字符

    path = path.resolve()

    # 不在项目目录中就保持绝对路径
    try:
        path = path.relative_to(os.getcwd())
    except ValueError:
        pass
    
    return urllib.parse.quote(path.as_posix(), safe=":/")


mergedRun: dict[str, Any] = {"results": []}
# 用于避免 result 重复，检查 ruleId、message、artifactLocationUri、startLine 和 startColumn
seenResults: set[tuple[str, str, str, int, int]] = set()

if args.tool == "Microsoft C++ Code Analysis":
    # Microsoft C++ Code Analysis 为每个源码文件生成一个 SARIF，需将它们合并
    sarifPaths = sorted(glob.glob("obj\\x64\\Release\\**\\*.sarif", recursive=True))
    if len(sarifPaths) == 0:
        raise Exception("未找到 SARIF 文件")

    outputPath = pathlib.Path("code-analysis\\PREfast.sarif")

    for sarifPath in sarifPaths:
        with open(sarifPath, "r", encoding="utf-8") as file:
            sarif = json.load(file)

        for run in sarif.get("runs", []):
            if "tool" not in mergedRun:
                mergedRun["tool"] = run.get("tool")

            # 当前 run 的 artifact 索引映射到 mergedRun.artifact 索引
            indexToUri: list[str] = []

            # 将 URI 内联 Github 才能正确显示代码执行路径，因此不需要 artifacts 了
            for artifact in run.get("artifacts", []):
                uri: str = artifact["location"]["uri"]
                indexToUri.append(make_uri(pathlib.Path.from_uri(uri)))

            # 将 artifact 索引改为 URI
            for result in run.get("results", []):
                if "ruleId" not in result:
                    continue

                locations = result.get("locations", [])
                if len(locations) > 0:
                    physicalLocation = locations[0]["physicalLocation"]
                    artifactLocationUri = indexToUri[
                        physicalLocation["artifactLocation"]["index"]
                    ]
                    startLine = physicalLocation["region"]["startLine"]
                    startColumn = physicalLocation["region"]["startColumn"]
                else:
                    artifactLocationUri = ""
                    startLine = -1
                    startColumn = -1

                # 根据定义，message 必须存在
                resultKey = (
                    result["ruleId"],
                    result["message"].get("text", ""),
                    artifactLocationUri,
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
                            artifactLocation = location["location"]["physicalLocation"][
                                "artifactLocation"
                            ]
                            artifactLocation["uri"] = indexToUri[
                                artifactLocation["index"]
                            ]
                            artifactLocation.pop("index")

                mergedRun["results"].append(result)
else:
    # 每个项目会生成一个日志文件记录 clang-tidy 的输出，需要解析和合并
    clangTidyLogs = sorted(
        glob.glob("obj\\x64\\Release\\**\\*.ClangTidy.log", recursive=True)
    )
    if len(clangTidyLogs) == 0:
        raise Exception("未找到 clang-tidy 日志文件")

    mergedRun["tool"] = {
        "driver": {
            "name": "clang-tidy",
            "informationUri": "https://clang.llvm.org/extra/clang-tidy/",
        }
    }

    msgRegex = re.compile(
        r"^(?P<path>.+?):(?P<line>\d+):(?P<column>\d+): *(?P<severity>note|remark|warning|error|fatal): *(?P<message>.+?)(?: *\[(?P<ruleId>[^\]]+)\])?$"
    )

    for logFile in clangTidyLogs:
        with open(logFile, "r", encoding="utf-8") as f:
            logContent = f.read()

        # 日志包含 BOM
        BOM = '\ufeff'
        if logContent.startswith(BOM):
            logContent = logContent[len(BOM):]

        for line in logContent.splitlines():
            match = re.match(msgRegex, line)
            if match is None:
                continue

            severity = match.group("severity")
            ruleId = match.group("ruleId")
            if severity == "note" or severity == "remark" or ruleId is None:
                continue

            resultKey = (
                ruleId,
                match.group("message"),
                make_uri(pathlib.Path(match.group("path"))),
                int(match.group("line")),
                int(match.group("column")),
            )
            # 避免 result 重复
            if resultKey in seenResults:
                continue
            seenResults.add(resultKey)

            mergedRun["results"].append(
                {
                    "ruleId": ruleId,
                    "level": "warning" if severity == "warning" else "error",
                    "message": {"text": resultKey[1]},
                    "locations": [
                        {
                            "physicalLocation": {
                                "artifactLocation": {
                                    "uri": resultKey[2]
                                },
                                "region": {
                                    "startLine": resultKey[3],
                                    "startColumn": resultKey[4],
                                },
                            }
                        }
                    ],
                }
            )

outputPath = pathlib.Path(
    f"code-analysis\\{"clang-tidy" if args.tool == "clang-tidy" else "PREfast"}.sarif"
)
outputPath.parent.mkdir(parents=True, exist_ok=True)

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
