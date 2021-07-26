with open('../EffectCommon/conanfile.py') as f:
    exec(f.read())


class ModuleRAVUConan(EffectCommonConan):
    pass


del EffectCommonConan
