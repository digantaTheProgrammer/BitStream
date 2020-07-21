#ifndef BITSTREAM_LIB
#define BITSTREAM_LIB
#include <memory.h>
#include <stdlib.h>
#define BYTE_SIZE 8
#define BYTE char  //BYTE should have size in bits > BYTE_SIZE, also LSB BYTE_SIZE bits are considered as values. It does not guarantee that while reading, sizeinbits(BYTE)-BYTE_SIZE MSB bits will be set to zero. Also while writing, the buffer is corrupted
#define BYTE_FULL 255 // Value of type BYTE such that all BYTE_SIZE LSB are set and all other bits unset
#define INIT_CAP 1
int LEN_BYTE_IN(int len){ return ((len/BYTE_SIZE)+((len % BYTE_SIZE)?0:-1));} //returns index offset in len bits

class bitstream{
	public:
		void writeBits(BYTE*,int);
		void writeUABits(BYTE*,int);
		void readBits(BYTE*,int,int);
		int getLength();
		void align();
		bitstream(){capacity=INIT_CAP*BYTE_SIZE;stream=(BYTE*)malloc(INIT_CAP*sizeof(BYTE));head=tail=stream;last=LEN_BYTE_IN(capacity)+stream;headresidue=0;length=0;};
	private:
		int capacity;
		int length;
		BYTE* stream;
		BYTE* head;
		BYTE* tail;
		BYTE* last;
		int headresidue;
		void aligned_copy(BYTE*);
		void resize(int);
		void inc_head();
		void inc_tail();
		void dec_head();
		void dec_tail();
		BYTE* pre(BYTE*);
		BYTE* next(BYTE*);
		void bareAlignedwrite(BYTE*,int);
		void bareAlign();
		int trivialWrite(BYTE*,int);
		int trivialRead(BYTE*,int);
		int RESIDUE();
		int isAligned();
		BYTE* alignedIterate(BYTE*,int,int);
};
#endif