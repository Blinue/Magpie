with open('../EffectCommon/conanfile.py') as f:
	exec(f.read())


class ModuleCommonConan(EffectCommonConan):
	pass


del EffectCommonConan
