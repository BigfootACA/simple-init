/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<termios.h>
#include"array.h"

static const struct{
        unsigned int speed;
        unsigned int number;
}speeds[]={
        #if defined(B0)
        {B0,        0     },
        #endif
        #if defined(B50)
        {B50,       50    },
        #endif
        #if defined(B75)
        {B75,       75    },
        #endif
        #if defined(B110)
        {B110,      110   },
        #endif
        #if defined(B134)
        {B134,      134   },
        #endif
        #if defined(B150)
        {B150,      150   },
        #endif
        #if defined(B200)
        {B200,      200   },
        #endif
        #if defined(B300)
        {B300,      300   },
        #endif
        #if defined(B600)
        {B600,      600   },
        #endif
        #if defined(B1200)
        {B1200,     1200  },
        #endif
        #if defined(B1800)
        {B1800,     1800  },
        #endif
        #if defined(B2400)
        {B2400,     2400  },
        #endif
        #if defined(B4800)
        {B4800,     4800  },
        #endif
        #if defined(B9600)
        {B9600,     9600  },
        #endif
        #if defined(B19200)
        {B19200,    19200  },
        #elif defined(EXTA)
        {EXTA,      19200  },
        #endif
        #if defined(B38400)
        {B38400,    38400  },
        #elif defined(EXTB)
        {EXTB,      38400  },
        #endif
        #if defined(B57600)
        {B57600,    57600  },
        #endif
        #if defined(B115200)
        {B115200,   115200 },
        #endif
        #if defined(B230400)
        {B230400,   230400 },
        #endif
        #if defined(B460800)
        {B460800,   460800 },
        #endif
        #if defined(B500000)
        {B500000,   500000 },
        #endif
        #if defined(B576000)
        {B576000,   576000 },
        #endif
        #if defined(B921600)
        {B921600,   921600 },
        #endif
        #if defined(B1000000)
        {B1000000,  1000000},
        #endif
        #if defined(B1152000)
        {B1152000,  1152000},
        #endif
        #if defined(B1500000)
        {B1500000,  1500000},
        #endif
        #if defined(B2000000)
        {B2000000,  2000000},
        #endif
        #if defined(B2500000)
        {B2500000,  2500000},
        #endif
        #if defined(B3000000)
        {B3000000,  3000000},
        #endif
        #if defined(B3500000)
        {B3500000,  3500000},
        #endif
        #if defined(B4000000)
        {B4000000,  4000000},
        #endif
};

int tty_speed_convert(int number){
        for(int i=0;i<(int)ARRLEN(speeds);i++)
                if(number==(int)speeds[i].number)
                        return (int)speeds[i].speed;
        return -1;
}
