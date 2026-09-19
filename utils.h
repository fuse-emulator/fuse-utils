/* utils.h: useful utility functions
   Copyright (c) 2002-2003 Philip Kendall

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_UTILS_UTILS_H
#define FUSE_UTILS_UTILS_H

#include "libspectrum.h"

int init_libspectrum( void );
int get_creator( libspectrum_creator **creator, const char *program );
int read_file( const char *filename, libspectrum_file *file );
int write_file( const char *filename, const void *buffer, size_t length );

#define MICRODRIVE_FILE_NAME_LENGTH 10
#define MICRODRIVE_FILE_HEADER_LENGTH 9

typedef struct {
  libspectrum_byte type;
  libspectrum_word length;
  libspectrum_word parameter1;
  libspectrum_word parameter2;
} microdrive_file_header;

typedef struct {
  libspectrum_byte name[ MICRODRIVE_FILE_NAME_LENGTH ];
  libspectrum_byte flags;
  size_t blocks;
  size_t stored_length;
  int complete;
  int bad_checksum;
  libspectrum_byte *data;
  size_t data_length;
} microdrive_file;

int decode_microdrive_file_header( microdrive_file_header *header,
                                   const libspectrum_byte *data,
                                   size_t length );
int get_microdrive_files( libspectrum_microdrive *microdrive,
                          microdrive_file **files, size_t *count );
void free_microdrive_files( microdrive_file *files, size_t count );

struct rzx_key {
  libspectrum_dword id;
  const char *description;
  libspectrum_rzx_dsa_key key;
};

extern struct rzx_key known_keys[];

#endif				/* #ifndef FUSE_UTILS_UTILS_H */
