with open('../EffectCommon/conanfile.py') as f:
	exec(f.read())


class RuntimeConan(EffectCommonConan):
	pass


del EffectCommonConan
