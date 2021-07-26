with open('../EffectCommon/conanfile.py') as f:
    exec(f.read())


class ModuleFFXConan(EffectCommonConan):
    pass


del EffectCommonConan
