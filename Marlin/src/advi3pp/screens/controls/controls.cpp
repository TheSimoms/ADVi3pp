/**
 * ADVi3++ Firmware For Wanhao Duplicator i3 Plus (based on Marlin 2)
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

#include "../../parameters.h"
#include "../../../lcd/extui/ui_api.h"
#include "controls.h"
#include "../../core/dgus.h"
#include "../../core/status.h"
#include "../core/wait.h"
#include "../print/temperatures.h"
#include "../print/sd_card.h"
#include "../settings/print_settings.h"

namespace ADVi3pp {

Controls controls;

//! Dispatch a key value to the right handler
//! @param key_value    The sub-action to handle
//! @return             True if the action was handled
bool Controls::do_dispatch(KeyValue key_value)
{
    // Do not call Parent::do_dispatch

    switch(key_value)
    {
        case KeyValue::Temps:           show_temps(); break;
        case KeyValue::Print:           show_print(); break;
        case KeyValue::Controls:        pages.show(Page::Controls); break;
        case KeyValue::Tuning:          pages.show(Page::Tuning); break;
        case KeyValue::Settings:        pages.show(Page::Settings); break;
        case KeyValue::Infos:           pages.show(Page::Infos); break;
        case KeyValue::Motors:          pages.show(Page::MotorsSettings); break;
        case KeyValue::Leveling:        pages.show(Page::Leveling); break;
        case KeyValue::PrintSettings:   show_print_settings(); break;
        case KeyValue::Back:            back_command(); break;
        default:                        return false;
    }

    return true;
}

//! Show one of the temperature graph screens depending of the context: either the SD printing screen,
//! the printing screen or the temperature screen.
void Controls::show_temps()
{
    if(!ExtUI::isPrinting() && !ExtUI::isPrintingPaused())
    {
        temperatures.show();
        return;
    }

    // If there is a print running (or paused), display the print screen.
    pages.show(Page::Print);
}

//! Show Print Settings page (only if a print is running or paused)
void Controls::show_print_settings()
{
    if(!ExtUI::isPrinting() && !ExtUI::isPrintingPaused())
    {
        temperatures.show();
        return;
    }

    // If there is a print running (or paused), display the print settings.
    print_settings.show();
}

//! Show one of the Printing screens depending of the context:
//! - If a print is running, display the Print screen
//! - Otherwise, try to access the SD card. Depending of the result, display the SD card Page or the Temperatures page
void Controls::show_print()
{
    // If there is a print running (or paused), display the SD or USB print screen
    if(ExtUI::isPrinting() || ExtUI::isPrintingPaused())
    {
        pages.show(Page::Print);
        return;
    }

    wait.wait(F("Accessing the SD card..."));
    background_task.set(Callback {this, &Controls::show_sd});
}

//! Show the SD card page (if a SD card is inserted)
void Controls::show_sd()
{
    background_task.clear();

    ExtUI::mountMedia();
    ExtUI::FileList{}.refresh();
    status.reset();
    if(!ExtUI::isMediaInserted())
    {
        status.set(F("No SD card detected."));
        pages.show_back_page();
        return;
    }

    sd_card.show();
}

}
