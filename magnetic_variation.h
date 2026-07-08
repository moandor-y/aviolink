/****************************************************************************
 *
 *   WMM_Tiny - A tiny implementation of the World Magnetic Model
 *   Copyright (c) 2018 John Blaiklock (MIT License)
 *
 ****************************************************************************/

#pragma once

#include <stdint.h>

namespace magnetic_variation {

// Initialize the World Magnetic Model. Call once during setup.
void Initialize();

// Calculate the decimal year representation from GPS date.
// year: 2-digit representation of 21st century (e.g., 26 for 2026)
// month: 1 to 12
// day: 1 to 31
float GetDecimalYear(uint8_t year, uint8_t month, uint8_t day);

// Get the magnetic declination in degrees.
// latitude_deg: -90.0 (South) to 90.0 (North)
// longitude_deg: -180.0 (West) to 180.0 (East)
// time_years: date in decimal years (e.g. 2026.5)
float GetDeclination(float latitude_deg, float longitude_deg, float time_years);

}  // namespace magnetic_variation
