# Contribution Guide

**First of all, thank you for spending your valuable time on this project!**

Magpie is a personal project, initially inspired by Integer Scaler and Lossless Scaling. However, the project has become much more powerful than its predecessors. Due to the limited capability and energy of the developers, we welcome any form of contribution to this project! Magpie follows the [All Contributors](https://github.com/all-contributors/all-contributors) protocol. No matter in what form you contributed to the project (e.g. code, document, test, etc.), the developer will add you to the contributors list as long as you contributed enough.

If you are new here, we strongly recommend you to read [this article](https://opensourceway.community/open-source-guide/how-to-contribute/) (it's in Chinese).

Below are several forms of contribution you may want to make:

### I have a question 🙏

[FAQ](https://github.com/Blinue/Magpie/wiki/FAQ_EN) includes most of the common questions asked. You can also search in [Issue](https://github.com/Blinue/Magpie/issues) and [Discussion](https://github.com/Blinue/Magpie/discussions) to see if anyone has raised the same question before. Feel free to go ahead and ask in Discussion if not.

### I encountered an error 🐞

Magpie hasn't been thoroughly tested, so errors are inevitable. We hope you can let us know about the errors, which helps everyone having the same problems.

Please start from searching the errors in Issue and Discussion to avoid duplicate questions. Please publish a [Issue (bug report)](https://github.com/Blinue/Magpie/issues/new?assignees=&labels=bug&template=01_bug.yaml). Here are some best practices for the developers to quickly locate the error:

1. Choose a clear and simple subject. For example, "The rounded corner of windows in Windows 11 causes the top-left and bottom-right corners to be blurry" is an excellent subject. "Application Error," on the other hand, is hard to interpret.
2. It is especially helpful to upload the log files, which are stored in the logs folder. **Please upload the logs as attachments rather than copying their content.**
3. Please specify in details how to replicate the errors. It would be the best if you also provide us with some screenshots.
4. Additional information may be helpful for the developers. e.g. special display configurations, graphics card models, etc.

### I have a feature request 🚀

Please share any "whimsy" you have with the developers! We merely added the features based on our experience. Your ideas, however, may make a difference!

Please search in [Issue](https://github.com/Blinue/Magpie/issues) first for your feature requests to make sure they're not duplicate. Please also check [Projects](https://github.com/Blinue/Magpie/projects) (in Chinese) to see if the features have already been implemented. Please submit a [Issue (feature request)](https://github.com/Blinue/Magpie/issues/new?assignees=&labels=enhancement&template=03_request.yaml) with detailed descriptions of your suggestion, for example, if other applications have implemented similar features.

**If you'd like a new special effect to be added, please specify its name, use cases, and existing shader implementation (if exists). We also strongly recommend you to attach a comparison image with the current special effects.**

### I'd like to contribute codes 💻

Contributing codes is the more direct way to help Magpie. You may fix bugs, add new features, or correct formatting issues. You are not limited by the magnitude of the contribution. Each single line is important!

Like all other open source projects, you need to initiate pull requests to contribute codes for Magpie. The developers may ask you to make some modifications. Your pull request may be closed if you failed to respond in a timely manner.

Please merge to the `main` branch if the codes are for bug fixes of existing features. Otherwise please merge to the `dev` branch. Merging to the `dev` branch is always the safe option if you are not sure.

**Note: once you contribute codes to Magpie, you are agreeing to transfer the copyright of those codes to the current copyright owner of Magpie.** The purpose is to allow developers to make important decisions (like changing the license) without getting consent from every single contributor. The developers promise that the only changes in license will be shifting to newer versions of GPL. If you'd like to keep your copyrights, you will have to quit contributing and to apply the changes only to your forks.

You need to follow the following rules when contributing codes:

1. Keep in the same style as that of the current codes, including no change-of-line for curly brackets, using tabs for indentation, the ways to name variables, classes, and source files, using UTF-8 without BOM encoding, using inline comments in most of the cases, and using git-style messages, etc. Here is an example for C++:
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
            // Error Handling
        }

        int sum = 0;
        for (int i = 0; i < _m1; ++i) {
            sum += i;
        }
    }
    ```
2. Check [Projects](https://github.com/Blinue/Magpie/projects) or submit pull requests to communicate with the developers before you make major changes to make sure they align with the road map of the project.
3. Your branch must compile.
4. Please take the suggestions raised by VS as much as possible.

### I'd like to contribute translations 🌍

Adding new translations and fixing current translations are both welcomed. It's very easy to contribute translations to Magpie. All UI texts are stored in resx. Creating a new `Resources.xx-xx.resx` file and translating all strings is all you need to do. Just like for contributing codes, you need to submit pull requests. We strongly recommend you to periodically check your translations, since there are frequent changes in the Magpie UI.

**We recommend using the [ResXManager](https://marketplace.visualstudio.com/items?itemName=TomEnglert.ResXManager) plugin, which projects user-friendly interface and allows you to not writing even a single line of code.**

### I'd like to contribute to the documentations 📖

Because of the laziness of the developers, the documentations of Magpie are constantly missing or out of date. Contributions to documentations are more than welcomed! The MagPie wiki is automatically published from the docs folder in the `main` branch, so editing the documentation follows the same procedures of contributing codes.

Please merge to the `main` branch if you are improving the documentations of existing features. Otherwise merge to the `dev` branch. All changes in the `dev` branch will be merged to the `main` branch when new versions are released, and the wiki will be updated as well.

### I'd like to fund Magpie 💰

The developers spend a lot of time every week to implement new features without getting paid. Currently there is no way to fund Magpie. Staring, forking, and advertising are the best ways to support us!
