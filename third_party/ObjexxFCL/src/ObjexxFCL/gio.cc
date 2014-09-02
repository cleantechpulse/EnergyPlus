// Global I/O Support
//
// Project: Objexx Fortran Compatibility Library (ObjexxFCL)
//
// Version: 4.0.0
//
// Language: C++
//
// Copyright (c) 2000-2014 Objexx Engineering, Inc. All Rights Reserved.
// Use of this source code or any derivative of it is restricted by license.
// Licensing is available from Objexx Engineering, Inc.:  http://objexx.com

// ObjexxFCL Headers
#include <ObjexxFCL/gio.hh>
#include <ObjexxFCL/Backspace.hh>
#include <ObjexxFCL/Fstring.hh>
#include <ObjexxFCL/GlobalStreams.hh>
#include <ObjexxFCL/IOFlags.hh>
#include <ObjexxFCL/Stream.hh>
#include <ObjexxFCL/stream.functions.hh>
#include <ObjexxFCL/string.functions.hh>

// C++ Headers
#include <cassert>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#define stat _stat
#endif

namespace ObjexxFCL {
namespace gio {

// Streams Collection
inline
static
GlobalStreams &
streams()
{
	static GlobalStreams streams_;
	return streams_;
}

// Unit /////

int
get_unit()
{
	return streams().next_unit(); // Negative Unit => failure
}

Name
def_name( Unit const unit )
{
	Stream const * const Stream_p( streams()[ unit ] );
	if ( Stream_p && Stream_p->is_open() && ( ! Stream_p->name().empty() ) ) { // As-is behavior is the default
		return Stream_p->name();
	} else {
		return Name( "FOR" + string_of( unit ) + ".DAT" ); // Intel Fortran style
	}
}

Name
def_name( Unit const unit, IOFlags const & flags )
{
	if ( flags.scratch() ) { // Scratch file name
		return Name( Stream::scratch_name() );
	} else if ( flags.asis() ) { // Same name if already connected
		Stream const * const p( streams()[ unit ] );
		if ( p && p->is_open() && ( ! p->name().empty() ) && p->asis_compatible( flags ) ) {
			return p->name();
		} else {
			return Name( "FOR" + string_of( unit ) + ".DAT" ); // Intel Fortran style
		}
	} else { // Default name for the unit
		return Name( "FOR" + string_of( unit ) + ".DAT" ); // Intel Fortran style
	}
}

// Open /////

// Open File on Specified Unit
bool
open( Unit const unit, Name const & name, IOFlags & flags )
{
	flags.clear_status();
	if ( flags.asis() ) {
		Stream * const p( streams()[ unit ] );
		if ( ( p != nullptr ) && p->is_open() && ( p->name() == name ) && p->asis_compatible( flags ) ) { // Case-sensitive name comparison used
			p->asis_update( flags ); // Update flags that an AsIs re-open can change
			return true; // Use existing connection
		}
	}
	if ( unit >= 0 ) {
		Stream * p( nullptr );
		if ( flags.rw() ) {
			p = streams().add( unit, name, flags );
		} else if ( flags.read() ) {
			p = streams().add_i( unit, name, flags );
		} else if ( flags.write() ) {
			p = streams().add_o( unit, name, flags );
		} else { // Default to read+write
			p = streams().add( unit, name, flags );
		}
		if ( p == nullptr ) {
			flags.err( true ).ios( 30 );
		} else if ( ! p->is_open() ) {
			flags.err( true ).ios( 600 );
		}
		return ( p != nullptr );
	} else {
		flags.err( true ).ios( 32 );
		return false;
	}
}

// Open File on Specified Unit
bool
open( Unit const unit, Fstring const & name, IOFlags & flags )
{
	return open( unit, std::string( name.trimmed() ), flags );
}

// Open File on Specified Unit
bool
open( Unit const unit, Name const & name, std::ios_base::openmode const mode )
{
	IOFlags flags( IOFlags::handler() );
	if ( mode & std::ios_base::in ) flags.read_on();
	if ( mode & std::ios_base::out ) flags.write_on();
	if ( flags.read() && ! flags.write() ) flags.old_on();
	if ( mode & std::ios_base::binary ) flags.binary_on();
	if ( mode & std::ios_base::app ) flags.append_on();
	return open( unit, name, flags );
}

// Open File on Specified Unit
bool
open( Unit const unit, Fstring const & name, std::ios_base::openmode const mode )
{
	return open( unit, std::string( name.trimmed() ), mode );
}

// Open File on Specified Unit
bool
open( Unit const unit, Name const & name )
{
	IOFlags flags( IOFlags::handler() );
	return open( unit, name, flags );
}

// Open File on Specified Unit
bool
open( Unit const unit, Fstring const & name )
{
	return open( unit, std::string( name.trimmed() ) );
}

// Open Default File on Specified Unit
bool
open( Unit const unit, IOFlags & flags )
{
	return open( unit, def_name( unit, flags ), flags );
}

// Open Default File on Specified Unit
bool
open( Unit const unit, std::ios_base::openmode const mode )
{
	return open( unit, def_name( unit ), mode );
}

// Open Default File on Specified Unit
bool
open( Unit const unit )
{
	return open( unit, def_name( unit ) );
}

// Open File and Return Unit
Unit
open( Name const & name, IOFlags & flags )
{
	Unit const unit( streams().next_unit() );
	open( unit, name, flags );
	return unit;
}

// Open File and Return Unit
Unit
open( Fstring const & name, IOFlags & flags )
{
	return open( std::string( name.trimmed() ), flags );
}

// Open File and Return Unit
Unit
open( Name const & name, std::ios_base::openmode const mode )
{
	Unit const unit( streams().next_unit() );
	open( unit, name, mode );
	return unit;
}

// Open File and Return Unit
Unit
open( Fstring const & name, std::ios_base::openmode const mode )
{
	return open( std::string( name.trimmed() ), mode );
}

// Open File and Return Unit
Unit
open( Name const & name )
{
	Unit const unit( streams().next_unit() );
	open( unit, name );
	return unit;
}

// Open File and Return Unit
Unit
open( Fstring const & name )
{
	return open( std::string( name.trimmed() ) );
}

// Open Default File and Return Unit
Unit
open( IOFlags & flags )
{
	Unit const unit( streams().next_unit() );
	open( unit, def_name( unit ), flags );
	return unit;
}

// Open Default File and Return Unit
Unit
open()
{
	Unit const unit( streams().next_unit() );
	open( unit, def_name( unit ) );
	return unit;
}

// Read /////

// Read from Unit
Read
read( Unit const unit, std::string const & fmt, IOFlags & flags )
{
	flags.clear_status();
	Stream * Stream_p( streams()[ unit ] );
	if ( ( ! ( Stream_p && Stream_p->is_open() ) ) && open( unit, flags ) ) Stream_p = streams()[ unit ]; // Opened a default file on the unit
	if ( Stream_p && Stream_p->is_open() ) {
		auto stream_p( dynamic_cast< std::istream * >( &Stream_p->stream() ) );
		if ( stream_p ) return Read( *stream_p, fmt, flags );
	}
	flags.err( true ).ios( 11 );
	return Read( flags );
}

// Read from Unit
Read
read( Unit const unit, gio::Fmt const & fmt, IOFlags & flags )
{
	flags.clear_status();
	Stream * Stream_p( streams()[ unit ] );
	if ( ( ! ( Stream_p && Stream_p->is_open() ) ) && open( unit, flags ) ) Stream_p = streams()[ unit ]; // Opened a default file on the unit
	if ( Stream_p && Stream_p->is_open() ) {
		auto stream_p( dynamic_cast< std::istream * >( &Stream_p->stream() ) );
		if ( stream_p ) return Read( *stream_p, fmt, flags );
	}
	flags.err( true ).ios( 11 );
	return Read( flags );
}

// Read from Unit
Read
read( Unit const unit, std::string const & fmt )
{
	Stream * Stream_p( streams()[ unit ] );
	if ( ( ! ( Stream_p && Stream_p->is_open() ) ) && open( unit ) ) Stream_p = streams()[ unit ]; // Opened a default file on the unit
	if ( Stream_p && Stream_p->is_open() ) {
		auto stream_p( dynamic_cast< std::istream * >( &Stream_p->stream() ) );
		if ( stream_p ) return Read( *stream_p, fmt );
	}
	return Read();
}

// Read from Unit
Read
read( Unit const unit, gio::Fmt const & fmt )
{
	Stream * Stream_p( streams()[ unit ] );
	if ( ( ! ( Stream_p && Stream_p->is_open() ) ) && open( unit ) ) Stream_p = streams()[ unit ]; // Opened a default file on the unit
	if ( Stream_p && Stream_p->is_open() ) {
		auto stream_p( dynamic_cast< std::istream * >( &Stream_p->stream() ) );
		if ( stream_p ) return Read( *stream_p, fmt );
	}
	return Read();
}

// Read from stdin
Read
read( std::string const & fmt, IOFlags & flags )
{
	return Read( std::cin, fmt, flags );
}

// Read from stdin
Read
read( gio::Fmt const & fmt, IOFlags & flags )
{
	return Read( std::cin, fmt, flags );
}

// Read from stdin
Read
read( std::string const & fmt )
{
	return Read( std::cin, fmt );
}

// Read from stdin
Read
read( gio::Fmt const & fmt )
{
	return Read( std::cin, fmt );
}

// Read Line from Unit
void
read_line( Unit const unit, IOFlags & flags, std::string & line )
{
	flags.clear_status();
	Stream * Stream_p( streams()[ unit ] );
	if ( ( ! ( Stream_p && Stream_p->is_open() ) ) && open( unit, flags ) ) Stream_p = streams()[ unit ]; // Opened a default file on the unit
	if ( Stream_p && Stream_p->is_open() ) {
		auto stream_p( dynamic_cast< std::istream * >( &Stream_p->stream() ) );
		if ( stream_p ) {
			cross_platform_get_line( *stream_p, line );
			flags.set_status( *stream_p );
		}
	} else {
		flags.err( true ).ios( 11 );
	}
}

// Read Line from Unit
void
read_line( Unit const unit, std::string & line )
{
	Stream * Stream_p( streams()[ unit ] );
	if ( ( ! ( Stream_p && Stream_p->is_open() ) ) && open( unit ) ) Stream_p = streams()[ unit ]; // Opened a default file on the unit
	if ( Stream_p && Stream_p->is_open() ) {
		auto stream_p( dynamic_cast< std::istream * >( &Stream_p->stream() ) );
		if ( stream_p ) cross_platform_get_line( *stream_p, line );
	}
}

// Write /////

// Write to Unit
Write
write( Unit const unit, std::string const & fmt, IOFlags & flags )
{
	flags.clear_status();
	Stream * Stream_p( streams()[ unit ] );
	if ( ( ! ( Stream_p && Stream_p->is_open() ) ) && open( unit, flags ) ) Stream_p = streams()[ unit ]; // Opened a default file on the unit
	if ( Stream_p && Stream_p->is_open() ) {
		auto stream_p( dynamic_cast< std::ostream * >( &Stream_p->stream() ) );
		if ( stream_p ) {
			flags.ter( Stream_p->ter() );
			return Write( *stream_p, fmt, flags );
		}
	}
	flags.err( true ).ios( 11 );
	return Write( flags );
}

// Write to Unit
Write
write( Unit const unit, gio::Fmt const & fmt, IOFlags & flags )
{
	flags.clear_status();
	Stream * Stream_p( streams()[ unit ] );
	if ( ( ! ( Stream_p && Stream_p->is_open() ) ) && open( unit, flags ) ) Stream_p = streams()[ unit ]; // Opened a default file on the unit
	if ( Stream_p && Stream_p->is_open() ) {
		auto stream_p( dynamic_cast< std::ostream * >( &Stream_p->stream() ) );
		if ( stream_p ) {
			flags.ter( Stream_p->ter() );
			return Write( *stream_p, fmt, flags );
		}
	}
	flags.err( true ).ios( 11 );
	return Write( flags );
}

// Write to Unit
Write
write( Unit const unit, std::string const & fmt )
{
	Stream * Stream_p( streams()[ unit ] );
	if ( ( ! ( Stream_p && Stream_p->is_open() ) ) && open( unit ) ) Stream_p = streams()[ unit ]; // Opened a default file on the unit
	if ( Stream_p && Stream_p->is_open() ) {
		auto stream_p( dynamic_cast< std::ostream * >( &Stream_p->stream() ) );
		if ( stream_p ) return Write( *stream_p, fmt, Stream_p->ter() );
	}
	return Write();
}

// Write to Unit
Write
write( Unit const unit, gio::Fmt const & fmt )
{
	Stream * Stream_p( streams()[ unit ] );
	if ( ( ! ( Stream_p && Stream_p->is_open() ) ) && open( unit ) ) Stream_p = streams()[ unit ]; // Opened a default file on the unit
	if ( Stream_p && Stream_p->is_open() ) {
		auto stream_p( dynamic_cast< std::ostream * >( &Stream_p->stream() ) );
		if ( stream_p ) return Write( *stream_p, fmt, Stream_p->ter() );
	}
	return Write();
}

// Write End-of-Line to Unit
void
write( Unit const unit )
{
	Stream * Stream_p( streams()[ unit ] );
	if ( ( ! ( Stream_p && Stream_p->is_open() ) ) && open( unit ) ) Stream_p = streams()[ unit ]; // Opened a default file on the unit
	if ( Stream_p && Stream_p->is_open() ) {
		auto stream_p( dynamic_cast< std::ostream * >( &Stream_p->stream() ) );
		if ( stream_p ) *stream_p << Stream_p->ter();
	}
}

// Write to stdout
Write
write( std::string const & fmt, IOFlags & flags )
{
	return Write( std::cout, fmt, flags, Write::LF );
}

// Write to stdout
Write
write( gio::Fmt const & fmt, IOFlags & flags )
{
	return Write( std::cout, fmt, flags, Write::LF );
}

// Write to stdout
Write
write( std::string const & fmt )
{
	return Write( std::cout, fmt, Write::LF );
}

// Write to stdout
Write
write( gio::Fmt const & fmt )
{
	return Write( std::cout, fmt, Write::LF );
}

// Print /////

// Print to stdout
Print
print( std::string const & fmt )
{
	return Print( fmt );
}

// Print to stdout
Print
print( Fmt const & fmt )
{
	return Print( fmt );
}

// Print to stdout with List-Directed Format
Print
print()
{
	return Print();
}

// Flush /////

// Flush
void
flush( Unit const unit )
{
	Stream * const Stream_p( streams()[ unit ] );
	if ( ( Stream_p ) && ( Stream_p->write() ) ) { // Writeable global stream
		if ( Stream_p->is_open() ) {
			auto ostream_p( dynamic_cast< std::ostream * >( &Stream_p->stream() ) );
			if ( ostream_p ) ostream_p->flush();
		}
	}
}

// Inquire /////

// Inquire by Unit
void
inquire( Unit const unit, IOFlags & flags )
{
	flags.clear();
	Stream * const Stream_p( streams()[ unit ] );
	if ( Stream_p ) { // Global stream
		flags.unit( unit );
		flags.name( Stream_p->name() );
		flags.exists( Stream_p->is_open() ? true : std::ifstream( Stream_p->name() ).good() );
		flags.open( Stream_p->is_open() );
		flags.read( Stream_p->read() );
		flags.write( Stream_p->write() );
		flags.binary( Stream_p->binary() );
		flags.append( Stream_p->append() );
		flags.asis( Stream_p->asis() );
		if ( Stream_p->is_open() ) {
			flags.size( Stream_p->size() );
			flags.pos( Stream_p->pos() );
		} else {
			struct stat fs;
			stat( Stream_p->name().c_str(), &fs );
			flags.size( fs.st_size );
		}
	} else { // Unit is not connected
		flags.unit( -1 );
	}
}

// Inquire by Name
void
inquire( Name const & name, IOFlags & flags )
{
	flags.clear();
	flags.name( name );
	Stream * const Stream_p( streams()[ name ] );
	if ( Stream_p ) { // Global stream
		flags.unit( streams().unit( name ) );
		flags.exists( Stream_p->is_open() ? true : std::ifstream( name ).good() );
		flags.open( Stream_p->is_open() );
		flags.read( Stream_p->read() );
		flags.write( Stream_p->write() );
		flags.binary( Stream_p->binary() );
		flags.append( Stream_p->append() );
		flags.asis( Stream_p->asis() );
		if ( Stream_p->is_open() ) {
			flags.size( Stream_p->size() );
			flags.pos( Stream_p->pos() );
		} else {
			struct stat fs;
			stat( name.c_str(), &fs );
			flags.size( fs.st_size );
		}
	} else { // Name is not an active global stream
		flags.unit( -1 );
		flags.exists( std::ifstream( name ).good() );
		flags.open( false ); // Can't tell if not a global stream
		struct stat fs;
		stat( name.c_str(), &fs );
		flags.size( fs.st_size );
	}
}

// Inquire by Name
void
inquire( Fstring const & name, IOFlags & flags )
{
	inquire( std::string( name.trimmed() ), flags );
}

// Inquire by Name
void
inquire( c_cstring const name, IOFlags & flags )
{
	inquire( std::string( name ), flags );
}

// Backspace /////

// Backspace
void
backspace( Unit const unit, IOFlags & flags )
{
	flags.clear_status();
	Stream * const Stream_p( streams()[ unit ] );
	if ( Stream_p ) { // Global stream
		if ( Stream_p->is_open() ) {
			auto istream_p( dynamic_cast< std::istream * >( &Stream_p->stream() ) );
			if ( istream_p ) Backspace( *istream_p, flags );
			auto ostream_p( dynamic_cast< std::ostream * >( &Stream_p->stream() ) );
			if ( ostream_p ) Backspace( *ostream_p, flags );
		} else {
			flags.err( true ).ios( 11 );
		}
	} else { // Unit is not connected
		flags.err( true ).ios( 606 );
	}
}

// Backspace
void
backspace( Unit const unit )
{
	IOFlags flags( IOFlags::handler() );
	backspace( unit, flags );
}

// Rewind /////

// Rewind
void
rewind( Unit const unit, IOFlags & flags )
{
	flags.clear_status();
	auto Stream_p( dynamic_cast< Stream * >( streams()[ unit ] ) );
	if ( Stream_p ) { // Global stream
		if ( Stream_p->is_open() ) {
			Stream_p->rewind( flags.truncate() );
		} else {
			flags.err( true ).ios( 11 );
		}
	} else { // Unit is not connected
		flags.err( true ).ios( 606 );
	}
}

// Rewind
void
rewind( Unit const unit )
{
	IOFlags flags( IOFlags::handler() );
	rewind( unit, flags );
}

// Rewind and Truncate
void
rewind_truncate( Unit const unit )
{
	IOFlags flags( IOFlags::handler() );
	flags.truncate_on();
	rewind( unit, flags );
}

// Close /////

// Close File
void
close( Unit const unit, IOFlags & flags )
{
	flags.clear_status();
	Stream * const p( streams()[ unit ] );
	if ( p ) {
		p->close();
		if ( flags.del() || flags.scratch() ) std::remove( p->name().c_str() );
		streams().del( unit );
	}
	if ( ! p ) {
		flags.err( true ).ios( 28 );
	}
}

// Close File
void
close( Unit const unit )
{
	IOFlags flags( IOFlags::handler() );
	close( unit, flags );
}

} // gio
} // ObjexxFCL
