#ifndef FLOAT16_H
#define FLOAT16_H

#include <stdint.h>
#include <math.h>
#include <string.h>

// Define macros for easier management
#if defined(__F16C__)
#define USE_F16C 1
#else
#define USE_F16C 0
#endif

#if defined(__ARM_NEON)
#define USE_NEON 1
#else
#define USE_NEON 0
#endif

typedef uint16_t float16_t;

#if USE_F16C
#include <immintrin.h>
#include <stdio.h>

static inline float16_t float32_to_float16(float value) {
    __m128 iv = _mm_set_ss(value);
    __m128i resv = _mm_cvtps_ph(iv, _MM_FROUND_TO_NEAREST_INT);
    return (float16_t)_mm_extract_epi16(resv, 0);
}

static inline float float16_to_float32(float16_t value) {
    __m128i iv = _mm_cvtsi32_si128(value);
    __m128 resv = _mm_cvtph_ps(iv);
    return _mm_cvtss_f32(resv);
}

#elif USE_NEON
#include <arm_neon.h>
#include <stdio.h>

static inline float16_t float32_to_float16(float value) {
    float32x4_t input = vdupq_n_f32(value);
    float16x4_t result = vcvt_f16_f32(input);
    return vget_lane_f16(result, 0);
}

static inline float float16_to_float32(float16_t value) {
    float16x4_t input = vdup_n_f16(value);
    float32x4_t result = vcvt_f32_f16(input);
    return vgetq_lane_f32(result, 0);
}

#else
// Software emulation
static inline uint32_t float32_to_uint32(float value) {
    uint32_t result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static inline float uint32_to_float32(uint32_t value) {
    float result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static inline uint16_t get_sign_bit(uint32_t bits) {
    return (bits >> 31) & 0x1;
}

static inline uint16_t get_exponent_bits(uint32_t bits) {
    return (bits >> 23) & 0xFF;
}

static inline uint32_t get_mantissa_bits(uint32_t bits) {
    return bits & 0x7FFFFF;
}

static inline uint16_t float32_to_float16(float value) {
    uint32_t bits = float32_to_uint32(value);
    uint16_t sign = get_sign_bit(bits);
    uint16_t exp = get_exponent_bits(bits);
    uint32_t mant = get_mantissa_bits(bits);

    if (exp == 0xFF) {
        if (mant != 0) {
            return (sign << 15) | 0x7E00; // NaN
        } else {
            return (sign << 15) | 0x7C00; // Infinity
        }
    }

    int new_exp = exp - 127 + 15;
    if (new_exp >= 31) {
        return (sign << 15) | 0x7C00; // Overflow to infinity
    } else if (new_exp <= 0) {
        return (sign << 15); // Underflow to zero
    }

    return (sign << 15) | (new_exp << 10) | (mant >> 13);
}

static inline float float16_to_float32(uint16_t value) {
    uint16_t sign = (value >> 15) & 0x1;
    uint16_t exp = (value >> 10) & 0x1F;
    uint16_t mant = value & 0x3FF;

    if (exp == 0x1F) {
        if (mant != 0) {
            return NAN; // NaN
        } else {
            return sign ? -INFINITY : INFINITY; // Infinity
        }
    }

    if (exp == 0) {
        if (mant == 0) {
            return sign ? -0.0f : 0.0f; // Zero
        } else {
            // Denormalized
            float denorm = (float)mant / (1 << 10);
            return sign ? -denorm : denorm;
        }
    }

    uint32_t new_exp = exp + 127 - 15;
    uint32_t new_mant = mant << 13;

    uint32_t result = (sign << 31) | (new_exp << 23) | new_mant;
    return uint32_to_float32(result);
}
#endif

#endif // FLOAT16_H

