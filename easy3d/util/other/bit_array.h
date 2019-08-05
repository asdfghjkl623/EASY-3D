
#ifndef _MPL_BASIC_BIT_ARRAY_H_
#define _MPL_BASIC_BIT_ARRAY_H_


#include <basic/basic_common.h>


inline unsigned int BitsToBytes(unsigned int nb_bits)
{
	//std::size_t num_needed = (nb_bits + 7) / 8;
	return (nb_bits >> 3) + ((nb_bits & 7) ? 1 : 0);
}

inline unsigned int BitsToDwords(unsigned int nb_bits)
{
	//std::size_t num_needed = (nb_bits + 31) / 32;
	return (nb_bits >> 5) + ((nb_bits & 31) ? 1 : 0);
}


// Use this one instead of an array of bools. Takes less ram, nearly as fast [no bounds checking and so on].
class BASIC_API BitArray
{
public:
	//! Constructor
	BitArray();
	BitArray(unsigned int nb_bits);
	//! Destructor
	~BitArray();

	bool init(unsigned int nb_bits);

	// Data management
	inline	void set(unsigned int idx)		{ mBits[idx >> 5] |= 1 << (idx & 31);		}
	inline	void unset(unsigned int idx)	{ mBits[idx >> 5] &= ~(1 << (idx & 31));	}
	inline	void toggle(unsigned int idx)	{ mBits[idx >> 5] ^= 1 << (idx & 31);		}

	inline	void clear()	{ memset(mBits, 0, mSize * sizeof(unsigned int)); }
	inline	void set_all()	{ memset(mBits, 0xff, mSize * sizeof(unsigned int)); }

	// Data access
	inline	bool is_set(unsigned int idx) const { return (mBits[idx >> 5] & (1 << (idx & 31))) != 0; }

	inline	const unsigned int*	bits() const	{ return mBits; }
	inline	unsigned int		size() const	{ return mSize; }

protected:
	unsigned int*	mBits;		//!< Array of bits   // must be 32 bits
	unsigned int	mSize;		//!< Size of the array
};


#endif // _MPL_BASIC_BIT_ARRAY_H_
