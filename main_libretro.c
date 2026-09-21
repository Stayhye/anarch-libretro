/**
  @file main_libretro.c

  This is a libretro (core for RetroArch) implementation of the game front end.

  To compile:

  gcc -Wall -O3 -fPIC -shared -o anarch_libretro.so main_libretro.c

  To run:

  retroarch -L ./anarch_libretro.so

  by 宋文武 (iyzsong), 2023

  Released under CC0 1.0 (https://creativecommons.org/publicdomain/zero/1.0/)
  plus a waiver of all other intellectual property. The goal of this work is
  be and remain completely in the public domain forever, available for any use
  whatsoever.
*/

#define SFG_FPS 60
#define SFG_LOG(str) puts(str);
#define SFG_SCREEN_RESOLUTION_X 320
#define SFG_SCREEN_RESOLUTION_Y 240
#define SFG_DITHERED_SHADOW 1
#define SFG_DIMINISH_SPRITES 1
#define SFG_PLAYER_DAMAGE_MULTIPLIER 1024
#define SFG_LOOP_NO_FRAMESKIP 1

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "libretro.h"
#include "game.h"
#include "sounds.h"


static void fallback_log(enum retro_log_level level, const char *fmt, ...);
static retro_log_printf_t log_cb = fallback_log;
static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_audio_sample_t audio_cb;
static int64_t _time = 0;
static bool _music = true;
static uint16_t _fb[SFG_SCREEN_RESOLUTION_X * SFG_SCREEN_RESOLUTION_Y * 2];
static uint16_t _ab[SFG_SFX_SAMPLE_COUNT];
static uint16_t _apos = 0;


void SFG_setPixel(uint16_t x, uint16_t y, uint8_t colorIndex)
{
    _fb[y * SFG_SCREEN_RESOLUTION_X + x] = paletteRGB565[colorIndex];
}

uint32_t SFG_getTimeMs()
{
    return _time / 1000;
}

void SFG_save(uint8_t data[SFG_SAVE_SIZE])
{
}

uint8_t SFG_load(uint8_t data[SFG_SAVE_SIZE])
{
    return 1;
}

void SFG_sleepMs(uint16_t timeMs)
{
}

void SFG_getMouseOffset(int16_t *x, int16_t *y)
{
    *x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
    *y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
}

void SFG_processEvent(uint8_t event, uint8_t data)
{
}

int8_t SFG_keyPressed(uint8_t key)
{
#define js(x) input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_ ## x)
#define mouse(x) input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_ ## x)
#define kbd(x) input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_ ## x)
    switch (key) {
    case SFG_KEY_UP:
        return js(UP) || kbd(w) || kbd(UP);
    case SFG_KEY_RIGHT:
        return js(RIGHT) || kbd(e) || kbd(RIGHT);
    case SFG_KEY_DOWN:
        return js(DOWN) || kbd(s) || kbd(DOWN);
    case SFG_KEY_LEFT:
        return js(LEFT) || kbd(q) || kbd(LEFT);
    case SFG_KEY_A:
        return js(A) || mouse(LEFT) || kbd(j) || kbd(LCTRL);
    case SFG_KEY_B:
        return js(B) || kbd(k) || kbd(LSHIFT);
    case SFG_KEY_C:
        return js(X) || kbd(l);
    case SFG_KEY_JUMP:
        return js(Y) || kbd(SPACE);
    case SFG_KEY_PREVIOUS_WEAPON:
        return js(L) || mouse(WHEELUP);
    case SFG_KEY_NEXT_WEAPON:
        return js(R) || mouse(WHEELDOWN) || kbd(p);
    case SFG_KEY_CYCLE_WEAPON:
        return mouse(MIDDLE) || kbd(f);
    case SFG_KEY_MAP:
        return js(START) || kbd(TAB);
    case SFG_KEY_TOGGLE_FREELOOK:
        return mouse(RIGHT);
    case SFG_KEY_MENU:
        return js(SELECT) || kbd(ESCAPE);
    default:
        return 0;
    }
#undef js
#undef mouse
#undef kbd
}

void SFG_setMusic(uint8_t value)
{
    switch (value) {
    case SFG_MUSIC_TURN_ON:
        _music = true;
        break;
    case SFG_MUSIC_TURN_OFF:
        _music = false;
        break;
    case SFG_MUSIC_NEXT:
        SFG_nextMusicTrack();
        break;
    default:
        break;
    }
}

void SFG_playSound(uint8_t soundIndex, uint8_t volume)
{
    uint16_t pos = _apos;
    uint16_t volumeScale = 1 << (volume / 37);
    for (int i = 0; i < SFG_SFX_SAMPLE_COUNT; ++i) {
        _ab[pos] += (128 - SFG_GET_SFX_SAMPLE(soundIndex, i)) * volumeScale;
        pos = (pos < SFG_SFX_SAMPLE_COUNT - 1) ? (pos + 1) : 0;
    }
}

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
    (void)level;
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

static void on_frame_time(retro_usec_t usec)
{
    _time += usec;
}

void retro_set_environment(retro_environment_t cb)
{
    static struct retro_log_callback log;
    environ_cb = cb;
    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
        log_cb = log.log;

    static bool t = true;
    environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &t);

    static struct retro_frame_time_callback frame_time_cb = {
        .callback = on_frame_time,
        .reference = 1000000 / SFG_FPS,
    };
    environ_cb(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &frame_time_cb);
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
    input_state_cb = cb;
}

void retro_get_system_info(struct retro_system_info *info)
{
    info->need_fullpath = false;
    info->valid_extensions = "";
    info->library_version = "0.1";
    info->library_name = "anarch";
    info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    int width = SFG_SCREEN_RESOLUTION_X;
    int height = SFG_SCREEN_RESOLUTION_Y;
    info->geometry.base_width = width;
    info->geometry.base_height = height;
    info->geometry.max_width = width;
    info->geometry.max_height = height;
    info->geometry.aspect_ratio = 0.0;
    info->timing.fps = SFG_FPS;
    info->timing.sample_rate = 8000;

    static enum retro_pixel_format pixfmt = RETRO_PIXEL_FORMAT_RGB565;
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixfmt);
}

void retro_init(void)
{
}

bool retro_load_game(const struct retro_game_info *game)
{
    SFG_init();
    return true;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
}

void retro_deinit(void)
{
}

void retro_reset(void)
{
}

void retro_run(void)
{
    input_poll_cb();
    if (!SFG_mainLoopBody())
        environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
    video_cb(_fb,
             SFG_SCREEN_RESOLUTION_X, SFG_SCREEN_RESOLUTION_Y,
             SFG_SCREEN_RESOLUTION_X * 2);
    for (int i = 0; i < 8 * SFG_MS_PER_FRAME; ++i) {
        uint16_t x = _ab[_apos];
        if (_music)
            x += 32 * (SFG_getNextMusicSample() - SFG_musicTrackAverages[SFG_MusicState.track]);
        audio_cb(x, x);
        _ab[_apos] = 0;
        _apos = (_apos < SFG_SFX_SAMPLE_COUNT - 1) ? (_apos + 1) : 0;
    }
}

size_t retro_serialize_size(void)
{
    return 0;
}

bool retro_serialize(void *data, size_t size)
{
    return false;
}

bool retro_unserialize(const void *data, size_t size)
{
    return false;
}

void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char *code) {}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
    return false;
}

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC;
}

void *retro_get_memory_data(unsigned id)
{
    switch (id) {
    case RETRO_MEMORY_SAVE_RAM:
        return SFG_game.save;
    default:
        return NULL;
    }
}

size_t retro_get_memory_size(unsigned id)
{
    switch (id) {
    case RETRO_MEMORY_SAVE_RAM:
        return SFG_SAVE_SIZE;
    default:
        return 0;
    }

}
