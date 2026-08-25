// Per-effect RANDOM-COLOR selector byte.
// PCAP-proven toggles: breathing[40], stars[52], snake[56], flashing[76].
// FORMULA: selector = 36 + 2*effect_id (decimal). Verified on all four:
//   0x02->40, 0x08->52, 0x0A->56, 0x14->76.
// Derived (untested): rain 46, shadow 54, trail 60, scan 64,
// explosion 72, collision 74, carousel 62, waterfall 68...
#pragma once

inline unsigned char GKRandomSelectorPos(unsigned char effect_id)
{
    unsigned int pos = 36u + 2u * (unsigned int)effect_id;
    return (pos <= 250u) ? (unsigned char)pos : 76;
}
