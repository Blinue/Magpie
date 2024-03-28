import sys
import os
import glob
import subprocess

if len(sys.argv) != 3:
    raise Exception("请勿直接运行此脚本")

windowsSdkDir = max(
    glob.glob(os.environ["ProgramFiles(x86)"] + "\\Windows Kits\\10\\bin\\10.*")
)
makepriPath = windowsSdkDir + "\\x64\\makepri.exe"
if not os.access(makepriPath, os.X_OK):
    raise Exception("未找到 makepri")

os.chdir(sys.argv[1])

with open("priconfig.xml", "w") as priConfig:
    priConfig.write(
        '<?xml version="1.0" encoding="utf-8"?>\n<resources targetOsVersion="10.0.0" majorVersion="1">'
    )
    for priPath in sys.argv[2].split(";"):
        priConfig.write(
            f"""
  <index root="\" startIndexAt="{priPath}">
    <default>
      <qualifier name="Language" value="en-US" />
      <qualifier name="Contrast" value="standard" />
      <qualifier name="Scale" value="200" />
      <qualifier name="HomeRegion" value="001" />
      <qualifier name="TargetSize" value="256" />
      <qualifier name="LayoutDirection" value="LTR" />
      <qualifier name="DXFeatureLevel" value="DX9" />
      <qualifier name="Configuration" value="" />
      <qualifier name="AlternateForm" value="" />
      <qualifier name="Platform" value="UAP" />
    </default>
    <indexer-config type="PRI" />
    <indexer-config type="RESFILES" qualifierDelimiter="." />
  </index>"""
        )
    priConfig.write("\n</resources>")

subprocess.run(
    f'"{makepriPath}" New /pr . /cf priconfig.xml /of resources.pri /in Magpie.App /o',
    capture_output=True,
)
os.remove("priconfig.xml")
