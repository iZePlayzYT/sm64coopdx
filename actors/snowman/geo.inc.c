// 0x0C00021C
const GeoLayout mr_blizzard_hidden_geo[] = {
   GEO_SCALE(0x00, 16384),
   GEO_OPEN_NODE(),
      GEO_ANIMATED_PART(LAYER_OPAQUE, 0, 0, 0, NULL),
      GEO_OPEN_NODE(),
         GEO_ANIMATED_PART(LAYER_OPAQUE, 0, 0, 0, NULL),
         GEO_OPEN_NODE(),
            GEO_BILLBOARD(),
            GEO_OPEN_NODE(),
               GEO_DISPLAY_LIST(LAYER_ALPHA, snowman_seg5_dl_0500C620),
            GEO_CLOSE_NODE(),
         GEO_CLOSE_NODE(),
         GEO_OPEN_NODE(),
            GEO_ANIMATED_PART(LAYER_OPAQUE, 356, 0, 0, NULL),
            GEO_OPEN_NODE(),
               GEO_BILLBOARD(),
               GEO_OPEN_NODE(),
                  GEO_DISPLAY_LIST(LAYER_ALPHA, snowman_seg5_dl_0500CBF8),
               GEO_CLOSE_NODE(),
            GEO_CLOSE_NODE(),
            GEO_ANIMATED_PART(LAYER_ALPHA, 356, 0, 0, snowman_seg5_dl_0500CCE8),
            GEO_ANIMATED_PART(LAYER_ALPHA, 356, 0, 0, snowman_seg5_dl_0500C760),
            GEO_SWITCH_CASE(2, geo_switch_anim_state),
            GEO_OPEN_NODE(),
               GEO_NODE_START(),
               GEO_NODE_START(),
               GEO_OPEN_NODE(),
                  GEO_SCALE(0x00, 65536),
                  GEO_OPEN_NODE(),
                     GEO_ASM(0, geo_mario_set_player_colors),
                     GEO_TRANSLATE_ROTATE_WITH_DL(LAYER_ALPHA, 490, 14, 43, 305, 0, 248, mario_cap_m_logo_decal),
                     GEO_TRANSLATE_ROTATE_WITH_DL(LAYER_OPAQUE, 490, 14, 43, 305, 0, 248, mario_cap_seg3_dl_03022F48),
                  GEO_CLOSE_NODE(),
               GEO_CLOSE_NODE(),
            GEO_CLOSE_NODE(),
            GEO_ANIMATED_PART(LAYER_OPAQUE, 89, 0, -229, NULL),
            GEO_OPEN_NODE(),
               GEO_ANIMATED_PART(LAYER_OPAQUE, 0, 0, 0, snowman_seg5_dl_0500C500),
               GEO_OPEN_NODE(),
                  GEO_ANIMATED_PART(LAYER_OPAQUE, 68, 0, 0, snowman_seg5_dl_0500CAA8),
                  GEO_ASM(0, geo_update_projectile_pos_from_parent),
               GEO_CLOSE_NODE(),
            GEO_CLOSE_NODE(),
            GEO_ANIMATED_PART(LAYER_OPAQUE, 356, 0, 0, NULL),
         GEO_CLOSE_NODE(),
      GEO_CLOSE_NODE(),
   GEO_CLOSE_NODE(),
   GEO_END(),
};

// 0x0C000348
const GeoLayout mr_blizzard_geo[] = {
   GEO_SHADOW(SHADOW_CIRCLE_4_VERTS, 0xB4, 160),
   GEO_OPEN_NODE(),
      GEO_BRANCH_AND_LINK(mr_blizzard_hidden_geo),
   GEO_CLOSE_NODE(),
   GEO_END(),
};
