#ifndef STRUCT_DEFS_H
#define STRUCT_DEFS_H
// There are three channels with independed wave specifications which are mapped to either RGB or HSL
// Modulation of amplitude and velocity uses a triangle form, with t=0 having no modulation and the amp/lambda being maximally REDUCED at the triangle peak
// BEWARE that velocity_mod_ratio less than 1.0 may give rise to negative effective velocities because of the way v is used with time; the wave ends up
// where it would have been at time t which is less advanced than it was a t - dt.
typedef struct sChannelDef
{
    char waveform;             // 'c' for cosine, 't' for triangle, 's' for square, 'u' for uniform (constant - only amplitude properties apply)
    float ratio;                // applies to triangle and square waves. Gives the fraction along the wavelenth of the max for triangle, or is the mark:space ratio for square
    float lambda;              // wavelength in number of pixels
    float velocity;            // speed of the wave in pixels/s. Negative is towards the ESP32 and positive is away.
    float velocity_mod_depth;    // fraction of velocity REDUCTION at maximum modulation. 0 for no modulation. 0.9 will give 10% at max
    float velocity_mod_ratio;    // position of triangle peak. 0=ramp down, 0.5=symmetric triangle, 1=ramp up. BEWARE - see ***
    float velocity_mod_period_s; // period for modulation
    float velocity_mod_offset_s; // offset to apply to the period, effectively a phase shift. May be negative
    float amplitude;           // amplitude in range 0.0-1.0
    float amp_mod_depth;       // similar to lambda
    float amp_mod_ratio;
    float amp_mod_period_s;
    float amp_mod_offset_s;
    float phase;               // value in range -1.0 to 1.0 for fraction of wavelength by which the wave is shifted. Negative towards ESP32, positive away.
    // TODO add velocity modulation ???

} tChannelDef;

// Used internally with post-modulation lambda and amp
typedef struct sChannelDef2
{
    char waveform;             // 'c' for cosine, 't' for triangle, 's' for square, 'u' for uniform (constant - only amplitude properties apply)
    float ratio;                // applies to triangle and square waves. Gives the fraction along the wavelenth of the max for triangle, or is the mark:space ratio for square
    float lambda;              // wavelength in number of pixels
    float amplitude;           // amplitude in range 0.0-1.0
    float velocity;            // speed of the wave in pixels/s. Negative is towards the ESP32 and positive is away.
    float phase;               // value in range -1.0 to 1.0 for fraction of wavelength by which the wave is shifted. Negative towards ESP32, positive away.

} tChannelDef2;

// for persistence to Flash
typedef struct sWaveDef
{
    char channel_map;  // channels 1,2,3 map to: 'r' for RGB, 'h' for HSL
    tChannelDef channel1;
    tChannelDef channel2;
    tChannelDef channel3;
} tWaveDef;

// Storage for the current wave.
extern char channel_map;
// These should be initialised to 0/null elements.
extern tChannelDef c1;
extern tChannelDef c2;
extern tChannelDef c3;

#endif