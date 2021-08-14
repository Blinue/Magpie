with open('../EffectCommon/conanfile.py') as f:
	exec(f.read())


class ModuleAnime4KConan(EffectCommonConan):
	pass


del EffectCommonConan
