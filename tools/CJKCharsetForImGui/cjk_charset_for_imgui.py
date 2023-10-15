"""
导出供 ImGui 使用的字符表，格式为 https://github.com/ocornut/imgui/blob/bcfc1ad8f63997751a7269788511157ed872da2c/imgui_draw.cpp#L2883 
"""

import os

os.chdir(os.path.dirname(__file__))

with open("characters.txt", "r", encoding="utf-8") as input:
    characters = input.read()

CJK_RANGE = (0x4E00, 0x9FAF)
codePoints = [False] * (CJK_RANGE[1] - CJK_RANGE[0] + 1)

# 将所有码位记录到表中
for char in characters:
    codePoint = ord(char)

    if codePoint < CJK_RANGE[0] or codePoint > CJK_RANGE[1]:
        continue

    codePoints[codePoint - CJK_RANGE[0]] = True

# 计算每个码位和前一个的距离
offsets = list()
prevIdx = 0
for i in range(0, len(codePoints)):
    if codePoints[i]:
        offsets.append(i - prevIdx)
        prevIdx = i


# 分行并确保每行以逗号结尾
def wrapText(str: str, width: int):
    result = ""

    cur = 0
    while cur + width < len(str):
        for i in range(width, 1, -1):
            if str[cur + i - 1] == ",":
                result += str[cur : cur + i]
                result += "\n"

                cur += i
                break
        else:
            raise Exception("分行失败")

    result += str[cur:]
    return result

print(wrapText(",".join(map(lambda idx: str(idx), offsets)), 141))
