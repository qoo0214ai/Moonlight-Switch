//
//  ingame_overlay.cpp
//  Moonlight
//
//  Created by Даниил Виноградов on 29.05.2021.
//

#ifdef PLATFORM_SWITCH
#include <borealis/platforms/switch/switch_input.hpp>
#endif

#include "helper.hpp"
#include "ingame_overlay_view.hpp"
#include "streaming_input_overlay.hpp"
#include "button_selecting_dialog.hpp"
#include "UpscalingSupport.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>

#define SET_SETTING(n, func)                                                   \
    case n:                                                                    \
        Settings::instance().func;                                             \
        break;

#define GET_SETTINGS(combo_box, n, i)                                          \
    case n:                                                                    \
        combo_box->setSelection(i);                                            \
        break;

#define DEFAULT                                                                \
    default:                                                                   \
        break;

using namespace brls;

namespace {
void updateStrengthControl(BooleanSliderCell* cell,
                           bool enabled, const std::string& detailText) {
    cell->setSliderVisibility(enabled ? brls::Visibility::VISIBLE
                                      : brls::Visibility::GONE);
    cell->setValueText(enabled ? detailText : "");
}

float sliderProgressToDitheringStrength(float progress) {
    return static_cast<float>(int(progress * 9.0f + 0.5f) + 1);
}

float ditheringStrengthToSliderProgress(float strength) {
    if (strength < 1.0f)
        strength = 1.0f;
    else if (strength > 10.0f)
        strength = 10.0f;

    return (strength - 1.0f) / 9.0f;
}

std::string getDitheringStrengthText(float strength) {
    return std::to_string(int(strength));
}

std::string getRcasStrengthText(float strength) {
    return std::to_string(int(strength * 100.0f)) + "%";
}
}

bool debug = false;

// MARK: - Ingame Overlay View
IngameOverlay::IngameOverlay(StreamingView* streamView)
    : streamView(streamView) {
    brls::Application::registerXMLView(
        "LogoutTab", [streamView]() { return new LogoutTab(streamView); });
    brls::Application::registerXMLView(
        "OptionsTab", [streamView]() { return new OptionsTab(streamView); });

    this->inflateFromXMLRes("xml/views/ingame_overlay/overlay.xml");

    addGestureRecognizer(
        new TapGestureRecognizer([this](TapGestureStatus status, Sound* sound) {
            if (status.state == GestureState::END)
                this->dismiss();
        }));

    applet->addGestureRecognizer(new TapGestureRecognizer(
        [](TapGestureStatus status, Sound* sound) {}));

    getAppletFrameItem()->title =
        streamView->getHost().hostname + ": " + streamView->getApp().name;
    updateAppletFrameItem();
}

brls::AppletFrame* IngameOverlay::getAppletFrame() { return applet; }

// MARK: - Logout Tab
LogoutTab::LogoutTab(StreamingView* streamView) : streamView(streamView) {
    this->inflateFromXMLRes("xml/views/ingame_overlay/logout_tab.xml");

    disconnect->setText("streaming/disconnect"_i18n);
    disconnect->registerClickAction([this, streamView](View* view) {
        this->dismiss([streamView] { streamView->terminate(false); });
        return true;
    });

    terminateButton->setText("streaming/terminate"_i18n);
    terminateButton->registerClickAction([this, streamView](View* view) {
        this->dismiss([streamView] { streamView->terminate(true); });
        return true;
    });
}

// MARK: - Options Tab
OptionsTab::OptionsTab(StreamingView* streamView) : streamView(streamView) {
    this->inflateFromXMLRes("xml/views/ingame_overlay/options_tab.xml");

    // Switch3 direct launch intentionally skips Moonlight's normal main menu.
    // Keep the core stream-quality controls reachable from the in-game overlay
    // so resolution/FPS/codec/bitrate can still be changed without reinstalling.
    std::vector<std::string> resolutions = {
        "settings/resolution_native"_i18n, "360p", "480p", "540p", "720p",
        "1080p",
#if !defined(__PSV__)
        "1440p",
#endif
    };
    streamResolution->setText("settings/resolution"_i18n);
    streamResolution->setData(resolutions);
    switch (Settings::instance().resolution()) {
        GET_SETTINGS(streamResolution, -1, 0)
        GET_SETTINGS(streamResolution, 360, 1)
        GET_SETTINGS(streamResolution, 480, 2)
        GET_SETTINGS(streamResolution, 540, 3)
        GET_SETTINGS(streamResolution, 720, 4)
        GET_SETTINGS(streamResolution, 1080, 5)
#if !defined(__PSV__)
        GET_SETTINGS(streamResolution, 1440, 6)
#endif
        DEFAULT
    }
    streamResolution->getEvent()->subscribe([](int selected) {
        switch (selected) {
            SET_SETTING(0, set_resolution(-1))
            SET_SETTING(1, set_resolution(360))
            SET_SETTING(2, set_resolution(480))
            SET_SETTING(3, set_resolution(540))
            SET_SETTING(4, set_resolution(720))
            SET_SETTING(5, set_resolution(1080))
#if !defined(__PSV__)
            SET_SETTING(6, set_resolution(1440))
#endif
            DEFAULT
        }
    });

#if defined(__PSV__)
    std::vector<std::string> fpss = {"24", "30", "40", "50", "60"};
#else
    std::vector<std::string> fpss = {"30", "40", "60", "120"};
#endif
    streamFps->setText("settings/fps"_i18n);
    streamFps->setData(fpss);
    switch (Settings::instance().fps()) {
#if defined(__PSV__)
        GET_SETTINGS(streamFps, 24, 0)
        GET_SETTINGS(streamFps, 30, 1)
        GET_SETTINGS(streamFps, 40, 2)
        GET_SETTINGS(streamFps, 50, 3)
        GET_SETTINGS(streamFps, 60, 4)
#else
        GET_SETTINGS(streamFps, 30, 0)
        GET_SETTINGS(streamFps, 40, 1)
        GET_SETTINGS(streamFps, 60, 2)
        GET_SETTINGS(streamFps, 120, 3)
#endif
        DEFAULT
    }
    streamFps->getEvent()->subscribe([](int selected) {
        switch (selected) {
#if defined(__PSV__)
            SET_SETTING(0, set_fps(24))
            SET_SETTING(1, set_fps(30))
            SET_SETTING(2, set_fps(40))
            SET_SETTING(3, set_fps(50))
            SET_SETTING(4, set_fps(60))
#else
            SET_SETTING(0, set_fps(30))
            SET_SETTING(1, set_fps(40))
            SET_SETTING(2, set_fps(60))
            SET_SETTING(3, set_fps(120))
#endif
            DEFAULT
        }
    });

    std::vector<VideoCodec> supportedCodecs = {
        H264,
#if !defined(__PSV__)
        H265,
#endif
    };
    std::vector<std::string> supportedCodecNames;
    int selectedCodec = 0;
    for (int i = 0; i < (int)supportedCodecs.size(); i++) {
        supportedCodecNames.push_back(getVideoCodecName(supportedCodecs[i]));
        if (supportedCodecs[i] == Settings::instance().video_codec())
            selectedCodec = i;
    }
    streamCodec->init(
        "settings/video_codec"_i18n, supportedCodecNames, selectedCodec,
        [supportedCodecs](int selected) {
            if (selected >= 0 && selected < (int)supportedCodecs.size())
                Settings::instance().set_video_codec(supportedCodecs[selected]);
        });

#if defined(__PSV__)
    const float streamBitrateMax = 20000;
#elif defined(PLATFORM_SWITCH)
    const float streamBitrateMax = 100000;
#else
    const float streamBitrateMax = 150000;
#endif
    const float streamBitrateOffset = 500;
    const float streamBitrateLimit = streamBitrateMax - streamBitrateOffset;

    auto updateStreamBitrateSubtitle = [this](int bitrate) {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(1)
               << (bitrate / 1000.0f);
        streamBitrateHeader->setSubtitle(stream.str() + " Mbps");
    };

    float streamBitrateProgress =
        (Settings::instance().bitrate() - streamBitrateOffset) /
        streamBitrateLimit;
    streamBitrateSlider->getProgressEvent()->subscribe(
        [this, streamBitrateOffset, streamBitrateLimit,
         updateStreamBitrateSubtitle](float progress) {
            int bitrate =
                progress * streamBitrateLimit + streamBitrateOffset;
            Settings::instance().set_bitrate(bitrate);
            updateStreamBitrateSubtitle(bitrate);
        });
    streamBitrateSlider->setProgress(streamBitrateProgress);
    updateStreamBitrateSubtitle(Settings::instance().bitrate());

    guideKeyButtons->setText("settings/guide_key_buttons"_i18n);
    setupButtonsSelectorCell(guideKeyButtons,
                             Settings::instance().guide_key_options().buttons);
    guideKeyButtons->registerClickAction([this](View* view) {
        ButtonSelectingDialog* dialog = ButtonSelectingDialog::create(
            "settings/guide_key_setup_message"_i18n, [this](auto buttons) {
                auto options = Settings::instance().guide_key_options();
                options.buttons = buttons;
                Settings::instance().set_guide_key_options(options);
                setupButtonsSelectorCell(guideKeyButtons, buttons);
            });

        dialog->open();
        return true;
    });

#ifndef PLATFORM_SWITCH
    guideBySystemButton->removeFromSuperView();
#else
    guideBySystemButton->init(
        "settings/use_system_button"_i18n,
        {"hints/off"_i18n, "settings/buttons/screenshot"_i18n, "settings/buttons/home"_i18n},
        (int) Settings::instance().get_guide_system_button(), [this](int value) {
            if (value != 0 && Settings::instance().get_overlay_system_button() == (ButtonOverrideType) value) {
                brls::sync([this, value](){
                    showError("settings/system_button_duplication_error"_i18n, [](){});
                });
                guideBySystemButton->setSelection((int) Settings::instance().get_guide_system_button(), true);
                return;
            }

            Settings::instance().set_guide_system_button((ButtonOverrideType) value);

            auto color = Settings::instance().get_guide_system_button() == ButtonOverrideType::NONE ?
                Application::getTheme()["brls/text_disabled"] : Application::getTheme()["brls/accent"];
            guideBySystemButton->setDetailTextColor(color);
        });
    auto color = Settings::instance().get_guide_system_button() == ButtonOverrideType::NONE ?
         Application::getTheme()["brls/text_disabled"] : Application::getTheme()["brls/accent"];
    guideBySystemButton->setDetailTextColor(color);
#endif

    volumeHeader->setSubtitle(
        std::to_string(Settings::instance().get_volume()) + "%");
    float amplification =
        Settings::instance().get_volume_amplification() ? 500.0f : 100.0f;
    float progress = (float) Settings::instance().get_volume() / amplification;
    volumeSlider->getProgressEvent()->subscribe(
        [this, amplification](float progress) {
            int volume = int(progress * amplification);
            Settings::instance().set_volume(volume);
            volumeHeader->setSubtitle(std::to_string(volume) + "%");
        });
    volumeSlider->setProgress(progress);

    float rumbleForceProgress = Settings::instance().get_rumble_force();
    rumbleForceSlider->getProgressEvent()->subscribe([this](float value) {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(1) << int(value * 100);
        rumbleForceHeader->setSubtitle(stream.str() + "%");
        Settings::instance().set_rumble_force(value);
    });
    rumbleForceSlider->setProgress(rumbleForceProgress);

    float mouseProgress =
        ((float) Settings::instance().get_mouse_speed_multiplier() / 100.0f);
    mouseSlider->getProgressEvent()->subscribe([this](float value) {
        float multiplier = value * 1.5f + 0.5f;
        std::stringstream stream;
        stream << std::fixed << std::setprecision(1) << multiplier;
        mouseHeader->setSubtitle("x" + stream.str());
        Settings::instance().set_mouse_speed_multiplier(int(value * 100));
    });
    mouseSlider->setProgress(mouseProgress);

    inputOverlayButton->setText("streaming/mouse_input"_i18n);
    inputOverlayButton->registerClickAction([this](View* view) {
        this->dismiss([this]() {
            auto* overlay =
                new StreamingInputOverlay(this->streamView);
            Application::pushActivity(new Activity(overlay));
        });
        return true;
    });

    std::vector<std::string> keyboardTypes = {
        "settings/keyboard_compact"_i18n, "settings/keyboard_fullsized"_i18n};
    keyboardType->setText("settings/keyboard_type"_i18n);
    keyboardType->setData(keyboardTypes);
    switch (Settings::instance().get_keyboard_type()) {
        GET_SETTINGS(keyboardType, COMPACT, 0)
        GET_SETTINGS(keyboardType, FULLSIZED, 1)
        DEFAULT
    }
    keyboardType->getEvent()->subscribe([](int selected) {
        switch (selected) {
            SET_SETTING(0, set_keyboard_type(COMPACT))
            SET_SETTING(1, set_keyboard_type(FULLSIZED))
            DEFAULT
        }
    });

    std::vector<std::string> keyboardFingersOptions = {
        "3", "4", "5", "hints/off"_i18n};
    keyboardFingers->setText("settings/keyboard_fingers"_i18n);
    keyboardFingers->setData(keyboardFingersOptions);
    switch (Settings::instance().get_keyboard_fingers()) {
        GET_SETTINGS(keyboardFingers, 3, 0)
        GET_SETTINGS(keyboardFingers, 4, 1)
        GET_SETTINGS(keyboardFingers, 5, 2)
        GET_SETTINGS(keyboardFingers, -1, 3)
        DEFAULT
    }
    keyboardFingers->getEvent()->subscribe([](int selected) {
        switch (selected) {
            SET_SETTING(0, set_keyboard_fingers(3))
            SET_SETTING(1, set_keyboard_fingers(4))
            SET_SETTING(2, set_keyboard_fingers(5))
            SET_SETTING(3, set_keyboard_fingers(-1))
            DEFAULT
        }
    });

    touchscreenMouseMode->init("settings/touchscreen_mouse_mode"_i18n,
                               Settings::instance().touchscreen_mouse_mode(),
                               [](bool value) {
                                   Settings::instance().set_touchscreen_mouse_mode(value);
                               });
    
    swapStickToDpad->init("settings/swap_stick_to_dpad"_i18n, Settings::instance().swap_joycon_stick_to_dpad(),
                          [](bool value) { Settings::instance().set_swap_joycon_stick_to_dpad(value); });

    onscreenLogButton->init("streaming/show_logs"_i18n,
                            Settings::instance().write_log(), [](bool value) {
                                Settings::instance().set_write_log(value);
                                brls::Application::enableDebuggingView(value);
                            });

    debugButton->init(
        "streaming/debug_info"_i18n, streamView->draw_stats,
        [streamView](bool value) { streamView->draw_stats = value; });

#ifdef SUPPORT_UPSCALING
    if (!isVideoUpscalingSupported()) {
        imageAdjustmentsHeader->removeFromSuperView(true);
        ditheringButton->removeFromSuperView(true);
        upscalingButton->removeFromSuperView(true);
        upscalingModeButton->removeFromSuperView(true);
        rcasButton->removeFromSuperView(true);
    } else {
        auto updateDitheringControls = [this](bool enabled) {
            updateStrengthControl(ditheringButton, enabled,
                                  getDitheringStrengthText(
                                      Settings::instance().dithering_strength()));
        };
        auto updateRcasControls = [this](bool enabled) {
            updateStrengthControl(rcasButton, enabled,
                                  getRcasStrengthText(
                                      Settings::instance().rcas_strength()));
        };

        ditheringButton->init(
            "settings/dithering"_i18n, Settings::instance().dithering(),
            [updateDitheringControls](bool value) {
                Settings::instance().set_dithering(value);
                updateDitheringControls(value);
            });

        const float ditheringStrength = Settings::instance().dithering_strength();
        ditheringButton->getProgressEvent()->subscribe([this](float value) {
            const float strength = sliderProgressToDitheringStrength(value);
            Settings::instance().set_dithering_strength(strength);
            updateStrengthControl(ditheringButton,
                                  Settings::instance().dithering(),
                                  getDitheringStrengthText(strength));
        });
        ditheringButton->setProgress(
            ditheringStrengthToSliderProgress(ditheringStrength));
        updateDitheringControls(Settings::instance().dithering());

#if defined(PLATFORM_APPLE) && !defined(PLATFORM_TVOS)
        upscalingButton->removeFromSuperView(true);
        upscalingModeButton->init(
            "settings/upscaling"_i18n,
            {"hints/off"_i18n, "MetalFX", "FSR1"},
            (int)Settings::instance().upscaling_mode(),
            [](int value) { Settings::instance().set_upscaling_mode((UpscalingMode)value); });
#else
        upscalingModeButton->removeFromSuperView(true);
        upscalingButton->init(
            "settings/upscaling"_i18n, Settings::instance().upscaling(),
            [](bool value) { Settings::instance().set_upscaling(value); });
#endif
        rcasButton->init(
            "settings/rcas_sharpening"_i18n, Settings::instance().rcas(),
            [updateRcasControls](bool value) {
                Settings::instance().set_rcas(value);
                updateRcasControls(value);
            });

        const float rcasStrength = Settings::instance().rcas_strength();
        rcasButton->getProgressEvent()->subscribe([this](float value) {
            Settings::instance().set_rcas_strength(value);
            updateStrengthControl(rcasButton,
                                  Settings::instance().rcas(),
                                  getRcasStrengthText(value));
        });
        rcasButton->setProgress(rcasStrength);
        updateRcasControls(Settings::instance().rcas());
    }
#else
    imageAdjustmentsHeader->removeFromSuperView(true);
    ditheringButton->removeFromSuperView(true);
    upscalingButton->removeFromSuperView(true);
    upscalingModeButton->removeFromSuperView(true);
    rcasButton->removeFromSuperView(true);
#endif
}

OptionsTab::~OptionsTab() { Settings::instance().save(); }

std::string
OptionsTab::getTextFromButtons(std::vector<ControllerButton> buttons) {
    std::string buttonsText;
    if (!buttons.empty()) {
        for (int i = 0; i < buttons.size(); i++) {
            buttonsText += brls::Hint::getKeyIcon(buttons[i], true);
            if (i < buttons.size() - 1)
                buttonsText += " + ";
        }
    } else {
        buttonsText = "hints/off"_i18n;
    }
    return buttonsText;
}

NVGcolor
OptionsTab::getColorFromButtons(const std::vector<brls::ControllerButton>& buttons) {
    Theme theme = Application::getTheme();
    return buttons.empty() ? theme["brls/text_disabled"]
                           : theme["brls/list/listItem_value_color"];
}

void OptionsTab::setupButtonsSelectorCell(brls::DetailCell* cell, const std::vector<ControllerButton>& buttons) {
    cell->setDetailText(getTextFromButtons(buttons));
    cell->setDetailTextColor(getColorFromButtons(buttons));
}
