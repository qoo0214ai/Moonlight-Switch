//
//  main_args.hpp
//  Moonlight
//
//  Created by XITRIX on 22.01.2024.
//


#pragma once

#include <string>

void registerDeepLinkHandler();

bool startFromArgs(int argc, char** argv);

// iOS Switch3 build: launch the saved Qoo HOME/Switch3 favorite directly
// when the app is opened from the iPad Home Screen.
bool startSwitch3DirectLaunch();
bool startFromUrl(const std::string& url, bool resetActivityStack = true);
