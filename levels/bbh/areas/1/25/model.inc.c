#include "pc/rom_assets.h"
// 0x07015098 - 0x07015198
ROM_ASSET_LOAD_VTX(bbh_seg7_vertex_07015098, 0x00371c40, 68724, 0x00015098, 256);

// 0x07015198 - 0x07015298
ROM_ASSET_LOAD_VTX(bbh_seg7_vertex_07015198, 0x00371c40, 68724, 0x00015198, 256);

// 0x07015298 - 0x07015398
static const Gfx bbh_seg7_dl_07015298[] = {
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, spooky_09000000),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES)),
    gsSPVertex(bbh_seg7_vertex_07015098, 16, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  2,  1,  3, 0x0),
    gsSP2Triangles( 2,  3,  4, 0x0,  4,  3,  5, 0x0),
    gsSP2Triangles( 4,  5,  6, 0x0,  0,  7,  1, 0x0),
    gsSP2Triangles( 8,  7,  0, 0x0,  7,  9, 10, 0x0),
    gsSP2Triangles(11,  7, 10, 0x0,  8,  9,  7, 0x0),
    gsSP2Triangles( 8, 12,  9, 0x0,  8, 13, 12, 0x0),
    gsSP2Triangles(12, 14,  9, 0x0, 12, 15, 14, 0x0),
    gsSPVertex(bbh_seg7_vertex_07015198, 16, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  3,  4,  5, 0x0),
    gsSP2Triangles( 0,  6,  7, 0x0,  0,  7,  1, 0x0),
    gsSP2Triangles( 3,  7,  8, 0x0,  6,  8,  7, 0x0),
    gsSP2Triangles( 6,  9,  8, 0x0,  3,  8,  4, 0x0),
    gsSP2Triangles(10, 11, 12, 0x0, 10, 13, 11, 0x0),
    gsSP2Triangles(11, 14, 12, 0x0, 13, 15, 11, 0x0),
    gsSPEndDisplayList(),
};

// 0x07015398 - 0x07015408
const Gfx bbh_seg7_dl_07015398[] = {
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_MODULATERGBA, G_CC_MODULATERGBA),
    gsSPClearGeometryMode(G_LIGHTING | G_CULL_BACK),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPTileSync(),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, G_TX_RENDERTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, 5, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, 5, G_TX_NOLOD),
    gsDPSetTileSize(0, 0, 0, (32 - 1) << G_TEXTURE_IMAGE_FRAC, (32 - 1) << G_TEXTURE_IMAGE_FRAC),
    gsSPDisplayList(bbh_seg7_dl_07015298),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF),
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPSetGeometryMode(G_LIGHTING | G_CULL_BACK),
    gsSPEndDisplayList(),
};