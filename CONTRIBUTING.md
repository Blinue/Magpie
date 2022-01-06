# 贡献指南

**首先，感谢你将宝贵的时间花费在本项目上！** 

Magpie 是一个个人项目，最早启发于 Integer Scaler 和 Lossless Scaling，但现在它比前辈们强大的多。开发者的能力和精力有限，因此欢迎任何形式的贡献！Magpie 遵循 [All Contributors](https://github.com/all-contributors/all-contributors) 规范，无论你以何种形式做出贡献，诸如编写代码、撰写文档、用户测试等，只要你的贡献足够，开发者都会把你加入贡献者清单。

如果你是新手，强烈推荐你读一下[这篇文章](https://opensourceway.community/open-source-guide/how-to-contribute/)。

下面是一些你可能想做的贡献：

### 我有一个问题 🙏

[FAQ](https://github.com/Blinue/Magpie/wiki/FAQ) 汇总了常见的问题，你也可以搜索 [Issue](https://github.com/Blinue/Magpie/issues) 和 [Discussion](https://github.com/Blinue/Magpie/discussions) 看是否已经有人提出过。如果依然没有得到解答，请在 Discussion 中询问。

### 我遇到了一个错误 🐞

Magpie 没有广泛的测试过，因此错误不可避免。希望你能向开发者反馈这个错误，这样可以帮助所有和你遇到同样问题的人。

首先请在 Issue 和 Discussion 中搜索你遇到的错误，避免和现有 Issue 重复。汇报错误时请发布一个 [Issue（bug report）](https://github.com/Blinue/Magpie/issues/new?assignees=&labels=bug&template=01_bug.yaml)，下面是一些有利于开发者快速定位问题的最佳实践：

1. 选择一个清晰简洁的标题。如 “Windows 11窗口圆角导致游戏窗口超分后左下/右下角模糊” 是一个好标题，而 “应用程序错误” 则意义不明。
2. 上传日志文件对定位问题特别有帮助，它们位于 logs 文件夹下。**请以附件的形式上传日志，而不是将其中的内容复制到正文中。**
3. 请详细的描述复现步骤，最好提供一些截图。
4. 还有一些信息可能对开发者有帮助，比如特殊的显示器配置，显卡型号等。

### 我有一个功能建议 🚀

你可能有一些关于 Magpie 的奇思妙想，请和开发者分享它们！开发者通常只是通过自己的使用经验添加新功能，但你的点子可能会使 Magpie 与众不同。

首先请在 Issue 中搜索你的功能建议，确保不和已有的重复，尤其要查看 [Projects](https://github.com/Blinue/Magpie/projects) 里是否有这个功能。然后提交一个 [Issue (feature request)](https://github.com/Blinue/Magpie/issues/new?assignees=&labels=enhancement&template=03_request.yaml) 详细地描述你的建议，比如是否有其他软件实现了类似的功能。

**如果你想建议一个新的效果，请明确说明它的名称，适用场景以及已有的着色器实现（如果有的话），另外强烈建议你附上和现有效果的对比图。**

### 我想贡献代码 💻

贡献代码是帮助 Magpie 项目最直接的方式，你可以修复 bug，增加新功能或修正格式错误。不要因为修改太小就羞于贡献，你的每一行修改都很重要。

和所有开源项目一样，你需要提交 pull request 来向 Magpie 贡献代码。开发者可能会要求你做一些更改，如果你无法及时响应，你的 pull request 可能会被关闭。

如果是对已发布功能的 bug 修复，请合并到 main 分支，否则选择 dev 分支。如果你拿不准，合并到 dev 分支就好。

**注意：一旦你向 Magpie 贡献代码，便表示你同意将该代码的版权转让给 Magpie 当前的版权所有者。** 这是为了使开发者做出重大决定（如更换许可证）时无需征得每一个贡献者的同意。开发者承诺 Magpie 项目更换许可证的唯一情形是迁移到更新版本的 GPL 协议。如果你想保留版权，只能放弃贡献，将更改保留在自己的 Fork 中。

贡献代码时你需要遵守一些准则：

1. 和现有的代码风格保持一致，包括花括号不换行，tab 缩进，变量、类、源文件等的命名方式，所有源码文件格式均为 UTF-8 without BOM，大部分情况下采用行注释，git 消息风格等等。下面是一个代码风格的示例（C++）：
    ``` c++
    // ClassName.h
    
    class ClassName {
    public:
        void Test1();

    private:
        void _PrivateTest1();

        int _m1 = 0;
    };
    
    // ClassName.cpp
    
    void ClassName::Test1() {
        if (_m1 <= 0) {
            ++_m1;
        }
    }

    void ClassName::_PrivateTest1() {
        try {
            _m1 = std::stoi("123");
        } catch (std::exception& e) {
            // 错误处理
        }

        int sum = 0;
        for (int i = 0; i < _m1; ++i) {
            sum += i;
        }
    }
    ```
3. 如果你要进行比较大的更改请先查看 [Projects](https://github.com/Blinue/Magpie/projects) 或提交 pull request 和开发者交流，确保和项目当前的方向一致。
4. 你的分支必须可以通过编译检查。
5. 请尽量应用 VS 检查代码时提出的建议。

### 我想贡献翻译 🌍

贡献新的翻译和修正现有翻译都是非常欢迎的。向 Magpie 贡献翻译非常简单，所有用户界面文本均存储在 resx 中，创建新的Resources.xx-xx.resx 文件并翻译所有字符串即可。和[贡献代码](#我想贡献代码-)一样，你需要提交 pull request。强烈推荐你定期维护自己的翻译，因为 Magpie 的用户界面经常会进行较大的更改。

**推荐使用 [ResXManager](https://marketplace.visualstudio.com/items?itemName=TomEnglert.ResXManager) 插件。它提供了友好的用户界面，使你无需编写代码。**

### 我想贡献文档 📖

因为开发者的懒惰，Magpie 的文档长期处于缺失/过时的状态，因此文档的贡献是非常欢迎的。Magpie 的 wiki 是从 main 分支的 docs 文件夹自动发布的，因此修改文档和[贡献代码](#我想贡献代码-)的方式相同。

如果你想改进已发布功能的文档，请合并到 main 分支，否则请合并到 dev 分支。每次发布版本时，dev 分支的更改都将合并到 main 分支，wiki 也会相应的自动更新。

### 我想资助 Magpie 💰

开发者每周都会花费大量的时间开发新功能，这些工作都是无偿的。目前 Magpie 没有资助的渠道，对它进行 Star、Fork 或者宣传就是最好的资助！



