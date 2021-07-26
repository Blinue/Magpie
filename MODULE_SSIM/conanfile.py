with open('../EffectCommon/conanfile.py') as f:
    exec(f.read())


class ModuleSSIMConan(EffectCommonConan):
    pass


del EffectCommonConan
