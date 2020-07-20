#include "bitstream.h"
#include <stdio.h>

#define RESIDUE() (length-headresidue)%BYTE_SIZE

BYTE safemask(int len){
	return BYTE_FULL>>(BYTE_SIZE-len);
}

void partial_full_Copy(BYTE* low,BYTE* high,int lresidue){
	BYTE d=*high;
	*(low)=(d << lresidue)+*(low);
	*high= (d >>(BYTE_SIZE-lresidue)) & safemask(lresidue);
}

void bitstream::align(){
	int residue=RESIDUE();
	if(residue){
		int lresidue=headresidue;
		if(headresidue==0)
			headresidue=BYTE_SIZE;
		BYTE llast = *tail;
		if(residue+headresidue>BYTE_SIZE)
		{
			BYTE d=*head;
			headresidue=residue+headresidue-BYTE_SIZE;
			lresidue=BYTE_SIZE-residue;
			dec_head();
			*head=d & safemask(headresidue);
			inc_head();
			*head=(d >> headresidue) & safemask(BYTE_SIZE-headresidue);
			dec_tail();
		}
		if(head>tail){
			for(BYTE* i=next(head);i<=last;i++)
				partial_full_Copy(pre(i),i,lresidue);
			for(BYTE* i=stream;i<=tail;i++)
				partial_full_Copy(pre(i),i,lresidue);
		}
		else if(head<tail)
			for(BYTE* i=head+1;i<=tail;i++)
				partial_full_Copy(i-1,i,lresidue);
		if(lresidue!=headresidue){
			dec_head();
			partial_full_Copy(tail,&llast,lresidue);
			inc_tail();
			if(llast!=0)
				throw "2 Days of algorithm design gone down the gutter";
		}
		else
			headresidue=lresidue+residue;
	}
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
void bitstream::dec_tail(){
	if(tail==stream)
		tail=last;
	else tail--;
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
/*void bitstream::partialFetch(BYTE* &buf,int residue, int &len){
	BYTE b=*(buf++);
	length+=(len>BYTE_SIZE)?BYTE_SIZE:len;
	len-=BYTE_SIZE;
	*tail=((*tail)& safemask(residue))+(b<<residue);
	if(len>=-1*residue)
	{
		inc_tail();
		*tail=b>>(BYTE_SIZE-residue);
	}
}*/
void bitstream::bareAlignedwrite(BYTE* buf,int len){
	int n=len/BYTE_SIZE;
	if(n>0){
		if(tail<head || last-tail>=n-1){
			memcpy(tail,buf,sizeof(BYTE)*n);
			tail+=n;
			buf+=n;
			if(tail>last)
				tail=stream;
		}			
		else{
			memcpy(tail,buf,sizeof(BYTE)*(last-tail+1));
			tail=stream;
			n-=last-tail+1;
			buf+=last-tail+1;
			memcpy(tail,buf,sizeof(BYTE)*n);
			tail+=n;
			buf+=n;
		}
		length+=(len/BYTE_SIZE)*BYTE_SIZE;
	}
	if(len%BYTE_SIZE)
	{
		length+=len%BYTE_SIZE;
		*tail=*buf & safemask(len%BYTE_SIZE);
	}
}
int bitstream::trivialWrite(BYTE* buf,int len){
	if(length+len<=BYTE_SIZE){
		*head=(*head & safemask(length))+ ((*buf & safemask(len))<<length);
		headresidue=(len+length)%BYTE_SIZE;
		tail=next(head);
		length+=len;
		return 1;
	}
	return 0;
}
void bitstream::writeBits(BYTE* buf,int len){
	if(len<=0)
		throw "Invalid write";
	if(trivialWrite(buf,len))
		return;
	resize(len);
	int residue=RESIDUE();
	if(residue)
		throw "Can't write until aligned";
	bareAlignedwrite(buf,len);
	//if(residue)
	//	while(len>0)
	//		partialFetch(buf,residue,len);
			/*if(len>BYTE_SIZE){
				
				inc_tail();
				*tail=b>>(BYTE_SIZE-residue);
			}
			else{
				BYTE b=*(buf);
				length+=len;
				len-=(BYTE_SIZE-residue);
				*tail=((*tail)& safemask(residue))+(b<<residue);
				if(len>0)
				{
					inc_tail();
					*tail=b>>(BYTE_SIZE-residue);
					len=0;
				}
				else if(len==0)
					inc_tail();
			}*/
	//else
	//{
		
	//}
}

void bitstream::writeUABits(BYTE* buf,int len){
	if(len<=0)
		throw "Invalid write";
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
			BYTE d=*buf & safemask(len);
			*tail=(d<<residue)+*tail;
			if(residue+len>BYTE_SIZE){
				inc_tail();
				*tail=(d>>BYTE_SIZE-residue)& safemask(residue);
			}
			else if(residue+len==BYTE_SIZE)
				inc_tail();
			length+=len;
		}
	}	
}

void bitstream::readBits(BYTE* buf,int len,int endian){
	if(len>length || len<=0)
		throw "Invalid read";
	if(headresidue){
		do{
			if(len>=BYTE_SIZE){
				*buf=*head;
				inc_head();
				partial_full_Copy(buf++,head,headresidue);
				length-=BYTE_SIZE;
				if(head==tail){
					headresidue=0;
					len-=BYTE_SIZE;
					break;
				}
			}
			else{
				int tread=(len>headresidue)?headresidue:len;
				*buf=*head & safemask(tread);
				headresidue-=tread;
				len-=tread;
				length-=tread;
				if(!headresidue)
					inc_head();
				if(len)
				{
					*buf= ((*head & safemask(len)) << tread)+*buf;
					length-=len;
					len=0;
					*head=(*head >> len) & safemask(BYTE_SIZE-len);
					if(head==tail)
						break;
					headresidue=BYTE_SIZE-len;
				}
			}
		}while((len-=BYTE_SIZE)>0);
	}
	if(len>0 && !headresidue)
		do{
			if(len>=BYTE_SIZE){
				*(buf++)=*head;
				inc_head();
				length-=BYTE_SIZE;
			}
			else{
				*(buf++)=(*head & safemask(len));
				*head= (*head >> len) & safemask(BYTE_SIZE-len);
				length-=len;
				if(head==tail)
					break;
				headresidue=BYTE_SIZE-len;
			}
		}while((len-=BYTE_SIZE)>0);
	
	if(head==tail && length>0)
	{
		inc_tail();
		int residue=RESIDUE();
		if(!residue)
			throw "Algoithmic error";
		headresidue=residue;
	}
}

int main(){
	bitstream bs;
	BYTE h=51;
	try{
		/*bs.writeBits(&h,2);
		bs.writeBits(&h,4);
		bs.writeBits(&h,1);
		bs.writeBits(&h,6);
		bs.writeUABits(&h,3);*/
		bs.writeBits(&h,7);
		bs.writeBits(&h,2);
		bs.align();
	}
	catch (const char* c){
		printf("%s\n",c);
	}
	return 0;
}