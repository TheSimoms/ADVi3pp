/**
 * ADVi3++ Firmware For Wanhao Duplicator i3 Plus (based on Marlin)
 *
 * Copyright (C) 2017-2021 Sebastien Andrivet [https://github.com/andrivet/]
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../parameters.h"
#include "settings.h"
#include "../versions.h"
#include "../core/core.h"
#include "../core/dgus.h"
#include "../core/dimming.h"
#include "../screens/controls/preheat.h"
#include "../screens/settings/pid_settings.h"
#include "../screens/settings/eeprom_mismatch.h"
#include "../screens/tuning/setup.h"
#include "../screens/leveling/xtwist.h"


namespace ADVi3pp {

Settings settings;


void Settings::on_factory_reset()
{
    settings.reset();
    if(eeprom_mismatch.does_mismatch())
        return;

    pages.reset();
    setup.show();
}

void Settings::on_store_settings(ExtUI::eeprom_write write, int& eeprom_index, uint16_t& working_crc)
{
    settings.write(write, eeprom_index, working_crc);
}

bool Settings::on_load_settings(ExtUI::eeprom_read read, int& eeprom_index, uint16_t& working_crc, bool validating)
{
    if(validating)
        return settings.validate(read, eeprom_index, working_crc);
    settings.read(read, eeprom_index, working_crc);
    return true;
}

uint16_t Settings::on_sizeof_settings()
{
    return settings.size_of();
}

void Settings::on_settings_written(bool success)
{
}

void Settings::on_settings_loaded(bool success)
{
    if(!success)
        eeprom_mismatch.set_mismatch();
}

void Settings::on_settings_validated(bool success)
{
    if(!success)
        eeprom_mismatch.set_mismatch();
}

bool Settings::write(eeprom_write write, int& eeprom_index, uint16_t& working_crc)
{
    EepromWrite eeprom{write, eeprom_index, working_crc};

    eeprom.write(settings_version);
    pid_settings.write(eeprom);
    xtwist.write(eeprom);
    eeprom.write(features_);

    return true;
}

bool Settings::validate(eeprom_read read, int& eeprom_index, uint16_t& working_crc)
{
    bool valid = true;
    EepromRead eeprom{read, eeprom_index, working_crc};

    uint16_t version = 0;
    eeprom.read(version);
    if(version != settings_version)
        valid = false;

    if(!pid_settings.validate(eeprom))
        valid = false;

    if(!xtwist.validate(eeprom))
        valid = false;

    Feature features = Feature::None;
    eeprom.read(features);

    return valid;
}

void Settings::read(eeprom_read read, int& eeprom_index, uint16_t& working_crc)
{
    EepromRead eeprom{read, eeprom_index, working_crc};

    uint16_t version = 0;
    eeprom.read(version);
    pid_settings.read(eeprom);
    xtwist.read(eeprom);
    eeprom.read(features_);
}

//! Reset presets.
void Settings::reset()
{
    pid_settings.reset();
    xtwist.reset();
    features_ = DEFAULT_FEATURES;
}

//! Return the size of data specific to ADVi3++
uint16_t Settings::size_of() const
{
    return
            sizeof(settings_version) +
            pid_settings.size_of() +
            xtwist.size_of() +
            sizeof(features_);
}

//! Save the current settings permanently in EEPROM memory
void Settings::save()
{
    eeprom_mismatch.reset_mismatch();
    ExtUI::saveSettings();
}

//! Restore settings from EEPROM memory
void Settings::restore()
{
    // Note: Previously, M420 (bed leveling compensation) was reset by M501. It is no more the case.
    ExtUI::loadSettings();
}

Feature Settings::flip_features(Feature features)
{
    flip_bits(features_, features);
    return features_ & features;
}

bool Settings::is_feature_enabled(Feature features) const
{
    return test_all_bits(features_, features);
}

void Settings::send_lcd_values(Variable features)
{
    WriteRamRequest{features}.write_words(adv::array<uint16_t, 2>{
        static_cast<uint16_t>(features_),
        ExtUI::get_lcd_contrast()
    });
}

//! Get the last used temperature for the hotend or the bad
//! @param kind Kind of temperature: hotend or bed
//! @return The last used themperature
uint16_t Settings::get_last_used_temperature(TemperatureKind kind) const
{
    return last_used_temperature_[kind == TemperatureKind::Hotend];
}

//! To be called when a new temperature is selected as a target
//! @param kind Kind of temperature: hotend or bed
//! @param temperature The new target temperature
void Settings::on_set_temperature(TemperatureKind kind, uint16_t temperature)
{
    if(temperature == 0)
        return;
    last_used_temperature_[kind == TemperatureKind::Hotend] = temperature;
    pid_settings.choose_best_pid(kind, temperature);
}


}
