/*
 *  parser.h - argument parser & dispatcher module - header 
 * 
 *  nc6 - an advanced netcat clone
 *  Copyright (C) 2001-2002 Mauro Tortonesi <mauro _at_ ferrara.linux.it>
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
#ifndef PARSER_H
#define PARSER_H

#include "misc.h"

#define NUMERIC_MODE		0x0001
#define USE_UDP			0x0002
#define STRICT_IPV6		0x0004
#define REUSE_ADDR      	0x0008
#define FILE_TRANSFER_MODE	0x0010

bool is_flag_set(unsigned long mask);
void parse_arguments(int argc, char **argv);

#endif /* PARSER_H */
