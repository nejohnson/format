/* ****************************************************************************
 * Copyright (C) 2010, Neil Johnson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms,
 * with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ************************************************************************* */

#include <stdio.h>
#include "format.h"



struct coord {
	short x, y;
};

void lcd_putc( short x, short y, char c )
{
	printf( "(%2d,%2d) \"%c\"\n", x, y, c );
}


void * lcd_putat( void * ap, const char *s, size_t n )
{
	struct coord *pc = (struct coord *)ap;
	
	while ( n-- )
	{
		lcd_putc( pc->x++, pc->y, *s++ );
		if ( pc->x >= 80 )
		{
			pc->x = 0;
			pc->y++;
		}
	}
	
	return (void *)pc;
}


int lcd_printf( struct coord loc, const char *fmt, ... )
{
    va_list arg;
    int done;
    
    va_start ( arg, fmt );
    done = format( lcd_putat, &loc, fmt, arg );
    va_end ( arg );
    
    return done;
}



    

int main( void )
{
    struct coord loc;
    int temperature;
    int status;
    
    temperature = 32;
    loc.x = 5;
    loc.y = 2;
    status = lcd_printf( loc, "Boiler temp = %+d Celsius", temperature );
    if ( status < 0 )
    {
    	/* error handler */
    }
    
    return 0;
}
