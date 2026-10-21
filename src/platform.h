#ifndef BOUNCE_PLATFORM_H
#define BOUNCE_PLATFORM_H

#include <stdint.h>

typedef uint32_t u32;

#ifdef BOUNCE_NX
/* Keep the shared game independent of the console's native button masks. */
#define PSP_CTRL_SELECT   (1u << 0)
#define PSP_CTRL_START    (1u << 1)
#define PSP_CTRL_UP       (1u << 2)
#define PSP_CTRL_RIGHT    (1u << 3)
#define PSP_CTRL_DOWN     (1u << 4)
#define PSP_CTRL_LEFT     (1u << 5)
#define PSP_CTRL_LTRIGGER (1u << 6)
#define PSP_CTRL_RTRIGGER (1u << 7)
#define PSP_CTRL_TRIANGLE (1u << 8)
#define PSP_CTRL_CIRCLE   (1u << 9)
#define PSP_CTRL_CROSS    (1u << 10)
#define PSP_CTRL_SQUARE   (1u << 11)
#elif defined(BOUNCE_VITA)
#include <psp2/ctrl.h>
#define PSP_CTRL_SELECT   SCE_CTRL_SELECT
#define PSP_CTRL_START    SCE_CTRL_START
#define PSP_CTRL_UP       SCE_CTRL_UP
#define PSP_CTRL_RIGHT    SCE_CTRL_RIGHT
#define PSP_CTRL_DOWN     SCE_CTRL_DOWN
#define PSP_CTRL_LEFT     SCE_CTRL_LEFT
#define PSP_CTRL_LTRIGGER SCE_CTRL_LTRIGGER
#define PSP_CTRL_RTRIGGER SCE_CTRL_RTRIGGER
#define PSP_CTRL_TRIANGLE SCE_CTRL_TRIANGLE
#define PSP_CTRL_CIRCLE   SCE_CTRL_CIRCLE
#define PSP_CTRL_CROSS    SCE_CTRL_CROSS
#define PSP_CTRL_SQUARE   SCE_CTRL_SQUARE
#else
#include <pspctrl.h>
#include <psptypes.h>
#endif

#endif
