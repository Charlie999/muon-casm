#ifndef CASM_int24_H
#define CASM_int24_H

const int int24_MAX = 8388607;

class int24
{
public:
    unsigned char m_Internal[3];

    int24()
    {
    }

    int24( const int val )
    {
        *this   = val;
    }

    int24( const int24& val )
    {
        *this   = val;
    }

    int24( const unsigned char a, const unsigned char b, const unsigned char c) {
        *this = int24(0);
        this->m_Internal[2] = a;
        this->m_Internal[1] = b;
        this->m_Internal[0] = c;
    }

    operator int() const
    {
        if ( m_Internal[2] & 0x80 ) // Is this a negative?  Then we need to sign extend.
        {
            return (0xff << 24) | (m_Internal[2] << 16) | (m_Internal[1] << 8) | (m_Internal[0] << 0);
        }
        else
        {
            return (m_Internal[2] << 16) | (m_Internal[1] << 8) | (m_Internal[0] << 0);
        }
    }

    operator float() const
    {
        return (float)this->operator int();
    }

    static int24 fromuint(unsigned int v) {
        return int24((v&0xFF0000)>>16, (v&0xFF00)>>8, (v&0xFF));
    }

    [[nodiscard]] unsigned int limit( ) const {
        return (m_Internal[2] << 16) | (m_Internal[1] << 8) | m_Internal[0];
    }

    int24& operator =( const int24& input )
    {
        m_Internal[0]   = input.m_Internal[0];
        m_Internal[1]   = input.m_Internal[1];
        m_Internal[2]   = input.m_Internal[2];

        return *this;
    }

    int24& operator =( const int input )
    {
        m_Internal[0]   = ((unsigned char*)&input)[0];
        m_Internal[1]   = ((unsigned char*)&input)[1];
        m_Internal[2]   = ((unsigned char*)&input)[2];

        return *this;
    }

    /***********************************************/

    int24 operator +( const int24& val ) const
    {
        return int24( (int)*this + (int)val );
    }

    int24 operator -( const int24& val ) const
    {
        return int24( (int)*this - (int)val );
    }

    int24 operator *( const int24& val ) const
    {
        return int24( (int)*this * (int)val );
    }

    int24 operator /( const int24& val ) const
    {
        return int24( (int)*this / (int)val );
    }

    int24 operator ~() const
    {
        return int24((~((int)*this)) & 0xFFFFFF );
    }

    int24 operator &( const int24& val ) const
    {
        return int24( ((int)*this & (int)val) & 0xFFFFFF );
    }

    int24 operator |( const int24& val ) const
    {
        return int24( ((int)*this | (int)val) & 0xFFFFFF );
    }

    int24 operator ^( const int24& val ) const
    {
        return int24( ((int)*this ^ (int)val) & 0xFFFFFF );
    }

    /***********************************************/

    int24 operator +( const int val ) const
    {
        return int24( (int)*this + val );
    }

    int24 operator -( const int val ) const
    {
        return int24( (int)*this - val );
    }

    int24 operator *( const int val ) const
    {
        return int24( (int)*this * val );
    }

    int24 operator /( const int val ) const
    {
        return int24( (int)*this / val );
    }

    /***********************************************/
    /***********************************************/


    int24& operator +=( const int24& val )
    {
        *this   = *this + val;
        return *this;
    }

    int24& operator -=( const int24& val )
    {
        *this   = *this - val;
        return *this;
    }

    int24& operator *=( const int24& val )
    {
        *this   = *this * val;
        return *this;
    }

    int24& operator /=( const int24& val )
    {
        *this   = *this / val;
        return *this;
    }

    /***********************************************/

    int24& operator +=( const int val )
    {
        *this   = *this + val;
        return *this;
    }

    int24& operator -=( const int val )
    {
        *this   = *this - val;
        return *this;
    }

    int24& operator *=( const int val )
    {
        *this   = *this * val;
        return *this;
    }

    int24& operator /=( const int val )
    {
        *this   = *this / val;
        return *this;
    }

    /***********************************************/
    /***********************************************/

    int24 operator >>( const int val ) const
    {
        return int24( (int)*this >> val );
    }

    int24 operator <<( const int val ) const
    {
        return int24( (int)*this << val );
    }

    /***********************************************/

    int24& operator >>=( const int val )
    {
        *this = *this >> val;
        return *this;
    }

    int24& operator <<=( const int val )
    {
        *this = *this << val;
        return *this;
    }

    /***********************************************/
    /***********************************************/

    operator bool() const
    {
        return (int)*this != 0;
    }

    bool operator !() const
    {
        return !((int)*this);
    }

    int24 operator -()
    {
        return int24( -(int)*this );
    }

    /***********************************************/
    /***********************************************/

    bool operator ==( const int24& val ) const
    {
        return (int)*this == (int)val;
    }

    bool operator !=( const int24& val ) const
    {
        return (int)*this != (int)val;
    }

    bool operator >=( const int24& val ) const
    {
        return (int)*this >= (int)val;
    }

    bool operator <=( const int24& val ) const
    {
        return (int)*this <= (int)val;
    }

    bool operator >( const int24& val ) const
    {
        return (int)*this > (int)val;
    }

    bool operator <( const int24& val ) const
    {
        return (int)*this < (int)val;
    }

    /***********************************************/

    bool operator ==( const int val ) const
    {
        return (int)*this == val;
    }

    bool operator !=( const int val ) const
    {
        return (int)*this != val;
    }

    bool operator >=( const int val ) const
    {
        return (int)*this >= val;
    }

    bool operator <=( const int val ) const
    {
        return (int)*this <= val;
    }

    bool operator >( const int val ) const
    {
        return ((int)*this) > val;
    }

    bool operator <( const int val ) const
    {
        return (int)*this < val;
    }

    /***********************************************/
    /***********************************************/
};


#endif //CASM_int24_H
