/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */


#include <math.h>
#include <stdio.h>

#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_audio_cfg.h"

ALLEGRO_DEBUG_CHANNEL("sound")

void _al_set_error(int error, char* string)
{
   ALLEGRO_ERROR("%s (error code: %d)\n", string, error);
}

ALLEGRO_AUDIO_DRIVER *_al_kcm_driver = NULL;

#if defined(ALLEGRO_CFG_KCM_OPENAL)
   extern struct ALLEGRO_AUDIO_DRIVER _al_kcm_openal_driver;
#endif
#if defined(ALLEGRO_CFG_KCM_ALSA)
   extern struct ALLEGRO_AUDIO_DRIVER _al_kcm_alsa_driver;
#endif
#if defined(ALLEGRO_CFG_KCM_OSS)
   extern struct ALLEGRO_AUDIO_DRIVER _al_kcm_oss_driver;
#endif
#if defined(ALLEGRO_CFG_KCM_DSOUND)
   extern struct ALLEGRO_AUDIO_DRIVER _al_kcm_dsound_driver;
#endif

/* Channel configuration helpers */
bool al_is_channel_conf(ALLEGRO_CHANNEL_CONF conf)
{
   return ((conf >= ALLEGRO_CHANNEL_CONF_1) && (conf <= ALLEGRO_CHANNEL_CONF_7_1));
}

size_t al_get_channel_count(ALLEGRO_CHANNEL_CONF conf)
{
   return (conf>>4)+(conf&0xF);
}

/* Depth configuration helpers */
size_t al_get_depth_size(ALLEGRO_AUDIO_DEPTH depth)
{
   switch (depth) {
      case ALLEGRO_AUDIO_DEPTH_INT8:
      case ALLEGRO_AUDIO_DEPTH_UINT8:
         return sizeof(int8_t);
      case ALLEGRO_AUDIO_DEPTH_INT16:
      case ALLEGRO_AUDIO_DEPTH_UINT16:
         return sizeof(int16_t);
      case ALLEGRO_AUDIO_DEPTH_INT24:
      case ALLEGRO_AUDIO_DEPTH_UINT24:
         return sizeof(int32_t);
      case ALLEGRO_AUDIO_DEPTH_FLOAT32:
         return sizeof(float);
      default:
         ASSERT(false);
         return 0;
   }
}

/* Returns a silent sample frame. */
int _al_kcm_get_silence(ALLEGRO_AUDIO_DEPTH depth)
{
   switch (depth) {
      case ALLEGRO_AUDIO_DEPTH_UINT8:
         return 0x80;
      case ALLEGRO_AUDIO_DEPTH_INT16:
         return 0x8000;
      case ALLEGRO_AUDIO_DEPTH_INT24:
         return 0x800000;
      default:
         return 0;
   }
}

static ALLEGRO_AUDIO_DRIVER_ENUM get_config_audio_driver(void)
{
   ALLEGRO_SYSTEM *sys = al_system_driver();
   const char *value;

   if (!sys || !sys->config)
      return ALLEGRO_AUDIO_DRIVER_AUTODETECT;

   value = al_get_config_value(sys->config, "sound", "driver");
   if (!value || value[0] == '\0')
      return ALLEGRO_AUDIO_DRIVER_AUTODETECT;

   if (0 == stricmp(value, "ALSA"))
      return ALLEGRO_AUDIO_DRIVER_ALSA;

   if (0 == stricmp(value, "OPENAL"))
      return ALLEGRO_AUDIO_DRIVER_OPENAL;

   if (0 == stricmp(value, "OSS"))
      return ALLEGRO_AUDIO_DRIVER_OSS;

   if (0 == stricmp(value, "DSOUND") || 0 == stricmp(value, "DIRECTSOUND"))
      return ALLEGRO_AUDIO_DRIVER_DSOUND;

   return ALLEGRO_AUDIO_DRIVER_AUTODETECT;
}

/* TODO: possibly take extra parameters
 * (freq, channel, etc) and test if you 
 * can create a voice with them.. if not
 * try another driver.
 */
static bool do_install_audio(ALLEGRO_AUDIO_DRIVER_ENUM mode)
{
   bool retVal;

   /* check to see if a driver is already installed and running */
   if (_al_kcm_driver) {
      _al_set_error(ALLEGRO_GENERIC_ERROR, "A driver already running");
      return false;
   }

   if (mode == ALLEGRO_AUDIO_DRIVER_AUTODETECT) {
      mode = get_config_audio_driver();
   }

   switch (mode) {
      case ALLEGRO_AUDIO_DRIVER_AUTODETECT:
         retVal = al_install_audio(ALLEGRO_AUDIO_DRIVER_ALSA);
         if (retVal)
            return retVal;
         retVal = al_install_audio(ALLEGRO_AUDIO_DRIVER_DSOUND);
         if (retVal)
            return retVal;
         retVal = al_install_audio(ALLEGRO_AUDIO_DRIVER_OSS);
         if (retVal)
            return retVal;
         retVal = al_install_audio(ALLEGRO_AUDIO_DRIVER_OPENAL);
         if (retVal)
            return retVal;
         _al_kcm_driver = NULL;
         return true;

      case ALLEGRO_AUDIO_DRIVER_OPENAL:
         #if defined(ALLEGRO_CFG_KCM_OPENAL)
            if (_al_kcm_openal_driver.open() == 0) {
               ALLEGRO_INFO("Using OpenAL driver\n"); 
               _al_kcm_driver = &_al_kcm_openal_driver;
               return true;
            }
            return false;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "OpenAL not available on this platform");
            return false;
         #endif

      case ALLEGRO_AUDIO_DRIVER_ALSA:
         #if defined(ALLEGRO_CFG_KCM_ALSA)
            if (_al_kcm_alsa_driver.open() == 0) {
               ALLEGRO_INFO("Using ALSA driver\n"); 
               _al_kcm_driver = &_al_kcm_alsa_driver;
               return true;
            }
            return false;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "ALSA not available on this platform");
            return false;
         #endif

      case ALLEGRO_AUDIO_DRIVER_OSS:
         #if defined(ALLEGRO_CFG_KCM_OSS)
            if (_al_kcm_oss_driver.open() == 0) {
               ALLEGRO_INFO("Using OSS driver\n");
               _al_kcm_driver = &_al_kcm_oss_driver;
               return true;
            }
            return false;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "OSS not available on this platform");
            return false;
         #endif

      case ALLEGRO_AUDIO_DRIVER_DSOUND:
         #if defined(ALLEGRO_CFG_KCM_DSOUND)
            if (_al_kcm_dsound_driver.open() == 0) {
               ALLEGRO_INFO("Using DirectSound driver\n"); 
               _al_kcm_driver = &_al_kcm_dsound_driver;
               return true;
            }
            return false;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "DirectSound not available on this platform");
            return false;
         #endif

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid audio driver");
         return false;
   }
}

/* Function: al_install_audio
 */
bool al_install_audio(ALLEGRO_AUDIO_DRIVER_ENUM mode)
{
   bool ret = do_install_audio(mode);
   if (ret) {
      _al_kcm_init_destructors();
   }
   return ret;
}

/* Function: al_uninstall_audio
 */
void al_uninstall_audio(void)
{
   if (_al_kcm_driver) {
      _al_kcm_shutdown_default_mixer();
      _al_kcm_shutdown_destructors();
      _al_kcm_driver->close();
      _al_kcm_driver = NULL;
   }
}

/* vim: set sts=3 sw=3 et: */