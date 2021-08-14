with open('../EffectCommon/conanfile.py') as f:
	exec(f.read())


class ModuleAcNetConan(EffectCommonConan):
	pass


del EffectCommonConan
