/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

/*
This version is for pigpio version 11+
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

#include "pigpio.h"
#include "command.h"

/* retv
  pigs          pipe
0 ""   <0 ERR   %d
1 ""   <0 ERR   %d
2 %d   <0 ERR   %d
3 %08X          %08X
4 %u            %u
5 HELP          HELP
*/

/* vfyt
 1 cmd
 2 cmd   %d
 3 cmd   %d %d
 4 cmd   %d %x
 6 HELP
 7 cmd   %x
 8 MODES %d %c
 9 PUD   %d %c
10 PROG  %s
*/

cmdInfo_t cmdInfo[]=
{
   /* num         str   vfyt retv ext */

   {PI_CMD_BC1,   "BC1",   7, 1,  0},
   {PI_CMD_BC2,   "BC2",   7, 1,  0},
   {PI_CMD_BR1,   "BR1",   1, 3,  0},
   {PI_CMD_BR2,   "BR2",   1, 3,  0},
   {PI_CMD_BS1,   "BS1",   7, 1,  0},
   {PI_CMD_BS2,   "BS2",   7, 1,  0},
   {PI_CMD_HELP,  "H",     6, 5,  0},
   {PI_CMD_HELP,  "HELP",  6, 5,  0},
   {PI_CMD_HWVER, "HWVER", 1, 4,  0},
   {PI_CMD_MODEG, "MG"   , 2, 2,  0},
   {PI_CMD_MODEG, "MODEG", 2, 2,  0},
   {PI_CMD_MODES, "M",     8, 0,  0},
   {PI_CMD_MODES, "MODES", 8, 0,  0},
   {PI_CMD_NB,    "NB",    4, 0,  0},
   {PI_CMD_NC,    "NC",    2, 0,  0},
   {PI_CMD_NO,    "NO",    1, 2,  0},
   {PI_CMD_NP,    "NP",    2, 0,  0},
   {PI_CMD_PFG,   "PFG",   2, 2,  0},
   {PI_CMD_PFS,   "PFS",   3, 2,  0},
   {PI_CMD_PIGPV, "PIGPV", 1, 4,  0},
   {PI_CMD_PRG,   "PRG",   2, 2,  0},
   {PI_CMD_PROC,  "PROC", 10, 2,  1},
   {PI_CMD_PROCD, "PROCD", 2, 2,  0},
   {PI_CMD_PROCR, "PROCR", 2, 2,  0},
   {PI_CMD_PROCS, "PROCS", 2, 2,  0},
   {PI_CMD_PRRG,  "PRRG",  2, 2,  0},
   {PI_CMD_PRS,   "PRS",   3, 2,  0},
   {PI_CMD_PUD,   "PUD",   9, 0,  0},
   {PI_CMD_PWM,   "P",     3, 0,  0},
   {PI_CMD_PWM,   "PWM",   3, 0,  0},
   {PI_CMD_READ,  "R",     2, 2,  0},
   {PI_CMD_READ,  "READ",  2, 2,  0},
   {PI_CMD_SERVO, "S",     3, 0,  0},
   {PI_CMD_SERVO, "SERVO", 3, 0,  0},
   {PI_CMD_WDOG,  "WDOG",  3, 0,  0},
   {PI_CMD_WRITE, "W",     3, 0,  0},
   {PI_CMD_WRITE, "WRITE", 3, 0,  0},
   {PI_CMD_TICK,  "T",     1, 4,  0},
   {PI_CMD_TICK,  "TICK",  1, 4,  0},
   {PI_CMD_TRIG,  "TRIG",  5, 0,  1},
   {PI_CMD_WVAS,  "WVAS", 11, 2,  3},
   {PI_CMD_WVBSY, "WVBSY", 1, 2,  0},
   {PI_CMD_WVCLR, "WVCLR", 1, 2,  0},
   {PI_CMD_WVGO,  "WVGO" , 1, 2,  0},
   {PI_CMD_WVGOR, "WVGOR", 1, 2,  0},
   {PI_CMD_WVHLT, "WVHLT", 1, 2,  0},
   {PI_CMD_WVSC,  "WVSC",  2, 2,  0},
   {PI_CMD_WVSM,  "WVSM",  2, 2,  0},
   {PI_CMD_WVSP,  "WVSP",  2, 2,  0},
};

char * cmdUsage = "\
BC1 x        clear gpios in bank 1\n\
BC2 x        clear gpios in bank 2\n\
BR1          read gpios bank 1\n\
BR2          read gpios bank 2\n\
BS1 x        set gpios in bank 1\n\
BS2 x        set gpios in bank 2\n\
H            displays command help\n\
HELP         displays command help\n\
HWVER        return hardware version\n\
M g m        set gpio mode\n\
MG g         get gpio mode\n\
MODEG g      get gpio mode\n\
MODES g m    set gpio mode\n\
NB h x       start notification\n\
NC h         close notification\n\
NO           request notification handle\n\
NP h         pause notification\n\
P u d        set PWM value for gpio\n\
PFG u        get PWM frequency for gpio\n\
PFS u d      set PWM frequency for gpio\n\
PIGPV        return pigpio version\n\
PRG u        get PWM range for gpio\n\
PROC t       validate and store script\n\
PROCD s      delete script\n\
PROCR s      run script\n\
PROCS s      stop script\n\
PRRG u       get PWM real range for gpio\n\
PRS u d      set PWM range for gpio\n\
PUD g p      set gpio pull up/down\n\
PWM u d      set PWM value for gpio\n\
R g          read gpio\n\
READ g       read gpio\n\
S u d        set servo value for gpio\n\
SERVO u d    set servo value for gpio\n\
T            return current tick\n\
TICK         return current tick\n\
TRIG u pl l  trigger level l for pl micros on gpio\n\
W g l        write level to gpio\n\
WDOG u d     set watchdog on gpio\n\
WRITE g l    write level to gpio\n\
WVAS u b t   wave add serial data\n\
WVBSY        check if wave busy\n\
WVCLR        wave clear\n\
WVGO         wave transmit\n\
WVGOR        wave transmit repeat\n\
WVHLT        wave stop\n\
WVSC ws      wave get cbs stats\n\
WVSM ws      wave get micros stats\n\
WVSP ws      wave get pulses stats\n\
\n\
b = baud rate\n\
d = decimal value\n\
g = gpio (0-53)\n\
h = handle (0-31)\n\
l = level (0-1)\n\
m = mode (RW540123)\n\
p = pud (ODU)\n\
pl = pulse length (0-100)\n\
s = script id\n\
t = text\n\
u = user gpio (0-31)\n\
x = hex value\n\
";

typedef struct
{
   int error;
   char * str;
} errInfo_t;

static errInfo_t errInfo[]=
{
   {PI_INIT_FAILED      , "pigpio initialisation failed"},
   {PI_BAD_USER_GPIO    , "gpio not 0-31"},
   {PI_BAD_GPIO         , "gpio not 0-53"},
   {PI_BAD_MODE         , "mode not 0-7"},
   {PI_BAD_LEVEL        , "level not 0-1"},
   {PI_BAD_PUD          , "pud not 0-2"},
   {PI_BAD_PULSEWIDTH   , "pulsewidth not 0 or 500-2500"},
   {PI_BAD_DUTYCYCLE    , "dutycycle outside set range"},
   {PI_BAD_TIMER        , "timer not 0-9"},
   {PI_BAD_MS           , "ms not 10-60000"},
   {PI_BAD_TIMETYPE     , "timetype not 0-1"},
   {PI_BAD_SECONDS      , "seconds < 0"},
   {PI_BAD_MICROS       , "micros not 0-999999"},
   {PI_TIMER_FAILED     , "gpioSetTimerFunc failed"},
   {PI_BAD_WDOG_TIMEOUT , "timeout not 0-60000"},
   {PI_NO_ALERT_FUNC    , "DEPRECATED"},
   {PI_BAD_CLK_PERIPH   , "clock peripheral not 0-1"},
   {PI_BAD_CLK_SOURCE   , "clock source not 0-1"},
   {PI_BAD_CLK_MICROS   , "clock micros not 1, 2, 4, 5, 8, or 10"},
   {PI_BAD_BUF_MILLIS   , "buf millis not 100-10000"},
   {PI_BAD_DUTYRANGE    , "dutycycle range not 25-40000"},
   {PI_BAD_SIGNUM       , "signum not 0-63"},
   {PI_BAD_PATHNAME     , "can't open pathname"},
   {PI_NO_HANDLE        , "no handle available"},
   {PI_BAD_HANDLE       , "unknown notify handle"},
   {PI_BAD_IF_FLAGS     , "ifFlags > 3"},
   {PI_BAD_CHANNEL      , "DMA channel not 0-14"},
   {PI_BAD_SOCKET_PORT  , "socket port not 1024-30000"},
   {PI_BAD_FIFO_COMMAND , "unknown fifo command"},
   {PI_BAD_SECO_CHANNEL , "DMA secondary channel not 0-6"},
   {PI_NOT_INITIALISED  , "function called before gpioInitialise"},
   {PI_INITIALISED      , "function called after gpioInitialise"},
   {PI_BAD_WAVE_MODE    , "waveform mode not 0-1"},
   {PI_BAD_CFG_INTERNAL , "bad parameter in gpioCfgInternals call"},
   {PI_BAD_WAVE_BAUD    , "baud rate not 100-250000"},
   {PI_TOO_MANY_PULSES  , "waveform has too many pulses"},
   {PI_TOO_MANY_CHARS   , "waveform has too many chars"},
   {PI_NOT_SERIAL_GPIO  , "no serial read in progress on gpio"},
   {PI_BAD_SERIAL_STRUC , "bad (null) serial structure parameter"},
   {PI_BAD_SERIAL_BUF   , "bad (null) serial buf parameter"}, 
   {PI_NOT_PERMITTED    , "no permission to update gpio"},
   {PI_SOME_PERMITTED   , "no permission to update one or more gpios"},
   {PI_BAD_WVSC_COMMND  , "bad WVSC subcommand"},
   {PI_BAD_WVSM_COMMND  , "bad WVSM subcommand"},
   {PI_BAD_WVSP_COMMND  , "bad WVSP subcommand"},
   {PI_BAD_PULSELEN     , "trigger pulse > 100 microseconds"},
   {PI_BAD_SCRIPT       , "invalid script"},
   {PI_BAD_SCRIPT_ID    , "unknown script id"},
   {PI_BAD_SER_OFFSET   , "add serial data offset > 30 minute"},
};

static char * fmtMdeStr="RW540123";
static char * fmtPudStr="ODU";

static int cmdMatch(char * str)
{
   int i;

   for (i=0; i<(sizeof(cmdInfo)/sizeof(cmdInfo_t)); i++)
   {
      if (strcasecmp(str, cmdInfo[i].name) == 0) return i;
   }
   return -1;
}

int cmdParse(char *buf, cmdCmd_t *cmd, int argc, char *argv[], gpioExtent_t *ext)
{
   char str[8];
   int f, valid, idx, val;
   char *ptr;
   char c, t;

   sscanf(buf, " %7s", str);

   cmd->cmd = -1;

   idx = cmdMatch(str);

   if (idx < 0) return idx;

   valid = 0;

   cmd->cmd = cmdInfo[idx].cmd;
   cmd->p1  = 0;
   cmd->p2  = 0;

   switch (cmdInfo[idx].vt)
   {
      case 1: /* BR1   BR2   HWVER NO    PIGPV TICK  WVBSY WVCLR WVGO WVGOR
                 WVHLT
              */
         f = sscanf(buf, " %7s %c", str, &t);
         if (f == 1) valid = 1;
         break;

      case 2: /* MODEG NC    NP    PFG   PRG   PROCD PROCR PROCS PRRG  READ
                 WVSC  WVSM  WVSP
              */
         f = sscanf(buf, " %7s %d %c", str, &cmd->p1, &t);
         if (f == 2) valid = 1;
         break;

      case 3: /* PFS   PRS   PWM   SERVO WDOG  WRITE
              */
         f = sscanf(buf, " %7s %d %d %c", str, &cmd->p1, &cmd->p2, &t);
         if (f == 3) valid = 1;
         break;

      case 4: /* NB
              */
         f = sscanf(buf, " %7s %d %x %c", str, &cmd->p1, &cmd->p2, &t);
         if (f == 3) valid = 1;
         break;

      case 5: /* TRIG
              */
         f = sscanf(buf, " %7s %d %d %d %c",
            str, &cmd->p1, &cmd->p2, &ext[0].data, &t);
         if (f == 4)
         {
            ext[0].n = sizeof(unsigned);
            ext[0].ptr = &ext[0].data;
            valid = 1;
         }
         break;

      case 6: /* HELP
              */
         valid = 1;
         break;

      case 7: /* BC1   BC2   BS1   BS2
              */
         f = sscanf(buf, " %7s %x %c", str, &cmd->p1, &t);
         if (f == 2) valid = 1;
         break;

      case 8: /* MODES
              */
         f = sscanf(buf, " %7s %d %c %c", str, &cmd->p1, &c, &t);
         if (f == 3)
         {
            val = toupper(c);
            ptr = strchr(fmtMdeStr, val);
            if (ptr != NULL)
            {
               val = ptr - fmtMdeStr;
               cmd->p2 = val;
               valid = 1;
            }
         }
         break;

      case 9: /* PUD
              */
         f = sscanf(buf, " %7s %d %c %c", str, &cmd->p1, &c, &t);
         if (f == 3)
         {
            val = toupper(c);
            ptr = strchr(fmtPudStr, val);
            if (ptr != NULL)
            {
               val = ptr - fmtPudStr;
               cmd->p2 = val;
               valid = 1;
            }
         }
         break;

      case 10: /* PROC
               */
         if (argc == 3)
         {
            cmd->p1 = strlen(argv[2]);
            ext[0].n = cmd->p1;
            ext[0].ptr = argv[2];
            valid = 1;
         }
         break;

      case 11: /* WVAS
               */
         if (argc == 6)
         {
            f = sscanf(buf, " %7s %d %d %d ",
               str, &cmd->p1, &ext[0].data, &ext[1].data);
            if (f == 4)
            {
               ext[0].n = sizeof(unsigned);
               ext[0].ptr = &ext[0].data;
               ext[1].n = sizeof(unsigned);
               ext[1].ptr = &ext[1].data;
               cmd->p2 = strlen(argv[5]);
               ext[2].n = cmd->p2;
               ext[2].ptr = argv[5];
               valid = 1;
            }
         }
         break;
   }

   if (valid) return idx;
   else       return -1;
}

char * cmdErrStr(int error)
{
   int i;

   for (i=0; i<(sizeof(errInfo)/sizeof(errInfo_t)); i++)
   {
      if (errInfo[i].error == error) return errInfo[i].str;
   }
   return "unknown error";
}
