// Per-effect RANDOM-COLOR selector byte (PCAP live-toggle proven):
// each effect flips ONE commit byte 00->07 when Random Color is chosen.
#pragma once
#include <QtCore>

struct GKRandomSel { unsigned char pos; };
static const QMap<unsigned char, unsigned char> GK_RANDOM_SELECTORS = {
    { 0x02, 40 },  // Breathing   [40] 00->07
    { 0x08, 52 },  // Twinkling Stars [52] 00->07
    { 0x0A, 56 },  // Snake       [56] 00->07
    { 0x14, 76 },  // Flashing    [76] 00->07
    { 0x04, 52 },  // Highlight - same flag group as stars (guess)
    { 0x05, 52 },  // Rain        - same flag group as stars (guess)
};
static const unsigned char GK_RANDOM_SELECTOR_DEFAULT = 76;

inline unsigned char GKRandomSelectorPos(unsigned char effect_id)
{
    auto it = GK_RANDOM_SELECTORS.constFind(effect_id);
    return (it != GK_RANDOM_SELECTORS.constEnd()) ? it.value() : GK_RANDOM_SELECTOR_DEFAULT;
}
