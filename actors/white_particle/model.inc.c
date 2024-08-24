#include "pc/rom_assets.h"
// White Particle

// 0x0302C660
ROM_ASSET_LOAD_VTX(white_particle_vertex, 0x00201410, 96653, 0x0002c660, 64);

// 0x0302C6A0
ROM_ASSET_LOAD_TEXTURE(white_particle_texture, "actors/white_particle/snow_particle.rgba16.inc.c", 0x00201410, 96653, 0x0002c6a0, 512);

// 0x0302C8A0 - 0x0302C938
const Gfx white_particle_dl[] = {
    gsDPPipeSync(),
    gsSPClearGeometryMode(G_LIGHTING),
    gsDPSetCombineMode(G_CC_MODULATERGBA, G_CC_MODULATERGBA),
    gsDPLoadTextureBlock(white_particle_texture, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 16, 0, G_TX_CLAMP, G_TX_CLAMP, 4, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsSPVertex(white_particle_vertex, 4, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  0,  2,  3, 0x0),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF),
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPSetGeometryMode(G_LIGHTING),
    gsSPEndDisplayList(),
};