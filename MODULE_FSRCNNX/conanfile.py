with open('../EffectCommon/conanfile.py') as f:
	exec(f.read())


class ModuleFSRCNNXConan(EffectCommonConan):
	pass


del EffectCommonConan
