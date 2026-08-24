/* tape2wav.c: Convert tape files (tzx, tap, etc.) to .wav files
   Copyright (c) 2007-2026 Fredrick Meunier
   Copyright (c) 2014-2015 Sergio Baldovi

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

   E-mail: fredm@spamcop.net

*/

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "libspectrum.h"

#include "compat.h"
#include "utils.h"

#define PROGRAM_NAME "tape2wav"
#define WAV_FORMAT_PCM 1
#define WAV_NUM_CHANNELS 1
#define WAV_BITS_PER_SAMPLE 8
#define WAV_FMT_CHUNK_SIZE 16
#define WAV_HEADER_SIZE 44

static void show_help( void );
static void show_version( void );
static int write_all( int fd, const void *buffer, size_t length,
                      const char *filename );
static int read_tape( char *filename, libspectrum_tape **tape );
static int write_tape( char *filename, libspectrum_tape *tape );

char *progname;
int sample_rate = 44100;

int
main( int argc, char **argv )
{
  int c, error = 0;
  libspectrum_tape *tzx;

  progname = argv[0];

  struct option long_options[] = {
    { "help", 0, NULL, 'h' },
    { "version", 0, NULL, 'V' },
    { 0, 0, 0, 0 }
  };

  while( ( c = getopt_long( argc, argv, "r:hV", long_options, NULL ) ) != -1 ) {

    switch( c ) {

    case 'r':
      sample_rate = abs( atoi( optarg ) );
      if( !sample_rate ) {
        fprintf( stderr, "%s: invalid sample rate `%s'\n", progname, optarg );
        error = 1;
      }
      break;
    case 'h': show_help(); return 0;
    case 'V': show_version(); return 0;

    case '?':
      /* getopt prints an error message to stderr */
      error = 1;
      break;

    default:
      error = 1;
      fprintf( stderr, "%s: unknown option `%c'\n", progname, (char) c );
      break;

    }

  }

  argc -= optind;
  argv += optind;

  if( error ) {
    fprintf( stderr, "Try `%s --help' for more information.\n", progname );
    return error;
  }

  if( argc < 2 ) {
    fprintf( stderr,
             "%s: usage: %s [-r rate] <infile> <outfile>\n",
             progname,
	     progname );
    fprintf( stderr, "Try `%s --help' for more information.\n", progname );
    return 1;
  }

  error = init_libspectrum(); if( error ) return error;

  if( read_tape( argv[0], &tzx ) ) return 1;

  if( write_tape( argv[1], tzx ) ) {
    libspectrum_tape_free( tzx );
    return 1;
  }

  libspectrum_tape_free( tzx );

  return 0;
}

static void
show_version( void )
{
  printf(
    PROGRAM_NAME " (" PACKAGE ") " PACKAGE_VERSION "\n"
    "Copyright (c) 2007 Fredrick Meunier\n"
    "License GPLv2+: GNU GPL version 2 or later "
    "<http://gnu.org/licenses/gpl.html>\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n" );
}

static void
show_help( void )
{
  printf(
    "Usage: %s [OPTION] <infile> <outfile>\n"
    "Converts ZX Spectrum tape images to audio files.\n"
    "\n"
    "Options:\n"
    "  -r <sample rate>  Set the sample rate for the target wav file. Defaults to\n"
    "                    44100 Hz.\n"
    "  -h, --help     Display this help and exit.\n"
    "  -V, --version  Output version information and exit.\n"
    "\n"
    "Report %s bugs to <%s>\n"
    "%s home page: <%s>\n"
    "For complete documentation, see the manual page of %s.\n",
    progname,
    PROGRAM_NAME, PACKAGE_BUGREPORT, PACKAGE_NAME, PACKAGE_URL, PROGRAM_NAME
  );
}

static int
read_tape( char *filename, libspectrum_tape **tape )
{
  libspectrum_byte *buffer; size_t length;

  if( read_file( filename, &buffer, &length ) ) return 1;

  *tape = libspectrum_tape_alloc();

  if( libspectrum_tape_read( *tape, buffer, length, LIBSPECTRUM_ID_UNKNOWN,
                             filename ) ) {
    free( buffer );
    return 1;
  }

  free( buffer );

  return 0;
}

static int
write_all( int fd, const void *buffer, size_t length, const char *filename )
{
  const libspectrum_byte *ptr = buffer;

  while( length ) {
    ssize_t written = write( fd, ptr, length );

    if( written < 0 ) {
      if( errno == EINTR ) continue;

      fprintf( stderr, "%s: error writing to '%s'\n", progname, filename );
      return 1;
    }

    if( !written ) {
      fprintf( stderr, "%s: error writing to '%s'\n", progname, filename );
      return 1;
    }

    ptr += written;
    length -= written;
  }

  return 0;
}

static int
write_tape( char *filename, libspectrum_tape *tape )
{
  libspectrum_buffer *header_buffer;
  libspectrum_buffer *sample_buffer;
  libspectrum_byte *header_data;
  libspectrum_byte *sample_data;
  size_t tape_length = 0;
  size_t terminal_pulse_length;
  libspectrum_byte terminal_level;
  libspectrum_error error;
  short level = 0; /* The last level output to this block */
  libspectrum_dword pulse_tstates = 0;
  libspectrum_dword balance_tstates = 0;
  int close_fd = 0, fd = -1, flags = 0, res = 1;
  uint32_t subchunk2size;
  uint32_t byte_rate;
  uint16_t block_align;

  unsigned int scale = 3500000/sample_rate;

  header_buffer = libspectrum_buffer_alloc();
  sample_buffer = libspectrum_buffer_alloc();
  header_data = NULL;
  sample_data = NULL;

  while( !(flags & LIBSPECTRUM_TAPE_FLAGS_TAPE) ) {
    libspectrum_dword pulse_length = 0;

    error = libspectrum_tape_get_next_edge( &pulse_tstates, &flags, tape );
    if( error != LIBSPECTRUM_ERROR_NONE ) {
      libspectrum_buffer_free( header_buffer );
      libspectrum_buffer_free( sample_buffer );
      return 1;
    }

    /* Invert the microphone state */
    if( pulse_tstates ||
        !( flags & LIBSPECTRUM_TAPE_FLAGS_NO_EDGE ) ||
        ( flags & ( LIBSPECTRUM_TAPE_FLAGS_STOP |
                    LIBSPECTRUM_TAPE_FLAGS_LEVEL_LOW |
                    LIBSPECTRUM_TAPE_FLAGS_LEVEL_HIGH ) ) ) {

      if( flags & LIBSPECTRUM_TAPE_FLAGS_NO_EDGE ) {
        /* Do nothing */
      } else if( flags & LIBSPECTRUM_TAPE_FLAGS_LEVEL_LOW ) {
        level = 0;
      } else if( flags & LIBSPECTRUM_TAPE_FLAGS_LEVEL_HIGH ) {
        level = 1;
      } else {
        level = !level;
      }

    }

    balance_tstates += pulse_tstates;

    if( flags & LIBSPECTRUM_TAPE_FLAGS_NO_EDGE ) continue;

    pulse_length = balance_tstates / scale;
    balance_tstates = balance_tstates % scale;

    /* TZXs produced by snap2tzx have very tight tolerances, err on the side of
       producing a pulse that is too long rather than too short */
    if( balance_tstates > scale>>1 ) {
      pulse_length++;
      balance_tstates = 0;
    }

    libspectrum_buffer_set( sample_buffer, level ? 0xff : 0x00,
                            pulse_length );
  }

  /* libspectrum signals end of tape with a zero-duration edge when there
     is no trailing pause. Render samples for that edge to finish the last
     pulse with an opposite level for at least 1ms before returning low. */
  tape_length = libspectrum_buffer_get_data_size( sample_buffer );
  sample_data = libspectrum_buffer_get_data( sample_buffer );
  if( tape_length && !pulse_tstates ) {
    terminal_pulse_length = ( sample_rate + 999 ) / 1000;
    terminal_level = sample_data[ tape_length - 1 ] ? 0x00 : 0xff;
    libspectrum_buffer_set( sample_buffer, terminal_level,
                            terminal_pulse_length );
    if( terminal_level ) libspectrum_buffer_set( sample_buffer, 0x00, 1 );
  }

  tape_length = libspectrum_buffer_get_data_size( sample_buffer );
  sample_data = libspectrum_buffer_get_data( sample_buffer );
  subchunk2size = tape_length;
  byte_rate = sample_rate * WAV_NUM_CHANNELS * WAV_BITS_PER_SAMPLE / 8;
  block_align = WAV_NUM_CHANNELS * WAV_BITS_PER_SAMPLE / 8;

  libspectrum_buffer_write( header_buffer, "RIFF", 4 );
  libspectrum_buffer_write_dword( header_buffer,
                                  WAV_HEADER_SIZE - 8 + subchunk2size );
  libspectrum_buffer_write( header_buffer, "WAVE", 4 );
  libspectrum_buffer_write( header_buffer, "fmt ", 4 );
  libspectrum_buffer_write_dword( header_buffer, WAV_FMT_CHUNK_SIZE );
  libspectrum_buffer_write_word( header_buffer, WAV_FORMAT_PCM );
  libspectrum_buffer_write_word( header_buffer, WAV_NUM_CHANNELS );
  libspectrum_buffer_write_dword( header_buffer, sample_rate );
  libspectrum_buffer_write_dword( header_buffer, byte_rate );
  libspectrum_buffer_write_word( header_buffer, block_align );
  libspectrum_buffer_write_word( header_buffer, WAV_BITS_PER_SAMPLE );
  libspectrum_buffer_write( header_buffer, "data", 4 );
  libspectrum_buffer_write_dword( header_buffer, subchunk2size );

  header_data = libspectrum_buffer_get_data( header_buffer );

  if( strncmp( filename, "-", 1 ) == 0 ) {
    fd = fileno( stdout );
    if( isatty( fd ) ) {
      fprintf( stderr, "%s: won't output binary data to a terminal\n",
               progname );
      goto out;
    }
  } else {
    fd = open( filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666 );
    if( fd == -1 ) {
      fprintf( stderr, "%s: unable to open file '%s' for writing\n", progname,
               filename );
      goto out;
    }
    close_fd = 1;
  }

#ifdef WIN32
  setmode( fd, O_BINARY );
#endif                          /* #ifdef WIN32 */

  if( write_all( fd, header_data,
                 libspectrum_buffer_get_data_size( header_buffer ), filename ) ) {
    goto out;
  }

  if( write_all( fd, sample_data, subchunk2size, filename ) ) {
    goto out;
  }

  res = 0;

out:

  if( close_fd && close( fd ) != 0 ) {
    fprintf( stderr, "%s: error closing '%s'\n", progname, filename );
  }

  libspectrum_buffer_free( header_buffer );
  libspectrum_buffer_free( sample_buffer );

  return res;
}
