#ifndef __OBJECTHEADER_H__
#define __OBJECTHEADER_H__

#include "xdefines.h"

class objectHeader {
public:
	enum { MAGIC = 0xCAFEBABE };

	objectHeader (size_t sz)
	: _size (sz),
   	  _magic (MAGIC)
	{
	}

	size_t getSize (void) { 
    sanityCheck(); 
    return _size; 
  }

private:
	bool sanityCheck (void) {
		if (_magic != MAGIC) {
			PRERR("********HLY FK: Current _magic %x at %p***********\n", _magic, &_magic);
			::abort();
		}
		return true;
	}

	size_t _magic;
	size_t _size;
};

#endif /* __OBJECTHEADER_H__ */
