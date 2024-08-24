#include "pc/rom_assets.h"
// 0x07011758 - 0x07011770
static const Lights1 ttc_seg7_lights_07011758 = gdSPDefLights1(
    0x5a, 0x41, 0x23,
    0xb4, 0x82, 0x46, 0x28, 0x28, 0x28
);

// 0x07011770 - 0x07011788
static const Lights1 ttc_seg7_lights_07011770 = gdSPDefLights1(
    0x7f, 0x66, 0x32,
    0xff, 0xcc, 0x65, 0x28, 0x28, 0x28
);

// 0x07011788 - 0x070117A0
static const Lights1 ttc_seg7_lights_07011788 = gdSPDefLights1(
    0x49, 0x00, 0x00,
    0x93, 0x00, 0x00, 0x28, 0x28, 0x28
);

// 0x070117A0 - 0x070117B8
static const Lights1 ttc_seg7_lights_070117A0 = gdSPDefLights1(
    0x7f, 0x00, 0x00,
    0xff, 0x00, 0x00, 0x28, 0x28, 0x28
);

// 0x070117B8 - 0x07011838
ROM_ASSET_LOAD_VTX(ttc_seg7_vertex_070117B8, 0x0042cf20, 42199, 0x000117b8, 128);

// 0x07011838 - 0x070118B8
ROM_ASSET_LOAD_VTX(ttc_seg7_vertex_07011838, 0x0042cf20, 42199, 0x00011838, 128);

// 0x070118B8 - 0x07011958
ROM_ASSET_LOAD_VTX(ttc_seg7_vertex_070118B8, 0x0042cf20, 42199, 0x000118b8, 160);

// 0x07011958 - 0x070119F8
ROM_ASSET_LOAD_VTX(ttc_seg7_vertex_07011958, 0x0042cf20, 42199, 0x00011958, 160);

// 0x070119F8 - 0x07011B38
static const Gfx ttc_seg7_dl_070119F8[] = {
    gsDPSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, machine_09000800),
    gsDPLoadSync(),
    gsDPLoadBlock(G_TX_LOADTILE, 0, 0, 32 * 32 - 1, CALC_DXT(32, G_IM_SIZ_16b_BYTES)),
    gsSPLight(&ttc_seg7_lights_07011758.l, 1),
    gsSPLight(&ttc_seg7_lights_07011758.a, 2),
    gsSPVertex(ttc_seg7_vertex_070117B8, 8, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  0,  3,  1, 0x0),
    gsSP2Triangles( 4,  5,  1, 0x0,  4,  1,  3, 0x0),
    gsSP2Triangles( 0,  2,  6, 0x0,  0,  6,  7, 0x0),
    gsSPLight(&ttc_seg7_lights_07011770.l, 1),
    gsSPLight(&ttc_seg7_lights_07011770.a, 2),
    gsSPVertex(ttc_seg7_vertex_07011838, 8, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  0,  2,  3, 0x0),
    gsSP2Triangles( 4,  5,  6, 0x0,  4,  7,  5, 0x0),
    gsSPLight(&ttc_seg7_lights_07011788.l, 1),
    gsSPLight(&ttc_seg7_lights_07011788.a, 2),
    gsSPVertex(ttc_seg7_vertex_070118B8, 10, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  0,  2,  3, 0x0),
    gsSP2Triangles( 3,  2,  4, 0x0,  3,  4,  5, 0x0),
    gsSP2Triangles( 5,  4,  6, 0x0,  5,  6,  7, 0x0),
    gsSP2Triangles( 7,  6,  8, 0x0,  7,  8,  9, 0x0),
    gsSPLight(&ttc_seg7_lights_070117A0.l, 1),
    gsSPLight(&ttc_seg7_lights_070117A0.a, 2),
    gsSPVertex(ttc_seg7_vertex_07011958, 10, 0),
    gsSP2Triangles( 0,  1,  2, 0x0,  2,  3,  0, 0x0),
    gsSP2Triangles( 0,  4,  1, 0x0,  5,  6,  7, 0x0),
    gsSP2Triangles( 5,  8,  9, 0x0,  5,  9,  6, 0x0),
    gsSPEndDisplayList(),
};

// 0x07011B38 - 0x07011BE0
const Gfx ttc_seg7_dl_07011B38[] = {
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_2CYCLE),
    gsDPSetRenderMode(G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2),
    gsDPSetDepthSource(G_ZS_PIXEL),
    gsDPSetFogColor(200, 255, 255, 255),
    gsSPFogPosition(900, 1000),
    gsSPSetGeometryMode(G_FOG),
    gsDPSetCombineMode(G_CC_MODULATERGB, G_CC_PASS2),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOLOD),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPTileSync(),
    gsDPSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 8, 0, G_TX_RENDERTILE, 0, G_TX_CLAMP, 5, G_TX_NOLOD, G_TX_CLAMP, 5, G_TX_NOLOD),
    gsDPSetTileSize(0, 0, 0, (32 - 1) << G_TEXTURE_IMAGE_FRAC, (32 - 1) << G_TEXTURE_IMAGE_FRAC),
    gsSPDisplayList(ttc_seg7_dl_070119F8),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_AA_ZB_OPA_SURF, G_RM_NOOP2),
    gsSPClearGeometryMode(G_FOG),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPEndDisplayList(),
};