#pragma once

#include <per/qspi.h>

#include <util/EffectState.h>

struct Settings
{
    EffectState preset;
    uint32_t mod_duration = 1000;
};

Settings loadSettings();
void saveSettings(daisy::QSPIHandle& qspi, const Settings& settings);
