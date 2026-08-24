// Per-effect commit flags extracted from vendor live-pick captures.
// Each effect has its own flag layout - there is NO universal pattern.
// Format: [40],[46],[52],[54],[56],[58],[64]  (76 handled separately)
#pragma once
#include <QtCore>

struct GKEffectFlags { unsigned char b40,b46,b52,b54,b56,b58,b64; };

static const QMap<unsigned char, GKEffectFlags> GK_EFFECT_FLAGS = {
    { 0x02, {0x07,0x00,0x07,0x00,0x07,0x07,0x00} },  // Breathing
    { 0x04, {0x07,0x00,0x00,0x00,0x07,0x07,0x00} },  // Highlight/Rainbow Wave
    { 0x05, {0x07,0x00,0x00,0x00,0x07,0x07,0x00} },  // Rain
    { 0x08, {0x07,0x00,0x00,0x00,0x07,0x07,0x00} },  // Twinkling Stars
    { 0x0A, {0x07,0x07,0x00,0x00,0x00,0x07,0x00} },  // Snake
    { 0x14, {0x07,0x07,0x00,0x00,0x07,0x07,0x00} },  // Flashing
};
// Default for effects without a capture (flashing-like layout):
static const GKEffectFlags GK_EFFECT_FLAGS_DEFAULT = {0x07,0x07,0x00,0x00,0x07,0x07,0x00};

inline void GKApplyEffectFlags(unsigned char* commit, unsigned char effect_id)
{
    auto it = GK_EFFECT_FLAGS.constFind(effect_id);
    const GKEffectFlags& f = (it != GK_EFFECT_FLAGS.constEnd()) ? it.value() : GK_EFFECT_FLAGS_DEFAULT;
    commit[40]=f.b40; commit[46]=f.b46; commit[52]=f.b52;
    commit[54]=f.b54; commit[56]=f.b56; commit[58]=f.b58; commit[64]=f.b64;
}
