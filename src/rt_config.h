/*
 *  rt_config.h - runtime configuration module - header
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2002 Mauro Tortonesi <mauro _at_ deepspace6.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */  
#ifndef RT_CONFIG_H
#define RT_CONFIG_H

#include "misc.h"

#ifdef HAVE_GETADDRINFO_AI_ADDRCONFIG
#define getaddinfo_supports_ai_addrconfig() getaddinfo_supports_flag(AI_ADDRCONFIG)
#else
#define getaddinfo_supports_ai_addrconfig() (FALSE)
#endif

#ifdef HAVE_GETADDRINFO_AI_ALL
#define getaddinfo_supports_ai_all() getaddinfo_supports_flag(AI_ALL)
#else
#define getaddinfo_supports_ai_all() (FALSE)
#endif

#ifdef HAVE_GETADDRINFO_AI_V4MAPPED
#define getaddinfo_supports_ai_v4mapped() getaddinfo_supports_flag(AI_V4MAPPED)
#else
#define getaddinfo_supports_ai_v4mapped() (FALSE)
#endif

#ifdef ENABLE_IPV6 
bool is_ipv6_enabled(void);
#else
#define is_ipv6_enabled() (FALSE)
#endif

bool getaddinfo_supports_flag(int flag);
bool is_getaddinfo_sane(void);
bool is_double_binding_sane(void);


#endif /* RT_CONFIG_H */
