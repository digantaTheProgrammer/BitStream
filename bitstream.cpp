#include "bitstream.h"
#include <stdio.h>

int bitstream::RESIDUE() {
	return (length-headresidue)%BYTE_SIZE;
}

BYTE safemask(int len){
	return BYTE_FULL>>(BYTE_SIZE-len);
}

BYTE lsb_as_lsb(BYTE* b,int len){
	return *b & safemask(len);
}
BYTE shiftout_lsb(BYTE* b,int len){
	return (*b>>len) & safemask(BYTE_SIZE-len);
}

BYTE shift_for_residue(BYTE* b,int residue){
	return *b << residue;
}
BYTE overflown_in_residue(BYTE* b,int residue){
	return (*b>>(BYTE_SIZE-residue)) & safemask(residue);
}

BYTE lsb_shift_residue(BYTE* b,int len,int residue){
	return (*b & safemask(len))<< residue;
}

// empty(0)<Partial<full(BYTE_SIZE) (strictly)
//Ensure low is partially filled AND high is fully filled
void partial_full_Copy(BYTE* low,BYTE* high,int lresidue){
	*low=shift_for_residue(high,lresidue)+ *low;
	*high=overflown_in_residue(high,lresidue);
}
//Ensure both are partial. Returns true if higher byte is required
int partial_partial_Copy(BYTE* low,BYTE* high,int lresidue,int hresidue){
	partial_full_Copy(low,high,lresidue);
	return (lresidue+hresidue>BYTE_SIZE);
}

int bitstream::isAligned(){
	return !(RESIDUE() && length>BYTE_SIZE);
}
void bitstream::bareAlign(){
	int residue=RESIDUE();
	int lresidue=headresidue;
	if(headresidue==0)
		headresidue=BYTE_SIZE;
	BYTE llast = *tail;
	if(residue+headresidue>BYTE_SIZE)
	{
		BYTE d=*head;
		headresidue=residue+headresidue-BYTE_SIZE;
		lresidue=BYTE_SIZE-residue;
		*pre(head)=lsb_as_lsb(head,headresidue);
		*head=shiftout_lsb(head,headresidue);
		dec_tail();
	}

	BYTE* stop=next(tail);
	for(BYTE* i=next(head);i!=stop;i=next(i))
			partial_full_Copy(pre(i),i,lresidue);//If tail is not full then headresidue+residue<=BYTE_SIZE , so the result of partial_partial_copy will be false is known in advance hence partal_full_copy works in that case too
	if(lresidue!=headresidue){
		dec_head();
		if(partial_partial_Copy(tail,&llast,lresidue,residue))
			throw "Algoithmic error";
		inc_tail();
	}
	else
		headresidue=lresidue+residue;
}
void bitstream::align(){
	if(!isAligned())
		bareAlign();
	else 
		throw "Already aligned!!";	
}

void bitstream::aligned_copy(BYTE* dst){
	int num=LEN_BYTE_IN(length-headresidue)+1+(headresidue>0);
	if(head>=tail)
		{
			memcpy(dst,head,sizeof(BYTE)*(last-head+1));
			memcpy(dst+(last-head)+1,stream,sizeof(BYTE)*(tail-stream));
		}
	else
		memcpy(dst,head,sizeof(BYTE)*(tail-head));
	head=dst;
	tail=head+num;
}
//Assumes trivialWrite failed
void  bitstream::resize(int len){
	if(LEN_BYTE_IN(len+length-headresidue)+(headresidue>0) >LEN_BYTE_IN(capacity)){
		int residue=RESIDUE();
		if(residue)
			align();
		while(LEN_BYTE_IN(capacity)<LEN_BYTE_IN(len+length-headresidue)+(headresidue>0))capacity*=2;
		BYTE* nstream=(BYTE*)malloc((LEN_BYTE_IN(capacity)+1)*sizeof(BYTE));
		aligned_copy(nstream);
		free(stream);
		stream=nstream;
		last=head+LEN_BYTE_IN(capacity);
	}
}

void bitstream::inc_tail(){
	if(tail==last)
		tail=stream;
	else
		tail++;
}
void bitstream::dec_tail(){
	if(tail==stream)
		tail=last;
	else tail--;
}

void bitstream::inc_head(){
	if(head==last)
		head=stream;
	else
		head++;
}
void bitstream::dec_head(){
	if(head==stream)
		head=last;
	else head--;
}

BYTE* bitstream::pre(BYTE* b){
	if(b==stream)
		return last;
	return b-1;
}
BYTE* bitstream::next(BYTE* b){
	if(b==last)
		return stream;
	return b+1;
}

#define READ 1
#define WRITE 0
BYTE* bitstream::alignedIterate(BYTE* dst,int n,int read){
	BYTE* &ltail = (read==READ) ? head : tail;
	BYTE* &lhead = (read==READ) ? tail : head;
	if(ltail<lhead || last-ltail>=n-1){
		if(read==READ)
			memcpy(dst,ltail,sizeof(BYTE)*n);
		else
			memcpy(ltail,dst,sizeof(BYTE)*n);
		ltail+=n;
		dst+=n;
		if(ltail>last)
			ltail=stream;
	}			
	else{
		if(read==READ)
			memcpy(dst,ltail,sizeof(BYTE)*(last-ltail+1));
		else
			memcpy(ltail,dst,sizeof(BYTE)*(last-ltail+1));
		ltail=stream;
		n-=last-ltail+1;
		dst+=last-ltail+1;
		if(read==READ)
			memcpy(dst,ltail,sizeof(BYTE)*n);
		else
			memcpy(ltail,dst,sizeof(BYTE)*n);
		ltail+=n;
		dst+=n;
	}
	return dst;
}

void bitstream::bareAlignedwrite(BYTE* buf,int len){
	int n=len/BYTE_SIZE;
	if(n>0)
		buf=alignedIterate(buf,n,WRITE);
	length+=n*BYTE_SIZE;
	if(len%BYTE_SIZE)
	{
		length+=len%BYTE_SIZE;
		*tail=lsb_as_lsb(buf,len%BYTE_SIZE);
	}
}
int bitstream::trivialWrite(BYTE* buf,int len){
	if(length>=BYTE_SIZE)
		return 0;
	headresidue=0;
	if(length+len<=BYTE_SIZE){
		BYTE d=lsb_as_lsb(buf,len);
		*head=shift_for_residue(&d,length)+*head;
		length+=len;
		if(length==BYTE_SIZE)
			inc_tail();
		return 1;
	}
	headresidue=length;
	if(length)
		tail=next(head);
	else
		tail=head;
	return 0;
}
void bitstream::writeBits(BYTE* buf,int len){
	if(len<=0)
		throw "Invalid write";
	//trivialWrite must be called before anything
	if(trivialWrite(buf,len))
		return;
	resize(len);
	int residue=RESIDUE();
	if(residue)
		throw "Can't write until aligned";
	bareAlignedwrite(buf,len);
}

void bitstream::writeUABits(BYTE* buf,int len){
	if(len<=0)
		throw "Invalid write";
	//trivialWrite must be called before anything
	if(trivialWrite(buf,len))
		return;
	resize(len);
	int residue = RESIDUE();
	if(!residue)
		bareAlignedwrite(buf,len);
	else{
		while(len>=BYTE_SIZE){
			partial_full_Copy(tail,buf,residue);
			inc_tail();
			*tail=*(buf++);
			len-=BYTE_SIZE;
			length+=BYTE_SIZE;
		}
		if(len)
		{
			BYTE d=lsb_as_lsb(buf,len);
			*tail=shift_for_residue(&d,residue)+*tail;
			if(residue+len>BYTE_SIZE){
				inc_tail();
				*tail=overflown_in_residue(&d,residue);
			}
			else if(residue+len==BYTE_SIZE)
				inc_tail();
			length+=len;
		}
	}	
}

int bitstream::trivialRead(BYTE* buf,int len){
	if(length>=BYTE_SIZE)
		return 0;
	length-=len;
	*buf=lsb_as_lsb(head,len);
	*head=shiftout_lsb(head,len);
	return 1;
}

void bitstream::readBits(BYTE* buf,int len,int endian){
	if(len>length || len<=0)
		throw "Invalid read";
	if(trivialRead(buf,len))
		return;
	if(endian && len%BYTE_SIZE)
		throw "Ambiguous endianness";
	int partial=0;
	if(headresidue)
		do{
			if(len>=BYTE_SIZE){
				*buf=*head;
				inc_head();
				partial_full_Copy(buf++,head,headresidue);
				length-=BYTE_SIZE;
			}
			else{
				int tread=(len>headresidue)?headresidue:len;
				length-=tread;
				*buf=lsb_as_lsb(head,tread);
				headresidue-=tread;
				len-=tread;
				if(!headresidue)
					inc_head();
				else
					*head=shiftout_lsb(head,tread);
				partial=tread;
				break; //Crucial for the last if block
			}
		}while((len-=BYTE_SIZE)>0);

	else if(len>=BYTE_SIZE)
		if(endian){
			buf=LEN_BYTE_IN(len)+buf;
			do{
				*(buf--)=*head;
				inc_head();
				length-=BYTE_SIZE;
			}while((len-=BYTE_SIZE)>=BYTE_SIZE);
		}
		else{
			int n=len/BYTE_SIZE;
			buf=alignedIterate(buf,n,READ);
			length-=n*BYTE_SIZE;
			len-=n*BYTE_SIZE;
		}
	if(len){
		BYTE d=lsb_as_lsb(head,len);
		if(partial)
			*buf= shift_for_residue(&d,partial)+*buf;
		else
			*buf=d;
		*head=shiftout_lsb(head,len);
		length-=len;
		headresidue=BYTE_SIZE-len;
	}
}