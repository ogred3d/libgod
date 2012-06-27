#include "common.h"
#include <boost/shared_ptr.hpp>

namespace asn1
{
#include "God.h"
#include "der_encoder.h"
#include "xer_encoder.h"
};

#include "libgod.h"
#include "godasn1.h"
#include "goderror.h"

namespace libgod
{
	static int write_out(const void *buffer, size_t size, void *app_key)
	{
		std::ofstream *stream = reinterpret_cast<std::ofstream*>( app_key );
		if (!stream->write( reinterpret_cast<const char*>(buffer), size ))
			return -1;
		return 0;
	}
	
	GodASN1::GodASN1()
		: m_root( new asn1::God_t )
	{
		memset( m_root.get(), 0, sizeof(asn1::God_t) );
	}

	GodASN1::GodASN1 (const std::string& file)
		: m_root( new asn1::God_t )
	{
		memset( m_root.get(), 0, sizeof(asn1::God_t) );
		readFromFile( file );
	}

	GodASN1::GodASN1 (const GodASN1& rhs)
	{
		*this = rhs;
	}

	GodASN1::~GodASN1()
	{
	}

	GodASN1& GodASN1::operator= (const GodASN1& rhs)
	{
		if (this == &rhs)
			return *this;

		m_root.reset( new asn1::God_t );
		memcpy( m_root.get(), rhs.m_root.get(), sizeof(asn1::God_t) );
		return *this;
	}

	void GodASN1::writeToFile (const std::string& file)
	{
		asn1::asn_enc_rval_t status;
		std::ofstream ofs;
		ofs.exceptions ( std::ios_base::failbit | std::ios_base::badbit );
		try
		{
			ofs.open( file.c_str(), std::ios_base::binary );
			if (!ofs)
				throw GodError( "Can't open file " + file );
			asn1::God_t* raw = m_root.get();
			status = der_encode( &asn1::asn_DEF_God, raw, write_out, &ofs );
			if (status.encoded == -1)
				throw GodError( "Can't encode data to ASN1 stream" );
		}
		catch (std::exception& e)
		{
			if (ofs.is_open())
				ofs.close();
			// lose an exception
			if (unlink( file.c_str() ))
				throw GodError( "Encoding fails, also can't remove a file " + file );
			throw;
		}
	}

	void GodASN1::readFromFile (const std::string& file)
	{
		asn1::asn_dec_rval_t status;
		std::ifstream ifs;
		ifs.exceptions ( std::ios_base::failbit | std::ios_base::badbit );
		try
		{
			GodPtr temporary( new asn1::God_t );
			memset( temporary.get(), 0, sizeof(asn1::God_t) );

			ifs.open( file.c_str(), std::ios_base::binary );
			if (!ifs)
				throw GodError( "Can't open file " + file );

			if (!ifs.seekg( 0, std::ios_base::end ))
				throw GodError( "Error seeking to end of file " + file  );
			
			std::vector<char> buf( ifs.tellg() );
			if (!ifs.seekg( 0, std::ios_base::beg ))
				throw GodError( "Error seeking to begin of file " + file );
			ifs.read( (char*)&buf[0], buf.size() );

			asn1::God_t* raw = temporary.get();
			status = ber_decode( NULL, &asn1::asn_DEF_God, (void**)&raw, 
					&buf[0], buf.size() );
			if (status.code == asn1::RC_OK)
			{
				temporary.swap( m_root );
			}
			else
			{
				asn1::asn_DEF_God.free_struct( &asn1::asn_DEF_God, raw, 0 );
				throw GodError( "Can't encode data to ASN1 stream" );
			}
		}
		catch (std::exception& e)
		{
			if (ifs.is_open())
				ifs.close();
			throw;
		}
	}
	
	std::ostream& operator<< (std::ostream& os, const GodASN1& rhs)
	{
		asn1::asn_enc_rval_t status;
		asn1::God_t* raw = rhs.m_root.get();
		xer_encode( &asn1::asn_DEF_God, raw, asn1::XER_F_BASIC, write_out, &os );
		if (status.encoded == -1)
			os << "ERROR XER ENCODING";
		return os;
	}
	
	GodASN1::GodPtr GodASN1::getRoot()
	{
		return m_root;
	}
	
};

