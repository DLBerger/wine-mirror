/*
 * win16vdm - Win16 (NE) application loader stub
 *
 * Copyright 2026 Contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * STUB: This file contains only a placeholder main() function.
 * See documentation/rfc-win16-support.md for the full design.
 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win16vdm);

int __cdecl wmain( int argc, WCHAR *argv[] )
{
    FIXME( "win16vdm is not yet implemented\n" );
    FIXME( "See documentation/rfc-win16-support.md for the design RFC\n" );

    if (argc < 2)
    {
        fprintf( stderr, "Usage: win16vdm <ne-executable> [args...]\n" );
        return 1;
    }

    fprintf( stderr, "win16vdm: not yet supported\n" );
    return 1;
}
