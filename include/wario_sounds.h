#ifndef WARIO_SOUNDS_H
#define WARIO_SOUNDS_H

/* Wario Sound Effects */
// A random number 0-2 is added to the sound ID before playing, producing Yah/Wah/Hoo
#define SOUND_WARIO_YAH_WAH_HOO                 SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x00, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_HOOHOO                      SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x03, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_YAHOO                       SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x04, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_UH                          SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x05, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_HRMM                        SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x06, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_WAH2                        SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x07, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_WHOA                        SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x08, 0xC0, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_EEUH                        SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x09, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_ATTACKED                    SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x0A, 0xFF, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_OOOF                        SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x0B, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_OOOF2                       SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x0B, 0xD0, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_HERE_WE_GO                  SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x0C, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_YAWNING                     SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x0D, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_SNORING1                    SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x0E, 0x00, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_SNORING2                    SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x0F, 0x00, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_WAAAOOOW                    SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x10, 0xC0, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_HAHA                        SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x11, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_HAHA_2                      SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x11, 0xF0, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_UH2                         SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x13, 0xD0, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_UH2_2                       SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x13, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_ON_FIRE                     SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x14, 0xA0, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_DYING                       SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x15, 0xFF, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_PANTING_COLD                SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x16, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)

// A random number 0-2 is added to the sound ID before playing
#define SOUND_WARIO_PANTING                     SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x18, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)

#define SOUND_WARIO_COUGHING1                   SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x1B, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_COUGHING2                   SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x1C, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_COUGHING3                   SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x1D, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_PUNCH_YAH                   SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x1E, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_PUNCH_HOO                   SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x1F, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_MAMA_MIA                    SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x20, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_OKEY_DOKEY                  SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x21, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_NO_VOLUME_LOSS | SOUND_CONSTANT_FREQUENCY | SOUND_DISCRETE)
#define SOUND_WARIO_GROUND_POUND_WAH            SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x22, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_DROWNING                    SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x23, 0xF0, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_PUNCH_WAH                   SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x24, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)

// A random number 0-4 is added to the sound ID before playing, producing one of
// Yahoo! (60% chance), Waha! (20%), or Yippee! (20%).
#define SOUND_WARIO_YAHOO_WAHA_YIPPEE           SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x2B, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)

#define SOUND_WARIO_DOH                         SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x30, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_GAME_OVER                   SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x31, 0xFF, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_HELLO                       SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x32, 0xFF, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_PRESS_START_TO_PLAY         SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x33, 0xFF, SOUND_NO_PRIORITY_LOSS | SOUND_NO_ECHO | SOUND_DISCRETE)
#define SOUND_WARIO_TWIRL_BOUNCE                SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x34, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_SNORING3                    SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x35, 0x00, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_SO_LONGA_BOWSER             SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x36, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)
#define SOUND_WARIO_IMA_TIRED                   SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x37, 0x80, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)

#define SOUND_WARIO_LETS_A_GO                   SOUND_ARG_LOAD(SOUND_BANK_WARIO_VOICE, 0x38, 0xFF, SOUND_NO_PRIORITY_LOSS | SOUND_DISCRETE)

#endif // WARIO_SOUNDS_H