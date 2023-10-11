import sys
import os
import tempfile
import glob
import shutil

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

with tempfile.TemporaryDirectory() as wikiRepoDir:
    os.chdir(wikiRepoDir)

    os.system("git init")
    actor = os.environ["GITHUB_ACTOR"]
    os.system("git config user.name " + actor)
    os.system(f"git config user.email {actor}@users.noreply.github.com")

    # 拉取
    if os.system("git pull " + wikiRepoUrl) != 0:
        raise Exception("git pull 失败")

    # 文档拷贝到临时目录
    docsDir = os.path.normpath(os.path.dirname(__file__) + "\\..\\docs")
    for file in glob.glob(docsDir + "\\*.md"):
        shutil.copy(file, wikiRepoDir)

    # 推送
    os.system("git add .")
    os.system('git commit -m "Published by CI"')
    cmd = os.path.expandvars('git push --set-upstream "${GIT_REPOSITORY_URL}" master')
    if os.system(cmd) != 0:
        raise Exception("git push 失败")
