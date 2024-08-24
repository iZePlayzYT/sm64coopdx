if not _G.charSelectExists then return end

E_MODEL_DAISY = smlua_model_util_get_id("daisy_geo")

local TEX_DAISY = get_texture_info("daisy_icon")

ACT_DAISY_JUMP = allocate_mario_action(ACT_GROUP_AIRBORNE | ACT_FLAG_ALLOW_VERTICAL_WIND_ACTION | ACT_FLAG_MOVING)

VOICETABLE_DAISY = {
    [CHAR_SOUND_ATTACKED] = {'daisy_attacked_1.ogg','daisy_attacked_2.ogg','daisy_attacked_3.ogg'},
    [CHAR_SOUND_COUGHING1] = 'daisy_coughing1.ogg',
    [CHAR_SOUND_COUGHING2] = 'daisy_coughing2.ogg',
    [CHAR_SOUND_COUGHING3] = 'daisy_coughing3.ogg',
    [CHAR_SOUND_DOH] = {'daisy_doh_1.ogg', 'daisy_doh_2.ogg'},
    [CHAR_SOUND_DROWNING] = 'daisy_drowning_1.ogg',
    [CHAR_SOUND_DYING] = 'daisy_dying.ogg',
    [CHAR_SOUND_EEUH] = 'daisy_eeuh.ogg',
    [CHAR_SOUND_GROUND_POUND_WAH] = 'daisy_ground_pound_wah.ogg',
    [CHAR_SOUND_HAHA] = 'daisy_haha.ogg',
    [CHAR_SOUND_HAHA_2] = 'daisy_haha_2.ogg',
    [CHAR_SOUND_HERE_WE_GO] = 'daisy_here_we_go.ogg',
    [CHAR_SOUND_HOOHOO] = 'daisy_hoohoo.ogg',
    [CHAR_SOUND_HRMM] = 'daisy_hrmm.ogg',
    [CHAR_SOUND_IMA_TIRED] = 'daisy_ima_tired.ogg',
    [CHAR_SOUND_MAMA_MIA] = 'daisy_mama_mia.ogg',
    [CHAR_SOUND_LETS_A_GO] = 'daisy_lets_a_go.ogg',
    [CHAR_SOUND_ON_FIRE] = 'daisy_on_fire.ogg',
    [CHAR_SOUND_OOOF] = 'daisy_oof.ogg',
    [CHAR_SOUND_OOOF2] = 'daisy_ooof2.ogg',
    [CHAR_SOUND_PANTING] = 'daisy_panting.ogg',
    [CHAR_SOUND_PANTING_COLD] = 'daisy_panting_cold.ogg',
    [CHAR_SOUND_PUNCH_HOO] = 'daisy_punch_hoo.ogg',
    [CHAR_SOUND_PUNCH_WAH] = 'daisy_punch_wah.ogg',
    [CHAR_SOUND_PUNCH_YAH] = 'daisy_punch_yah.ogg',
    [CHAR_SOUND_SO_LONGA_BOWSER] = 'daisy_so_longa_bowser.ogg',
    [CHAR_SOUND_SNORING1] = 'daisy_snoring1.ogg',
    [CHAR_SOUND_SNORING2] = 'daisy_snoring2.ogg',
    [CHAR_SOUND_SNORING3] = {'daisy_snoring2.ogg', 'daisy_snoring1.ogg', 'daisy_snoring3.ogg'},
    [CHAR_SOUND_TWIRL_BOUNCE] = 'daisy_twirl_bounce.ogg',
    [CHAR_SOUND_UH] = 'daisy_uh.ogg',
    [CHAR_SOUND_UH2] = 'daisy_uh2.ogg',
    [CHAR_SOUND_UH2_2] = 'daisy_uh2_2.ogg',
    [CHAR_SOUND_WAAAOOOW] = 'daisy_waaaooow.ogg',
    [CHAR_SOUND_WAH2] = 'daisy_wah2.ogg',
    [CHAR_SOUND_WHOA] = 'daisy_whoa.ogg',
    [CHAR_SOUND_YAHOO] = 'daisy_yahoo.ogg',
    [CHAR_SOUND_YAWNING] = 'daisy_yawning.ogg',
    [CHAR_SOUND_YAHOO_WAHA_YIPPEE] = { 'daisy_yahoo.ogg', 'daisy_yahoo2.ogg', 'daisy_yahoo3.ogg', 'daisy_yahoo4.ogg'},
    [CHAR_SOUND_YAH_WAH_HOO] = { 'daisy_yah1.ogg', 'daisy_yah2.ogg', 'daisy_yah3.ogg'},
    [CHAR_SOUND_HELLO] = 'daisy_double_jump.ogg'
}

--CAPS (Will be worked on in the future)--
--local capDAISY = {
    --normal = smlua_model_util_get_id("daisys_cap_geo"),
    --wing = smlua_model_util_get_id("daisys_wing_cap_geo"),
    --metal = smlua_model_util_get_id("daisys_metal_cap_geo"),
    --metalWing = smlua_model_util_get_id("daisys_metal_wing_cap_geo")
--}

local ANIMTABLE_DAISY = {
    [CHAR_ANIM_RUNNING] = 'daisy_running',
    [CHAR_ANIM_IDLE_HEAD_CENTER] = 'daisy_idle_head_center',
    [CHAR_ANIM_IDLE_HEAD_LEFT] = 'daisy_idle_head_left',
    [CHAR_ANIM_IDLE_HEAD_RIGHT] = 'daisy_idle_head_right',
    [CHAR_ANIM_BACKWARD_KB] = 'daisy_backward_kb',
    [CHAR_ANIM_FIRST_PERSON] = 'daisy_first_person',
    [CHAR_ANIM_FLUTTERKICK] = 'daisy_flutterkick',
    [CHAR_ANIM_FLUTTERKICK_WITH_OBJ] = 'daisy_flutterkick_with_obj',
    [CHAR_ANIM_FORWARD_KB] = 'daisy_forward_kb',
    [CHAR_ANIM_GROUND_BONK] = 'daisy_ground_bonk',
    [CHAR_ANIM_GROUND_KICK] = 'daisy_ground_kick',
    [CHAR_ANIM_GROUND_THROW] = 'daisy_ground_throw',
    [CHAR_ANIM_HEAVY_THROW] = 'daisy_heavy_throw',
    [CHAR_ANIM_MISSING_CAP] = 'daisy_missing_cap',
    [CHAR_ANIM_PULL_DOOR_WALK_IN] = 'daisy_pull_door_walk_in',
    [CHAR_ANIM_FIRST_PERSON] = 'daisy_first_person',
    [CHAR_ANIM_RELEASE_BOWSER] = 'daisy_release_bowser',
    [CHAR_ANIM_RETURN_FROM_STAR_DANCE] = 'daisy_return_from_star_dance',
    [CHAR_ANIM_RETURN_STAR_APPROACH_DOOR] = 'daisy_return_star_approach_door',
    [CHAR_ANIM_SLIDEFLIP_LAND] = 'daisy_sideflip_land',
    [CHAR_ANIM_SLEEP_IDLE] = 'daisy_sleep_idle',
    [CHAR_ANIM_START_SLEEP_SITTING] = 'daisy_sleep_sitting',
    [CHAR_ANIM_SOFT_BACK_KB] = 'daisy_soft_back_kb',
    [CHAR_ANIM_SOFT_FRONT_KB] = 'daisy_soft_front_kb',
    [CHAR_ANIM_STAR_DANCE] = 'daisy_star_dance',
    [CHAR_ANIM_START_CROUCHING] = 'daisy_start_crouch',
    [CHAR_ANIM_STOP_CROUCHING] = 'daisy_stop_crouch',
    [CHAR_ANIM_START_SLEEP_IDLE] = 'daisy_start_sleep',
    [CHAR_ANIM_STOP_SLIDE] = 'daisy_stop_slide',
    [CHAR_ANIM_SUMMON_STAR] = 'daisy_summon_star',
    [CHAR_ANIM_SWIM_WITH_OBJ_PART1] = 'daisy_swim_with_obj1',
    [CHAR_ANIM_SWIM_WITH_OBJ_PART2] = 'daisy_swim_with_obj2',
    [CHAR_ANIM_TRIPLE_JUMP_LAND] = 'daisy_triple_jump_land',
    [CHAR_ANIM_TURNING_PART1] = 'daisy_turning_part1',
    [CHAR_ANIM_TURNING_PART2] = 'daisy_turning_part2',
    [CHAR_ANIM_TWIRL_LAND] = 'daisy_twirl_land',
    [CHAR_ANIM_WAKE_FROM_LYING] = 'daisy_wake_from_lying',
    [CHAR_ANIM_WAKE_FROM_SLEEP] = 'daisy_wake_from_sleep',
    [CHAR_ANIM_CREDITS_RAISE_HAND] = 'daisy_credits_raise_hand',
    [CHAR_ANIM_CREDITS_RETURN_FROM_LOOK_UP] = 'daisy_credits_return_from_look_up',
    [CHAR_ANIM_CREDITS_START_WALK_LOOK_UP] = 'daisy_credits_start_walk_look_up',
    [CHAR_ANIM_CREDITS_TAKE_OFF_CAP] = 'daisy_credits_take_off_cap',
    [CHAR_ANIM_DYING_FALL_OVER] = 'daisy_dying_fall_over',
    [CHAR_ANIM_FALL_OVER_BACKWARDS] = 'daisy_fall_over_backwards',
    [CHAR_ANIM_FAST_LEDGE_GRAB] = 'daisy_fast_ledge_grab',
    --[CHAR_ANIM_FINAL_BOWSER_RAISE_HAND_SPIN] = 'daisy_final_bowser_raise_hand_spin',
    [CHAR_ANIM_FIRST_PUNCH_FAST] = 'daisy_first_punch_fast',
    [CHAR_ANIM_GENERAL_LAND] = 'daisy_general_land',
    [CHAR_ANIM_HEAD_STUCK_IN_GROUND] = 'daisy_head_stuck_in_ground',
    [CHAR_ANIM_LAND_FROM_DOUBLE_JUMP] = 'daisy_land_from_double_jump',
    [CHAR_ANIM_LAND_FROM_SINGLE_JUMP] = 'daisy_land_from_single_jump',
    [CHAR_ANIM_LAND_ON_STOMACH] = 'daisy_land_on_stomach',
    [CHAR_ANIM_LEGS_STUCK_IN_GROUND] = 'daisy_legs_stuck_in_ground',
    [CHAR_ANIM_PLACE_LIGHT_OBJ] = 'daisy_place_light_obj',
    [CHAR_ANIM_PUSH_DOOR_WALK_IN] = 'daisy_push_door_walk_in',
    [CHAR_ANIM_PUT_CAP_ON] = 'daisy_put_cap_on',
    [CHAR_ANIM_RUN_WITH_LIGHT_OBJ] = 'daisy_run_with_light_obj',
    [CHAR_ANIM_SECOND_PUNCH_FAST] = 'daisy_second_punch_fast',
    [CHAR_ANIM_SHIVERING_RETURN_TO_IDLE] = 'daisy_shivering_returning_to_idle',
    [CHAR_ANIM_SKID_ON_GROUND] = 'daisy_skid_on_ground',
    [CHAR_ANIM_SLOW_LAND_FROM_DIVE] = 'daisy_slow_land_from_dive',
    [CHAR_ANIM_SLOW_LEDGE_GRAB] = 'daisy_slow_ledge_grab',
    [CHAR_ANIM_SLOW_WALK_WITH_LIGHT_OBJ] = 'daisy_slow_walk_with_light_obj',
    [CHAR_ANIM_STAND_UP_FROM_LAVA_BOOST] = 'daisy_stand_up_from_lava_boost',
    [CHAR_ANIM_STOP_SKID] = 'daisy_stop_skid',
    [CHAR_ANIM_TAKE_CAP_OFF_THEN_ON] = 'daisy_take_cap_off_then_on',
    [CHAR_ANIM_THROW_CATCH_KEY] = 'daisy_throw_catch_key',
    [CHAR_ANIM_WALK_WITH_LIGHT_OBJ] = 'daisy_walk_with_light_obj',
    [CHAR_ANIM_BOTTOM_STUCK_IN_GROUND] = 'daisy_bottom_stuck_in_ground',
    --[CHAR_ANIM_CREDITS_PEACE_SIGN] = 'daisy_credits_peace_sign',
    [CHAR_ANIM_FIRE_LAVA_BURN] = 'daisy_fire_lava_burn',
    [CHAR_ANIM_GROUND_POUND] = 'daisy_ground_pound',
    [CHAR_ANIM_GROUND_POUND_LANDING] = 'daisy_ground_pound_landing',
    [CHAR_ANIM_TRIPLE_JUMP_GROUND_POUND] = 'daisy_triple_jump_ground_pound_anim',
    [CHAR_ANIM_UNLOCK_DOOR] = 'daisy_unlock_door',
    [CHAR_ANIM_SWINGING_BOWSER] = 'daisy_swinging_bowser',
    [CHAR_ANIM_HOLDING_BOWSER] = 'daisy_holding_bowser',
    [CHAR_ANIM_GRAB_BOWSER] = 'daisy_grab_bowser',
    [CHAR_ANIM_BEND_KNESS_RIDING_SHELL] = 'daisy_dressjump',
}

local PALETTE_DAISY = {
    [PANTS]  = { r = 0xFF, g = 0xFF, b = 0xFF },
    [SHIRT]  = { r = 0xEF, g = 0xCA, b = 0x11 },
    [GLOVES] = { r = 0xFF, g = 0xFF, b = 0xFF },
    [SHOES]  = { r = 0x00, g = 0x00, b = 0xFF },
    [HAIR]   = { r = 0xFF, g = 0x61, b = 0x00 },
    [SKIN]   = { r = 0xFD, g = 0xAE, b = 0x82 },
    [CAP]    = { r = 0xFF, g = 0x00, b = 0x00 },
    [EMBLEM] = { r = 0x00, g = 0xFF, b = 0xFF }
}

CT_DAISY = _G.charSelect.character_add("Princess Daisy", "The ruler of the Sarasaland, a tomboyish princess who brings enthusiasm and energy to every adventure! Voiced by MorphiGalaxi", "Melzinoff & MorphiGalaxi", {r = 255, g = 97, b = 0}, E_MODEL_DAISY, CT_MARIO, TEX_DAISY)
-- _G.charSelect.character_add_caps(E_MODEL_DAISY, capDAISY)
_G.charSelect.character_add_voice(E_MODEL_DAISY, VOICETABLE_DAISY)
_G.charSelect.character_add_palette_preset(E_MODEL_DAISY, PALETTE_DAISY)

--- @param m MarioState
local function act_daisy_jump(m)
    -- apply movement when using action
    common_air_action_step(m, ACT_JUMP_LAND, CHAR_ANIM_BEND_KNESS_RIDING_SHELL, AIR_STEP_NONE)

    -- setup when action starts (vertical speed and voiceline)
    if m.actionTimer == 0 then
        m.vel.y = 45
        play_character_sound(m, CHAR_SOUND_HELLO)
    end

    set_mario_particle_flags(m, PARTICLE_LEAF, 0)

    -- avoid issue with flying and then make the hover end after 2 secs or when stopping holding the button
    if m.prevAction ~= ACT_TRIPLE_JUMP and (m.flags & MARIO_WING_CAP) ~= 0 then
        if m.actionTimer >= 10 or (m.controller.buttonDown & A_BUTTON) == 0 then
            set_mario_action(m, ACT_FREEFALL, 0)
        end
    else
        if m.actionTimer >= 10 or (m.controller.buttonDown & A_BUTTON) == 0 then
            set_mario_action(m, ACT_FREEFALL, 0)
        end
    end

    -- increment the action timer to make the hover stop
    m.actionTimer = m.actionTimer + 1
end

--- @param m MarioState
function daisy_update(m)
    -- patch in custom animations
    local anim = ANIMTABLE_DAISY[m.marioObj.header.gfx.animInfo.animID]
    if anim ~= nil then
        smlua_anim_util_set_animation(m.marioObj, anim)
    end

    if (m.input & INPUT_A_PRESSED) ~= 0 and m.vel.y < 10 and m.prevAction ~= ACT_DAISY_JUMP and (
       m.action == ACT_JUMP or
       m.action == ACT_DOUBLE_JUMP or
       m.action == ACT_TRIPLE_JUMP or
       m.action == ACT_BACKFLIP or
       m.action == ACT_SIDE_FLIP) then
        set_mario_action(m, ACT_DAISY_JUMP, 0)
        set_mario_particle_flags(m, PARTICLE_LEAF, 0)
    end
end

hook_mario_action(ACT_DAISY_JUMP, act_daisy_jump)