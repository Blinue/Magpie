#include "pch.h"
#include "EffectDrawer.h"
#include "ScalingOptions.h"

namespace Magpie::Core {

bool EffectDrawer::Initialize(
	const EffectDesc& /*desc*/,
	const EffectOption& /*option*/,
	ID3D11Texture2D* /*inputTex*/,
	RECT* /*outputRect*/,
	RECT* /*virtualOutputRect*/
) noexcept {
	return true;
}

}
