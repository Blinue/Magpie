#include "pch.h"
#include "MicaBrush.h"
#if __has_include("MicaBrush.g.cpp")
#include "MicaBrush.g.cpp"
#endif

#include "CommonSharedConstants.h"
#include "XamlUtils.h"


using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::ViewManagement;
using namespace Windows::Foundation::Metadata;
using namespace Windows::System::Power;
using namespace Microsoft::Graphics::Canvas::Effects;

// 移植自 https://github.com/MicaForEveryone/MicaForEveryone/blob/master/MicaForEveryone.UI/Brushes/XamlMicaBrush.cs

static CompositionBrush BuildMicaEffectBrush(Compositor compositor, Color tintColor, float tintOpacity, float luminosityOpacity) {
	// Tint Color.
	ColorSourceEffect tintColorEffect;
	tintColorEffect.Name(L"TintColor");
	tintColorEffect.Color(tintColor);
	
	// OpacityEffect applied to Tint.
	OpacityEffect tintOpacityEffect;
	tintOpacityEffect.Name(L"TintOpacity");
	tintOpacityEffect.Opacity(tintOpacity);
	tintOpacityEffect.Source(tintColorEffect);

	// Apply Luminosity:

	// Luminosity Color.
	ColorSourceEffect luminosityColorEffect;
	luminosityColorEffect.Color(tintColor);

	// OpacityEffect applied to Luminosity.
	OpacityEffect luminosityOpacityEffect;
	luminosityOpacityEffect.Name(L"LuminosityOpacity");
	luminosityOpacityEffect.Opacity(luminosityOpacity);
	luminosityOpacityEffect.Source(luminosityColorEffect);

	// Luminosity Blend.
	// NOTE: There is currently a bug where the names of BlendEffectMode::Luminosity and BlendEffectMode::Color are flipped.
	BlendEffect luminosityBlendEffect;
	luminosityBlendEffect.Mode(BlendEffectMode::Color);
	luminosityBlendEffect.Background(CompositionEffectSourceParameter(L"BlurredWallpaperBackdrop"));
	luminosityBlendEffect.Foreground(luminosityOpacityEffect);

	// Apply Tint:

	// Color Blend.
	// NOTE: There is currently a bug where the names of BlendEffectMode::Luminosity and BlendEffectMode::Color are flipped.
	BlendEffect colorBlendEffect;
	colorBlendEffect.Mode(BlendEffectMode::Luminosity);
	colorBlendEffect.Background(luminosityBlendEffect);
	colorBlendEffect.Foreground(tintOpacityEffect);

	CompositionEffectBrush micaEffectBrush = compositor.CreateEffectFactory(colorBlendEffect).CreateBrush();
	micaEffectBrush.SetSourceParameter(
		L"BlurredWallpaperBackdrop",
		compositor.TryCreateBlurredWallpaperBackdropBrush()
	);

	return micaEffectBrush;
}

static CompositionBrush CreateCrossFadeEffectBrush(Compositor compositor, CompositionBrush from, CompositionBrush to) {
	CrossFadeEffect crossFadeEffect;
	// Name to reference when starting the animation.
	crossFadeEffect.Name(L"Crossfade");
	crossFadeEffect.Source1(CompositionEffectSourceParameter(L"source1"));
	crossFadeEffect.Source2(CompositionEffectSourceParameter(L"source2"));
	crossFadeEffect.CrossFade(0);

	CompositionEffectBrush crossFadeEffectBrush = compositor.CreateEffectFactory(
		crossFadeEffect,
		{ L"Crossfade.CrossFade" }
	).CreateBrush();
	crossFadeEffectBrush.Comment(L"Crossfade");
	// The inputs have to be swapped here to work correctly...
	crossFadeEffectBrush.SetSourceParameter(L"source1", to);
	crossFadeEffectBrush.SetSourceParameter(L"source2", from);
	return crossFadeEffectBrush;
}

static ScalarKeyFrameAnimation CreateCrossFadeAnimation(Compositor compositor) {
	ScalarKeyFrameAnimation animation = compositor.CreateScalarKeyFrameAnimation();
	auto easing = compositor.CreateCubicBezierEasingFunction({ 0.05f, 0.15f }, { 0.9f, 1.0f });
	animation.InsertKeyFrame(0, 0, easing);
	animation.InsertKeyFrame(1, 1, easing);
	/*LinearEasingFunction linearEasing = compositor.CreateLinearEasingFunction();
	animation.InsertKeyFrame(0.0f, 0.0f, linearEasing);
	animation.InsertKeyFrame(1.0f, 1.0f, linearEasing);*/
	animation.Duration(TimeSpan(std::chrono::milliseconds(250)));

	return animation;
}

namespace winrt::Magpie::App::implementation {

MicaBrush::MicaBrush(FrameworkElement root) {
	_rootElement = root;

	_app = Application::Current().as<App>();

	_hasMica = ApiInformation::IsMethodPresent(
		name_of<Compositor>(),
		L"TryCreateBlurredWallpaperBackdropBrush"
	);
}

void MicaBrush::OnConnected() {
	if (_settings == nullptr)
		_settings = UISettings();

	if (_accessibilitySettings == nullptr)
		_accessibilitySettings = AccessibilitySettings();

	if (!_fastEffects.has_value())
		_fastEffects = CompositionCapabilities::GetForCurrentView().AreEffectsFast();

	if (!_energySaver.has_value())
		_energySaver = PowerManager::EnergySaverStatus() == EnergySaverStatus::On;

	_windowActivated = _app.IsHostWndFocused();

	_UpdateBrush();

	_highContrastChangedToken = _accessibilitySettings.HighContrastChanged({ this, &MicaBrush::_AccessibilitySettings_HighContrastChanged });
	_energySaverStatusChangedToken = PowerManager::EnergySaverStatusChanged({ this, &MicaBrush::_PowerManager_EnergySaverStatusChanged });
	_compositionCapabilitiesChangedToken = CompositionCapabilities::GetForCurrentView().Changed({ this, &MicaBrush::_CompositionCapabilities_Changed });
	_rootElementThemeChangedToken = _rootElement.ActualThemeChanged({ this, &MicaBrush::_RootElement_ActualThemeChanged });
	_hostWndFocusedChangedToken = _app.HostWndFocusChanged({this, &MicaBrush::_App_HostWndFocusedChanged});
}

void MicaBrush::OnDisconnected() {
	if (_settings != nullptr) {
		_settings = nullptr;
	}

	if (_accessibilitySettings != nullptr) {
		_accessibilitySettings.HighContrastChanged(_highContrastChangedToken);
		_highContrastChangedToken = {};
		_accessibilitySettings = nullptr;
	}

	PowerManager::EnergySaverStatusChanged(_energySaverStatusChangedToken);
	_energySaverStatusChangedToken = {};

	CompositionCapabilities::GetForCurrentView().Changed(_compositionCapabilitiesChangedToken);
	_compositionCapabilitiesChangedToken = {};

	_rootElement.ActualThemeChanged(_rootElementThemeChangedToken);
	_rootElementThemeChangedToken = {};

	_app.HostWndFocusChanged(_hostWndFocusedChangedToken);
	_hostWndFocusedChangedToken = {};

	if (CompositionBrush() != nullptr) {
		CompositionBrush().Close();
		CompositionBrush(nullptr);
	}
}

IAsyncAction MicaBrush::_AccessibilitySettings_HighContrastChanged(AccessibilitySettings const&, IInspectable const&) {
	return Dispatcher().RunAsync(CoreDispatcherPriority::Normal, { this, &MicaBrush::_UpdateBrush });
}

IAsyncAction MicaBrush::_CompositionCapabilities_Changed(CompositionCapabilities sender, IInspectable const&) {
	return Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [&]() {
		_fastEffects = sender.AreEffectsFast();
		_UpdateBrush();
	});
}

IAsyncAction MicaBrush::_PowerManager_EnergySaverStatusChanged(IInspectable const&, IInspectable const&) {
	return Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [&]() {
		_energySaver = PowerManager::EnergySaverStatus() == EnergySaverStatus::On;
		_UpdateBrush();
   });
}

void MicaBrush::_RootElement_ActualThemeChanged(Windows::UI::Xaml::FrameworkElement const&, Windows::Foundation::IInspectable const&) {
	_UpdateBrush();
}

void MicaBrush::_App_HostWndFocusedChanged(IInspectable const&, bool isFocused) {
	_windowActivated = isFocused;
	_UpdateBrush();
}

void MicaBrush::_UpdateBrush() {
	if (_settings == nullptr || _accessibilitySettings == nullptr) {
		return;
	}

	ElementTheme currentTheme = _rootElement.ActualTheme();
	Compositor compositor = Window::Current().Compositor();

	bool useSolidColorFallback = !_hasMica || !_settings.AdvancedEffectsEnabled() ||
		!_windowActivated || _fastEffects == false || _energySaver == true;

	Color tintColor = XamlUtils::Win32ColorToWinRTColor(currentTheme == ElementTheme::Light ?
		CommonSharedConstants::LIGHT_TINT_COLOR : CommonSharedConstants::DARK_TINT_COLOR);
	float tintOpacity = currentTheme == ElementTheme::Light ? 0.5f : 0.8f;

	if (_accessibilitySettings.HighContrast()) {
		tintColor = _settings.GetColorValue(UIColorType::Background);
		useSolidColorFallback = true;
	}

	auto newBrush = useSolidColorFallback ?
		compositor.CreateColorBrush(tintColor) :
		BuildMicaEffectBrush(compositor, tintColor, tintOpacity, 1.0f);

	auto oldBrush = CompositionBrush();

	bool doCrossFade = oldBrush != nullptr &&
		CompositionBrush().Comment() != L"CrossFade" &&
		!(get_class_name(oldBrush) == name_of<CompositionColorBrush>() &&
			get_class_name(newBrush) == name_of<CompositionColorBrush>());

	if (doCrossFade) {
		auto crossFadeBrush = CreateCrossFadeEffectBrush(compositor, oldBrush, newBrush);
		ScalarKeyFrameAnimation animation = CreateCrossFadeAnimation(compositor);
		CompositionBrush(crossFadeBrush);

		CompositionScopedBatch crossFadeAnimationBatch = compositor.CreateScopedBatch(CompositionBatchTypes::Animation);
		crossFadeBrush.StartAnimation(L"CrossFade.CrossFade", animation);
		crossFadeAnimationBatch.End();

		crossFadeAnimationBatch.Completed([=, this](IInspectable const&, CompositionBatchCompletedEventArgs const&) {
			crossFadeBrush.Close();
			oldBrush.Close();
			CompositionBrush(newBrush);
		});
	} else {
		CompositionBrush(newBrush);
	}
}

}
