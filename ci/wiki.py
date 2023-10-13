import sys
import os
import tempfile
import glob
import shutil
import subprocess

try:
    # https://docs.github.com/en/actions/learn-github-actions/variables
    if os.environ["GITHUB_ACTIONS"].lower() == "true":
        # 不知为何在 Github Actions 中运行时默认编码为 ANSI，并且 print 需刷新流才能正常显示
        for stream in [sys.stdout, sys.stderr]:
            stream.reconfigure(encoding="utf-8")
except:
    pass

if not "GH_PERSONAL_ACCESS_TOKEN" in os.environ:
    raise Exception("未找到环境变量 GH_PERSONAL_ACCESS_TOKEN")

wikiRepoUrl = os.path.expandvars(
    "https://${GH_PERSONAL_ACCESS_TOKEN}@github.com/${GITHUB_REPOSITORY}.wiki.git"
)

# 创建临时目录
wikiRepoDir = tempfile.mkdtemp()
os.chdir(wikiRepoDir)

p = subprocess.run("git init")
if p.returncode != 0:
    raise Exception("git init 失败")

actor = os.environ["GITHUB_ACTOR"]
subprocess.run("git config user.name " + actor)
subprocess.run(f"git config user.email {actor}@users.noreply.github.com")

# 拉取
p = subprocess.run(f'git pull "{wikiRepoUrl}"')
if p.returncode != 0:
    raise Exception("git pull 失败")

# 将文档拷贝到临时目录
docsDir = os.path.normpath(os.path.dirname(__file__) + "\\..\\docs")
for file in glob.glob(docsDir + "\\*.md"):
    shutil.copy(file, wikiRepoDir)
    print("已拷贝 " + file, flush=True)

# 推送
subprocess.run("git add .")
subprocess.run('git commit -m "Published by CI"')
p = subprocess.run(f'git push --set-upstream "{wikiRepoUrl}" master')
if p.returncode != 0:
    raise Exception("git push 失败")
