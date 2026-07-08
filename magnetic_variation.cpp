/****************************************************************************
 *
 *   WMM_Tiny - A tiny implementation of the World Magnetic Model
 *   Copyright (c) 2018 John Blaiklock (MIT License)
 *
 ****************************************************************************/

#include "magnetic_variation.h"
#include <cmath>
#include <stdint.h>

namespace magnetic_variation {

namespace {

constexpr float PI_CONST = 3.14159265359f;
constexpr float RADIANS_TO_DEGREES = 0.017453292f;
constexpr float DEGREES_TO_RADIANS = (PI_CONST / 180.0f);
constexpr float A_CONST = 6378.137f;
constexpr float A2_CONST = (A_CONST * A_CONST);
constexpr float B_CONST = 6356.7523142f;
constexpr float B2_CONST = (B_CONST * B_CONST);
constexpr float RE_CONST = 6371.2f;
constexpr float A4_CONST = (A2_CONST * A2_CONST);
constexpr float B4_CONST = (B2_CONST * B2_CONST);
constexpr float C2_CONST = (A2_CONST - B2_CONST);
constexpr float C4_CONST = (A4_CONST - B4_CONST);
constexpr uint8_t COEFFICIENTS_COUNT = 90U;
constexpr float WMM_EPOCH = 2025.0f;

struct wmm_cof_record_t {
  float gnm;
  float hnm;
  float dgnm;
  float dhnm;
};

float c[13][13];
float cd[13][13];
float k[13][13];
float snorm[169];
float fn[13];
float fm[13];
wmm_cof_record_t wmm_cof_entries[COEFFICIENTS_COUNT];

// WMM-2025 coefficient values encoded using varint format
const uint8_t wmm_cof_entries_encoded[] = {
  0xCE, 0xEA, 0x23, 0x00, 0xB8, 0x01, 0x00, 0xDC, 0xDC, 0x01, 0x8E, 0xC6, 0x05, 0xA1, 0x01, 0xD7,
  0x03, 0xDE, 0x8F, 0x03, 0x00, 0xF4, 0x01, 0x00, 0x87, 0xCD, 0x03, 0xE8, 0xE9, 0x03, 0x74, 0xD5,
  0x04, 0xAD, 0x81, 0x02, 0xD7, 0x7F, 0xD0, 0x01, 0xF9, 0x01, 0xAA, 0xD4, 0x01, 0x00, 0x4D, 0x00,
  0xE9, 0xF7, 0x02, 0xF6, 0x08, 0x6A, 0x28, 0x96, 0xC2, 0x01, 0x87, 0x25, 0x04, 0x43, 0xB8, 0x46,
  0xF7, 0x55, 0xDC, 0x02, 0x69, 0xB6, 0x8B, 0x01, 0x00, 0x50, 0x00, 0xBB, 0x7C, 0xA2, 0x2B, 0x58,
  0x4B, 0xAD, 0x08, 0xFB, 0x14, 0x7C, 0x29, 0xFB, 0x2B, 0x88, 0x21, 0x38, 0x10, 0xB9, 0x01, 0xEC,
  0x3A, 0xC6, 0x01, 0x6C, 0xDC, 0x24, 0x00, 0x06, 0x00, 0xA9, 0x39, 0x86, 0x07, 0x0E, 0x45, 0x90,
  0x1D, 0x9A, 0x22, 0x00, 0x16, 0xEB, 0x15, 0xCD, 0x13, 0x06, 0x04, 0xCC, 0x16, 0xAE, 0x06, 0x16,
  0x11, 0x91, 0x03, 0xA5, 0x10, 0x09, 0x13, 0x84, 0x0A, 0x00, 0x42, 0x00, 0xBE, 0x09, 0xF8, 0x02,
  0x44, 0x03, 0x81, 0x0C, 0xA8, 0x02, 0x09, 0x50, 0xC5, 0x12, 0xA8, 0x07, 0x0C, 0x44, 0xD9, 0x06,
  0xD6, 0x09, 0x49, 0x09, 0x95, 0x02, 0xAD, 0x01, 0x03, 0x07, 0xDF, 0x09, 0x97, 0x0B, 0x09, 0x09,
  0x9B, 0x0C, 0x00, 0x00, 0x00, 0xC2, 0x0C, 0xE9, 0x07, 0x41, 0x06, 0xD8, 0x01, 0xD0, 0x02, 0x41,
  0x05, 0x91, 0x09, 0x4A, 0x05, 0x48, 0x9E, 0x02, 0xAA, 0x03, 0x41, 0x00, 0x19, 0xCA, 0x01, 0x48,
  0x4A, 0xEF, 0x01, 0xFB, 0x03, 0x48, 0x06, 0x8E, 0x02, 0x57, 0x08, 0x42, 0xA8, 0x03, 0x00, 0x41,
  0x00, 0xAC, 0x01, 0x87, 0x01, 0x02, 0x42, 0xEF, 0x02, 0xFE, 0x01, 0x00, 0x05, 0x14, 0xB2, 0x01,
  0x05, 0x44, 0xD9, 0x03, 0xE1, 0x01, 0x41, 0x04, 0xA9, 0x02, 0xBF, 0x01, 0x03, 0x45, 0x96, 0x02,
  0x07, 0x02, 0x46, 0xE8, 0x02, 0x74, 0x00, 0x03, 0x09, 0x27, 0x02, 0x02, 0x2E, 0x00, 0x00, 0x00,
  0x8E, 0x01, 0xF8, 0x03, 0x41, 0x43, 0x1E, 0xBA, 0x01, 0x01, 0x03, 0x42, 0x93, 0x01, 0x03, 0x43,
  0x59, 0x61, 0x43, 0x03, 0xC3, 0x02, 0x74, 0x00, 0x02, 0x18, 0x88, 0x01, 0x03, 0x41, 0x96, 0x01,
  0x46, 0x41, 0x42, 0xD7, 0x01, 0x08, 0x01, 0x04, 0xC1, 0x02, 0xA4, 0x01, 0x41, 0x01, 0x4D, 0x00,
  0x01, 0x00, 0xC0, 0x01, 0x21, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x14, 0x18, 0x01, 0x42, 0x4A,
  0x35, 0x00, 0x01, 0x46, 0xDB, 0x01, 0x43, 0x41, 0x49, 0x04, 0x00, 0x01, 0x0F, 0x6A, 0x41, 0x00,
  0x09, 0x66, 0x41, 0x41, 0x5B, 0x09, 0x00, 0x02, 0x67, 0xDB, 0x01, 0x00, 0x00, 0x1D, 0x00, 0x00,
  0x00, 0x4F, 0x00, 0x00, 0x00, 0x59, 0x1D, 0x00, 0x01, 0x18, 0x46, 0x00, 0x00, 0x46, 0x02, 0x00,
  0x01, 0x41, 0x05, 0x41, 0x00, 0x46, 0x43, 0x00, 0x00, 0x41, 0x4C, 0x00, 0x01, 0x0B, 0x51, 0x41,
  0x00, 0x4A, 0x5D, 0x41, 0x00, 0x42, 0x52, 0x41, 0x00, 0x1A, 0x57, 0x41, 0x00, 0x54, 0x00, 0x00,
  0x00, 0x42, 0x4D, 0x00, 0x00, 0x03, 0x07, 0x00, 0x00, 0x0C, 0x0A, 0x00, 0x41, 0x4D, 0x4E, 0x00,
  0x01, 0x06, 0x00, 0x00, 0x00, 0x06, 0x06, 0x01, 0x00, 0x05, 0x41, 0x00, 0x00, 0x41, 0x08, 0x00,
  0x00, 0x44, 0x01, 0x00, 0x00, 0x42, 0x4A, 0x41, 0x00, 0x4D, 0x01, 0x00, 0x00, 0x47, 0x02, 0x41,
  0x41
};

float convert_varint_to_float(const char** bytes) {
  float result;
  int32_t result_int;
  bool negative = false;
  bool first_byte = true;
  uint8_t shift = 0;

  do {
    if (first_byte) {
      if (**bytes & 0x40) {
        negative = true;
      }
      result_int = **bytes & 0x3f;
      shift = 6U;
      first_byte = false;
    } else {
      result_int += (uint32_t)(**bytes & 0x7f) << shift;
      shift += 7U;
    }

    if ((**bytes & 0x80) == 0U) {
      (*bytes)++;
      break;
    }
    (*bytes)++;
  } while (true);

  result = ((float)result_int) / 10.0f;
  if (negative) {
    result = -result;
  }
  return result;
}

}  // namespace

void Initialize() {
  uint8_t j;
  uint8_t m;
  uint8_t n;
  uint8_t D2;
  float gnm;
  float hnm;
  float dgnm;
  float dhnm;
  float flnmj;
  uint8_t i;
  const char* bytes = (const char*)&wmm_cof_entries_encoded[0];

  // unpack coefficients
  for (i = 0U; i < COEFFICIENTS_COUNT; i++) {
    wmm_cof_entries[i].gnm = convert_varint_to_float(&bytes);
    wmm_cof_entries[i].hnm = convert_varint_to_float(&bytes);
    wmm_cof_entries[i].dgnm = convert_varint_to_float(&bytes);
    wmm_cof_entries[i].dhnm = convert_varint_to_float(&bytes);
  }

  c[0][0] = 0.0f;
  cd[0][0] = 0.0f;

  j = 0U;
  for (n = 1U; n <= 12U; n++) {
    for (m = 0U; m <= n; m++) {
      gnm = wmm_cof_entries[j].gnm;
      hnm = wmm_cof_entries[j].hnm;
      dgnm = wmm_cof_entries[j].dgnm;
      dhnm = wmm_cof_entries[j].dhnm;
      j++;

      if (m <= n) {
        c[m][n] = gnm;
        cd[m][n] = dgnm;
        if (m != 0U) {
          c[n][m - 1U] = hnm;
          cd[n][m - 1U] = dhnm;
        }
      }
    }
  }

  // CONVERT SCHMIDT NORMALIZED GAUSS COEFFICIENTS TO UNNORMALIZED
  *snorm = 1.0f;
  for (n = 1U; n <= 12U; n++) {
    *(snorm + n) = *(snorm + n - 1U) * (float)(2U * n - 1U) / (float)n;
    j = 2U;
    m = 0U;
    for (D2 = n - m + 1U; D2 > 0U; D2--) {
      k[m][n] = (float)(((n - 1U) * (n - 1U)) - (m * m)) /
                (float)((2U * n - 1U) * (2U * n - 3U));
      if (m > 0U) {
        flnmj = (float)((n - m + 1U) * j) / (float)(n + m);
        *(snorm + n + m * 13U) = *(snorm + n + (m - 1U) * 13U) * std::sqrt(flnmj);
        j = 1U;
        c[n][m - 1U] = *(snorm + n + m * 13U) * c[n][m - 1U];
        cd[n][m - 1U] = *(snorm + n + m * 13U) * cd[n][m - 1U];
      }
      c[m][n] = *(snorm + n + m * 13U) * c[m][n];
      cd[m][n] = *(snorm + n + m * 13U) * cd[m][n];
      m += 1U;
    }
    fn[n] = (float)(n + 1U);
    fm[n] = (float)n;
  }
  k[1][1] = 0.0f;
}

float GetDecimalYear(uint8_t year, uint8_t month, uint8_t day) {
  float days_in_year = 365.0f;
  if (year % 4U == 0U) {
    days_in_year = 366.0f;
  }
  return (float)year + 2000.0f + (float)(month - 1U) / 12.0f +
         (float)(day - 1U) / (days_in_year);
}

float GetDeclination(float latitude_deg, float longitude_deg, float time_years) {
  float t = time_years;
  if (t < WMM_EPOCH) t = WMM_EPOCH;
  if (t > WMM_EPOCH + 5.0f) t = WMM_EPOCH + 5.0f;

  static float tc[13][13];
  static float sp[13];
  static float cp[13];
  static float dp[13][13];
  static float pp[13];
  float dt = t - WMM_EPOCH;
  float rlon = longitude_deg * DEGREES_TO_RADIANS;
  float rlat = latitude_deg * DEGREES_TO_RADIANS;
  float srlon = std::sin(rlon);
  float srlat = std::sin(rlat);
  float crlon = std::cos(rlon);
  float crlat = std::cos(rlat);
  float srlat2 = srlat * srlat;
  float crlat2 = crlat * crlat;
  sp[0] = 0.0f;
  sp[1] = srlon;
  cp[0] = 1.0f;
  cp[1] = crlon;
  dp[0][0] = 0.0f;
  pp[0] = 1.0f;

  // CONVERT FROM GEODETIC COORDS. TO SPHERICAL COORDS
  float q = std::sqrt(A2_CONST - C2_CONST * srlat2);
  float q2 = (A2_CONST / (B2_CONST)) * (A2_CONST / B2_CONST);
  float ct = srlat / std::sqrt(q2 * crlat2 + srlat2);
  float st = std::sqrt(1.0f - (ct * ct));
  float r2 = (A4_CONST - C4_CONST * srlat2) / (q * q);
  float r = std::sqrt(r2);
  float d = std::sqrt(A2_CONST * crlat2 + B2_CONST * srlat2);
  float ca = d / r;
  float sa = C2_CONST * crlat * srlat / (r * d);
  for (uint8_t m = 2U; m <= 12U; m++) {
    sp[m] = sp[1] * cp[m - 1U] + cp[1] * sp[m - 1U];
    cp[m] = cp[1] * cp[m - 1U] - sp[1] * sp[m - 1U];
  }
  float aor = RE_CONST / r;
  float ar = aor * aor;
  float br = 0.0f;
  float bt = 0.0f;
  float bp = 0.0f;
  float bpp = 0.0f;

  for (uint16_t n = 1U; n <= 12U; n++) {
    ar = ar * aor;
    uint8_t m = 0U;
    for (uint8_t D4 = n + 1U; D4 > 0U; D4--) {
      // COMPUTE UNNORMALIZED ASSOCIATED LEGENDRE POLYNOMIALS AND DERIVATIVES
      if (n == m) {
        *(snorm + n + m * 13U) = st * *(snorm + n - 1U + (m - 1U) * 13U);
        dp[m][n] =
            st * dp[m - 1U][n - 1U] + ct * *(snorm + n - 1U + (m - 1U) * 13U);
        goto S50;
      }
      if (n == 1U && m == 0U) {
        *(snorm + n + m * 13U) = ct * *(snorm + n - 1U + m * 13U);
        dp[m][n] = ct * dp[m][n - 1U] - st * *(snorm + n - 1U + m * 13U);
        goto S50;
      }
      if (n > 1U && n != m) {
        if (m > n - 2U) {
          *(snorm + n - 2U + m * 13U) = 0.0f;
        }
        if (m > n - 2U) {
          dp[m][n - 2U] = 0.0f;
        }
        *(snorm + n + m * 13U) = ct * *(snorm + n - 1U + m * 13U) -
                                 k[m][n] * *(snorm + n - 2U + m * 13U);
        dp[m][n] = ct * dp[m][n - 1U] - st * *(snorm + n - 1U + m * 13U) -
                   k[m][n] * dp[m][n - 2U];
      }
    S50:

      // TIME ADJUST THE GAUSS COEFFICIENTS
      tc[m][n] = c[m][n] + dt * cd[m][n];
      if (m != 0U) {
        tc[n][m - 1U] = c[n][m - 1U] + dt * cd[n][m - 1U];
      }

      // ACCUMULATE TERMS OF THE SPHERICAL HARMONIC EXPANSIONS
      float par = ar * *(snorm + n + m * 13U);
      float temp1;
      float temp2;

      if (m == 0) {
        temp1 = tc[m][n] * cp[m];
        temp2 = tc[m][n] * sp[m];
      } else {
        temp1 = tc[m][n] * cp[m] + tc[n][m - 1U] * sp[m];
        temp2 = tc[m][n] * sp[m] - tc[n][m - 1U] * cp[m];
      }

      bt = bt - ar * temp1 * dp[m][n];
      bp += (fm[m] * temp2 * par);
      br += (fn[n] * temp1 * par);

      // SPECIAL CASE: NORTH/SOUTH GEOGRAPHIC POLES
      if (st == 0.0f && m == 1U) {
        if (n == 1U) {
          pp[n] = pp[n - 1U];
        } else {
          pp[n] = ct * pp[n - 1U] - k[m][n] * pp[n - 2U];
        }
        bpp += (fm[m] * temp2 * ar * pp[n]);
      }
      m += 1U;
    }
  }
  if (st == 0.0f) {
    bp = bpp;
  } else {
    bp /= st;
  }

  // ROTATE MAGNETIC VECTOR COMPONENTS FROM SPHERICAL TO GEODETIC COORDINATES
  float bx = -bt * ca - br * sa;
  float by = bp;

  // COMPUTE DECLINATION
  return std::atan2(by, bx) / DEGREES_TO_RADIANS;
}

}  // namespace magnetic_variation
