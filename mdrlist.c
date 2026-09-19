/* mdrlist.c: Produce a listing of the blocks in a .mdr file
   Copyright (c) 2026 Fredrick Meunier

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "config.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libspectrum.h"
#include "utils.h"

#define PROGRAM_NAME "mdrlist"

const char *progname;

static void
print_name( const libspectrum_byte *name )
{
  char utf8[ MICRODRIVE_FILE_NAME_LENGTH * 9 + 1 ];

  if( name &&
      !libspectrum_zx_string_to_utf8( utf8, sizeof( utf8 ), name,
                                      MICRODRIVE_FILE_NAME_LENGTH ) )
    printf( "%s", utf8 );
}

static const char *
checksum_description( int status )
{
  switch( status ) {
  case 0: return "PASS";
  case -1: return "preset bad block";
  case 1: return "header checksum failed";
  case 2: return "record descriptor checksum failed";
  case 3: return "data checksum failed";
  default: return "unknown error";
  }
}

static void
list_files( libspectrum_microdrive *microdrive )
{
  microdrive_file *files;
  size_t i, file_count;

  if( get_microdrive_files( microdrive, &files, &file_count ) ) return;

  printf( "Files: %lu\n\n", (unsigned long)file_count );
  for( i = 0; i < file_count; i++ ) {
    microdrive_file_header header;
    int has_header =
      ( files[i].flags & LIBSPECTRUM_MICRODRIVE_RECORD_NON_PRINT ) &&
      !decode_microdrive_file_header( &header, files[i].data,
                                     files[i].data_length );

    printf( "  " );
    if( !( files[i].flags & LIBSPECTRUM_MICRODRIVE_RECORD_NON_PRINT ) ) {
      printf( "PRINT file: \"" );
      print_name( files[i].name );
      printf( "\"\n  Length: %lu bytes",
              (unsigned long)files[i].stored_length );
    } else if( has_header ) {
      switch( header.type ) {
      case 0:
        printf( "Program: \"" );
        print_name( files[i].name );
        printf( "\"" );
        if( header.parameter1 < 10000 )
          printf( " LINE %u", header.parameter1 );
        printf( "\n  Length: %u, includes variable length: %u",
                header.length, header.length - header.parameter2 );
        break;
      case 1:
        printf( "Number Array: \"" );
        print_name( files[i].name );
        printf( "\" DATA %c()\n  Length: %u",
                (char)( header.parameter1 & 63 ) + 64, header.length );
        break;
      case 2:
        printf( "Character Array: \"" );
        print_name( files[i].name );
        printf( "\" DATA %c$()\n  Length: %u",
                (char)( header.parameter1 & 63 ) + 64, header.length );
        break;
      case 3:
        printf( "Bytes: \"" );
        print_name( files[i].name );
        printf( "\" %s %u, %u",
                header.parameter1 == 16384 && header.length == 6912 ?
                  "SCREEN$" : "CODE",
                header.parameter1, header.length );
        break;
      default:
        printf( "Unknown type %u: \"", header.type );
        print_name( files[i].name );
        printf( "\"\n  Length: %u", header.length );
        break;
      }
    } else {
      printf( "Non-PRINT file: \"" );
      print_name( files[i].name );
      printf( "\"\n  Length: %lu bytes",
              (unsigned long)files[i].stored_length );
    }
    printf( " in %lu block%s\n", (unsigned long)files[i].blocks,
            files[i].blocks == 1 ? "" : "s" );
    printf( "  Status: %s%s\n\n",
            files[i].complete ? "complete" : "missing EOF",
            files[i].bad_checksum ? ", checksum error" : "" );
  }

  free_microdrive_files( files, file_count );
}

static void
list_blocks( libspectrum_microdrive *microdrive, size_t count )
{
  size_t i;

  for( i = 0; i < count; i++ ) {
    libspectrum_byte header_flags =
      libspectrum_microdrive_block_header_flag( microdrive, i );
    libspectrum_byte record_flags =
      libspectrum_microdrive_block_record_flags( microdrive, i );
    libspectrum_word length =
      libspectrum_microdrive_block_record_length( microdrive, i );
    int checksum = libspectrum_microdrive_checksum( microdrive, i );

    printf( "--= Block #%lu =--\n", (unsigned long)i );
    printf( "  Header flag: 0x%02x (%s)\n", header_flags,
            header_flags & LIBSPECTRUM_MICRODRIVE_HEADER_BLOCK ?
              "header block" : "not a header block" );
    printf( "  Block number: %u\n",
            libspectrum_microdrive_block_number( microdrive, i ) );
    printf( "  Header unused: 0x%04x\n",
            libspectrum_microdrive_block_header_unused( microdrive, i ) );
    printf( "  Header checksum: 0x%02x\n",
            libspectrum_microdrive_block_header_checksum( microdrive, i ) );

    if( !( record_flags & LIBSPECTRUM_MICRODRIVE_RECORD_EOF ) && !length ) {
      printf( "  Record: empty\n" );
    } else if( length > LIBSPECTRUM_MICRODRIVE_DATA_LEN ) {
      printf( "  Record: invalid (flags 0x%02x, number %u, length %u)\n",
              record_flags,
              libspectrum_microdrive_block_record_number( microdrive, i ),
              length );
    } else {
      printf( "  Record flags: 0x%02x (%s, %s)\n", record_flags,
              record_flags & LIBSPECTRUM_MICRODRIVE_RECORD_EOF ?
                "EOF" : "continued",
              record_flags & LIBSPECTRUM_MICRODRIVE_RECORD_NON_PRINT ?
                "non-PRINT" : "PRINT" );
      printf( "  Record number: %u\n",
              libspectrum_microdrive_block_record_number( microdrive, i ) );
      printf( "  Record length: %u bytes\n", length );
      printf( "  Record name: \"" );
      print_name( libspectrum_microdrive_block_record_name( microdrive, i ) );
      printf( "\"\n" );
      printf( "  Record checksum: 0x%02x\n",
              libspectrum_microdrive_block_record_checksum( microdrive, i ) );
      printf( "  Data checksum: 0x%02x\n",
              libspectrum_microdrive_block_data_checksum( microdrive, i ) );
    }
    printf( "  Checksums: %s\n\n", checksum_description( checksum ) );
  }
}

static int
process_microdrive( const char *filename, int show_blocks )
{
  libspectrum_file file;
  libspectrum_microdrive *microdrive;
  libspectrum_error error;
  size_t i, count, free_blocks = 0;

  if( read_file( filename, &file ) ) return 1;

  microdrive = libspectrum_microdrive_alloc();
  if( !microdrive ) {
    libspectrum_file_clear( &file );
    return 1;
  }

  error = libspectrum_microdrive_mdr_read( microdrive, file.buffer,
                                           file.length );
  libspectrum_file_clear( &file );
  if( error != LIBSPECTRUM_ERROR_NONE ) {
    libspectrum_microdrive_free( microdrive );
    return error;
  }

  count = libspectrum_microdrive_block_count( microdrive );
  printf( "\n\nListing of `%s':\n\n", filename );
  printf( "Cartridge name: \"" );
  for( i = 0; i < count; i++ ) {
    if( ( libspectrum_microdrive_block_header_flag( microdrive, i ) &
          LIBSPECTRUM_MICRODRIVE_HEADER_BLOCK ) &&
        libspectrum_microdrive_checksum( microdrive, i ) != -1 &&
        libspectrum_microdrive_checksum( microdrive, i ) != 1 ) {
      print_name(
        libspectrum_microdrive_block_cartridge_name( microdrive, i ) );
      break;
    }
  }
  printf( "\"\n" );
  for( i = 0; i < count; i++ ) {
    libspectrum_byte flags =
      libspectrum_microdrive_block_record_flags( microdrive, i );
    libspectrum_word length =
      libspectrum_microdrive_block_record_length( microdrive, i );

    if( !( flags & LIBSPECTRUM_MICRODRIVE_RECORD_EOF ) && !length )
      free_blocks++;
  }

  printf( "Cartridge blocks: %lu\n", (unsigned long)count );
  printf( "Free space: %lu bytes in %lu block%s\n",
          (unsigned long)( free_blocks * LIBSPECTRUM_MICRODRIVE_DATA_LEN ),
          (unsigned long)free_blocks, free_blocks == 1 ? "" : "s" );
  printf( "Write protected: %s\n",
          libspectrum_microdrive_write_protect( microdrive ) ? "yes" : "no" );

  if( show_blocks ) {
    printf( "\n" );
    list_blocks( microdrive, count );
  } else {
    list_files( microdrive );
  }

  error = libspectrum_microdrive_free( microdrive );
  return error != LIBSPECTRUM_ERROR_NONE;
}

static void
show_version( void )
{
  printf( PROGRAM_NAME " (" PACKAGE ") " PACKAGE_VERSION "\n"
          "Copyright (c) 2026 Fredrick Meunier\n"
          "License GPLv2+: GNU GPL version 2 or later "
          "<http://gnu.org/licenses/gpl.html>\n"
          "This is free software: you are free to change and redistribute it.\n"
          "There is NO WARRANTY, to the extent permitted by law.\n" );
}

static void
show_help( void )
{
  printf( "Usage: %s [OPTION] <microdrive file>...\n"
          "Outputs a description of the contents of MDR microdrive files.\n\n"
          "Options:\n"
          "  -b, --blocks   List individual cartridge blocks instead of files.\n"
          "  -h, --help     Display this help and exit.\n"
          "  -V, --version  Output version information and exit.\n\n"
          "Report %s bugs to <%s>\n"
          "%s home page: <%s>\n"
          "For complete documentation, see the manual page of %s.\n",
          progname, PROGRAM_NAME, PACKAGE_BUGREPORT, PACKAGE_NAME, PACKAGE_URL,
          PROGRAM_NAME );
}

int
main( int argc, char **argv )
{
  int c, error = 0, ret = 0, i, show_blocks = 0;
  struct option long_options[] = {
    { "blocks", 0, NULL, 'b' },
    { "help", 0, NULL, 'h' },
    { "version", 0, NULL, 'V' },
    { 0, 0, 0, 0 }
  };

  progname = argv[0];
  while( ( c = getopt_long( argc, argv, "bhV", long_options, NULL ) ) != -1 ) {
    switch( c ) {
    case 'b': show_blocks = 1; break;
    case 'h': show_help(); return 0;
    case 'V': show_version(); return 0;
    case '?': error = 1; break;
    default: error = 1; break;
    }
  }

  if( error ) {
    fprintf( stderr, "Try `%s --help' for more information.\n", progname );
    return error;
  }
  if( optind == argc ) {
    fprintf( stderr, "%s: usage: %s [OPTION] <microdrive files>...\n",
             progname, progname );
    fprintf( stderr, "Try `%s --help' for more information.\n", progname );
    return 1;
  }

  error = init_libspectrum();
  if( error ) return error;

  for( i = optind; i < argc; i++ )
    ret |= process_microdrive( argv[i], show_blocks );
  return ret;
}
