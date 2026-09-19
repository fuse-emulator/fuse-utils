/* utils.c: useful utility functions
   Copyright (c) 2002-2007 Philip Kendall

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

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef WIN32
#include <sys/utsname.h>
#endif

#include "libspectrum.h"

#include "compat.h"
#include "utils.h"

extern char *progname;

/* The minimum version of libspectrum we need */
static const char *LIBSPECTRUM_MIN_VERSION = "0.2.0";

int
init_libspectrum( void )
{
  if( libspectrum_check_version( LIBSPECTRUM_MIN_VERSION ) ) {
    if( libspectrum_init() ) return 1;
  } else {
    fprintf( stderr, "libspectrum version %s found, but %s required",
	     libspectrum_version(), LIBSPECTRUM_MIN_VERSION );
    return 1;
  }

  return 0;
}

int
decode_microdrive_file_header( microdrive_file_header *header,
                               const libspectrum_byte *data, size_t length )
{
  if( !header || !data || length < MICRODRIVE_FILE_HEADER_LENGTH ) return 1;

  header->type = data[0];
  header->length = data[1] | ( data[2] << 8 );
  header->parameter1 = data[3] | ( data[4] << 8 );
  header->parameter2 = data[5] | ( data[6] << 8 );
  return 0;
}

void
free_microdrive_files( microdrive_file *files, size_t count )
{
  size_t i;

  if( !files ) return;
  for( i = 0; i < count; i++ ) free( files[i].data );
  free( files );
}

int
get_microdrive_files( libspectrum_microdrive *microdrive,
                      microdrive_file **files, size_t *file_count )
{
  size_t block_count, i, j;

  if( !microdrive || !files || !file_count ) return 1;
  *files = NULL;
  *file_count = 0;
  block_count = libspectrum_microdrive_block_count( microdrive );

  *files = calloc( block_count, sizeof( **files ) );
  if( block_count && !*files ) return 1;

  for( i = 0; i < block_count; i++ ) {
    const libspectrum_byte *name;
    libspectrum_word length;
    libspectrum_byte flags, record;
    size_t end;

    flags = libspectrum_microdrive_block_record_flags( microdrive, i );
    length = libspectrum_microdrive_block_record_length( microdrive, i );
    if( !length || length > LIBSPECTRUM_MICRODRIVE_DATA_LEN ) continue;

    name = libspectrum_microdrive_block_record_name( microdrive, i );
    for( j = 0; j < *file_count; j++ )
      if( !memcmp( (*files)[j].name, name,
                   MICRODRIVE_FILE_NAME_LENGTH ) ) break;

    if( j == *file_count ) {
      memcpy( (*files)[j].name, name, MICRODRIVE_FILE_NAME_LENGTH );
      (*files)[j].flags = flags;
      (*file_count)++;
    }

    (*files)[j].blocks++;
    (*files)[j].stored_length += length;
    if( flags & LIBSPECTRUM_MICRODRIVE_RECORD_EOF )
      (*files)[j].complete = 1;
    if( libspectrum_microdrive_checksum( microdrive, i ) )
      (*files)[j].bad_checksum = 1;

    record = libspectrum_microdrive_block_record_number( microdrive, i );
    end = (size_t)record * LIBSPECTRUM_MICRODRIVE_DATA_LEN + length;
    if( end > (*files)[j].data_length ) (*files)[j].data_length = end;
  }

  for( i = 0; i < *file_count; i++ ) {
    if( !(*files)[i].data_length ) continue;
    (*files)[i].data = calloc( (*files)[i].data_length, 1 );
    if( !(*files)[i].data ) {
      free_microdrive_files( *files, *file_count );
      *files = NULL;
      *file_count = 0;
      return 1;
    }
  }

  for( i = 0; i < block_count; i++ ) {
    const libspectrum_byte *name;
    libspectrum_word length;
    size_t offset;

    length = libspectrum_microdrive_block_record_length( microdrive, i );
    if( !length || length > LIBSPECTRUM_MICRODRIVE_DATA_LEN ) continue;
    name = libspectrum_microdrive_block_record_name( microdrive, i );
    for( j = 0; j < *file_count; j++ )
      if( !memcmp( (*files)[j].name, name,
                   MICRODRIVE_FILE_NAME_LENGTH ) ) break;
    if( j == *file_count ) continue;

    offset = (size_t)libspectrum_microdrive_block_record_number(
               microdrive, i ) * LIBSPECTRUM_MICRODRIVE_DATA_LEN;
    memcpy( (*files)[j].data + offset,
            libspectrum_microdrive_block_data( microdrive, i ), length );
  }

  return 0;
}

int
get_creator( libspectrum_creator **creator, const char *program )
{
  char *custom;
  unsigned int version[4] = { 0, 0, 0, 0 };
  libspectrum_error error;
  size_t i;
  static const size_t CUSTOM_SIZE = 256;

#ifndef WIN32
  char osname[ 256 ];
  int sys_error;

  sys_error = compat_osname( osname, sizeof( osname ) );
  if( sys_error ) return 1;
#endif

  *creator = libspectrum_creator_alloc();

  error = libspectrum_creator_set_program( *creator, program );
  if( error ) { libspectrum_creator_free( *creator ); return error; }

  sscanf( VERSION, "%u.%u.%u.%u",
	  &version[0], &version[1], &version[2], &version[3] );
  for( i=0; i<4; i++ ) if( version[i] > 0xff ) version[i] = 0xff;

  error = libspectrum_creator_set_major( *creator,
					 version[0] * 0x100 + version[1] );
  if( error ) { libspectrum_creator_free( *creator ); return error; }

  error = libspectrum_creator_set_minor( *creator,
					 version[2] * 0x100 + version[3] );
  if( error ) { libspectrum_creator_free( *creator ); return error; }

  custom = libspectrum_new( char, CUSTOM_SIZE );

#ifdef WIN32
  snprintf( custom, CUSTOM_SIZE, "libspectrum: %s\nsystem: windows\n",
	    libspectrum_version() );
#else
  snprintf( custom, CUSTOM_SIZE, "libspectrum: %s\nuname: %s\n",
	    libspectrum_version(),
	    osname );
#endif

  error = libspectrum_creator_set_custom( *creator,
					  (libspectrum_byte*)custom,
					  strlen( custom ) );
  if( error ) {
    libspectrum_free( custom ); libspectrum_creator_free( *creator );
    return error;
  }

  return 0;
}

int
read_file( const char *filename, libspectrum_file *file )
{
  libspectrum_error error;

  libspectrum_file_init( file );
  error = libspectrum_file_open( file, filename );
  if( error ) {
    fprintf( stderr, "%s: couldn't read `%s' (libspectrum error %d)\n",
             progname, filename, error );
    return error;
  }

  return 0;
}

int
write_file( const char *filename, const void *buffer, size_t length )
{
  FILE *f;
  size_t bytes;

  f = fopen( filename, "wb" );
  if( !f ) {
    fprintf( stderr, "%s: couldn't open `%s': %s\n", progname, filename,
             strerror( errno ) );
    return 1;
  }

  bytes = fwrite( buffer, 1, length, f );
  if( bytes != length ) {
    fprintf( stderr, "%s: wrote only %lu of %lu bytes to `%s'\n", progname,
            (unsigned long)bytes, (unsigned long)length, filename );
    fclose( f );
    return 1;
  }

  if( fclose( f ) ) {
    fprintf( stderr, "%s: error closing `%s': %s\n", progname, filename,
             strerror( errno ) );
    return 1;
  }

  return 0;
}
