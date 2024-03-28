import sys
import os
import subprocess
import shutil
import requests
import hashlib
import json

try:
    # https://docs.github.com/en/actions/learn-github-actions/variables
    if os.environ["GITHUB_ACTIONS"].lower() == "true":
        # 不知为何在 Github Actions 中运行时默认编码为 ANSI，并且 print 需刷新流才能正常显示
        for stream in [sys.stdout, sys.stderr]:
            stream.reconfigure(encoding="utf-8")
except:
    pass

majorVersion = os.environ["MAJOR"]
minorVersion = os.environ["MINOR"]
patchVersion = os.environ["PATCH"]
tag = os.environ["TAG"]
isPrerelease = os.environ["PRERELEASE"].lower() == "true"
githubAccessToken = os.environ["ACCESS_TOKEN"]
repo = os.environ["GITHUB_REPOSITORY"]
actor = os.environ["GITHUB_ACTOR"]

subprocess.run("git config user.name " + actor)
subprocess.run(f"git config user.email {actor}@users.noreply.github.com")

subprocess.run(
    f"git remote set-url origin https://{githubAccessToken}@github.com/{repo}.git"
)

# 打标签
if subprocess.run(f"git tag -a {tag} -m {tag}").returncode != 0:
    raise Exception("打标签失败")

if subprocess.run("git push origin " + tag).returncode != 0:
    raise Exception("推送标签失败")

print("已创建标签 " + tag, flush=True)

headers = {
    "Accept": "application/vnd.github+json",
    "Authorization": "Bearer " + githubAccessToken,
    "X-GitHub-Api-Version": "2022-11-28",
}

# 获取前一个发布版本来生成默认发行说明
prevReleaseTag = None
try:
    if isPrerelease:
        # 发布预发行版与最新的版本（无论是正式版还是预发行版）对比
        response = requests.get(
            f"https://api.github.com/repos/{repo}/releases",
            json={"per_page": 1},
            headers=headers,
        )
        if response.ok:
            prevReleaseTag = response.json()[0]["tag_name"]
    else:
        # 发布正式版则与最新的正式版对比
        # 由于可以自己选择最新版本，此接口可能不会返回时间上最新发布的版本，不是大问题
        response = requests.get(
            f"https://api.github.com/repos/{repo}/releases/latest", headers=headers
        )
        if response.ok:
            prevReleaseTag = response.json()["tag_name"]
except:
    # 忽略错误
    pass

# 发布 release
if prevReleaseTag == None:
    body = ""
else:
    # 默认发行说明为比较两个 tag
    body = f"https://github.com/{repo}/compare/{prevReleaseTag}...{tag}"

response = requests.post(
    f"https://api.github.com/repos/{repo}/releases",
    json={
        "tag_name": tag,
        "name": tag,
        "prerelease": isPrerelease,
        "body": body,
        "discussion_category_name": "Announcements",
    },
    headers=headers,
)
if not response.ok:
    raise Exception("发布失败")

uploadUrl = response.json()["upload_url"]
uploadUrl = uploadUrl[: uploadUrl.find("{")] + "?name="

os.chdir(os.path.dirname(__file__) + "\\..\\publish")

pkgInfos = {}
for platform in ["x64", "ARM64"]:
    # 打包成 zip
    pkgName = "Magpie-" + tag + "-" + platform
    shutil.make_archive(pkgName, "zip", pkgName)
    pkgName += ".zip"

    # 上传资产
    with open(pkgName, "rb") as f:
        # 流式上传
        # https://requests.readthedocs.io/en/latest/user/advanced/#streaming-uploads
        response = requests.post(
            uploadUrl + pkgName,
            data=f,
            headers={**headers, "Content-Type": "application/zip"},
        )

        if not response.ok:
            raise Exception("上传失败")

        # 计算哈希
        f.seek(0, os.SEEK_SET)
        md5 = hashlib.file_digest(f, hashlib.md5).hexdigest()

    pkgInfos[platform] = (pkgName, md5)

print("已发布 " + tag, flush=True)

# 更新 version.json
# 此步应在发布版本之后，因为程序使用 version.json 检查更新
os.chdir("..")
with open("version.json", "w", encoding="utf-8") as f:
    json.dump(
        {
            "version": f"{majorVersion}.{minorVersion}.{patchVersion}",
            "tag": tag,
            "binary": {
                "x64": {
                    "url": f"https://github.com/{repo}/releases/download/{tag}/{pkgInfos['x64'][0]}",
                    "hash": pkgInfos["x64"][1],
                },
                "ARM64": {
                    "url": f"https://github.com/{repo}/releases/download/{tag}/{pkgInfos['ARM64'][0]}",
                    "hash": pkgInfos["ARM64"][1],
                },
            },
        },
        f,
        indent=4,
    )

# 提交对 version.json 的更改
if subprocess.run("git add version.json").returncode != 0:
    raise Exception("git add 失败")

if subprocess.run('git commit -m "Update version.json"').returncode != 0:
    raise Exception("git commit 失败")

if subprocess.run("git push").returncode != 0:
    raise Exception("git push 失败")
