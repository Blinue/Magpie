import sys
import os
import re

try:
    # https://docs.github.com/en/actions/learn-github-actions/variables
    if os.environ["GITHUB_ACTIONS"].lower() == "true":
        # 不知为何在 Github Actions 中运行时默认编码为 ANSI，并且 print 需刷新流才能正常显示
        for stream in [sys.stdout, sys.stderr]:
            stream.reconfigure(encoding="utf-8")
except:
    pass

#if not "GH_PERSONAL_ACCESS_TOKEN" in os.environ:
#    raise Exception("未找到环境变量 GH_PERSONAL_ACCESS_TOKEN")

os.chdir(os.path.dirname(__file__))

#####################################################################
#
# 编译和打包
#
#####################################################################

with open("..\\publish.py", encoding="utf-8") as f:
    exec(f.read(), {"__file__": "..\\publish.py"})
