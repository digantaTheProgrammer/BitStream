# BitStream

BitStream aims to be a C++ library to deal with, you guessed it, bitstreams !!!

## Usage

```C++
#include <bitstream.h>;
int main()
{
    //BYTE & BYTE_SIZE defined in bitstream.h
    BYTE data[500]; // 500*BYTE_SIZE bits of data
    bitstream bs;
    bs.writeBits(data,254);// Write the first 254 bits into the bitstream
    BYTE buffer[80];
    bs.readBits(buffer,80,0);// Read the first 80 bits into buffer. bitstream now has 174 bits
}
```
## Example
```C++
//BYTE_SIZE = 8, sizeof(BYTE) >= 8. Let BYTE = char. 
//sizeof(BYTE) = 8
BYTE buff[3];
buff[0]=20;	 // 00010100
buff[1]=121; // 01111001
buff[2]=179; // 10110011
bitstream bs; // Content of bs : <| ,where < is the reading end, | is the writing end, - signifies the alignment 
bs.writeBits(buff,19); // Content of bs : <00101000-10011110-110|

//bitstream has become unaligned. Subsequent calls to bs.writeBits will fail until bs.align() is called or bs.writeUABits() is used for writing. 
//bs.writeBits() guarantees to use memcpy for the writing operations rather than manual iteration , which is only possible when bits are BYTE_SIZE aligned.
```
Now with the above content in bs, let's perform the following experiments independently
#### Experiment : 1
```C++
BYTE rbuff[4]; //Content of rbuff : 00000000-00000000-00000000-00000000
bs.readBits(rbuff,5,0);//rbuff : 		00010100-00000000-00000000-00000000
//bs : <000-10011110-110|
bs.readBits(rbuff,4,0);//rbuff :		00001000-00000000-00000000-00000000
//bs : <0011110-110|
bs.readBits(rbuff,9,0);//rbuff :		10111100-00000001-00000000-00000000
//bs : <0|
bs.readBits(rbuff,1,0);//rbuff :		00000000-00000000-00000000-00000000
//bs : <|
```
#### Experiment : 2
```C++
BYTE rbuff[4]; //Content of rbuff : 00110100-10111010-11100100-00001001 (garbage value)
bs.readBits(rbuff,5,0);//rbuff : 		00010100-10111010-11100100-00001001
//bs : <000-10011110-110|
bs.readBits(rbuff,4,0);//rbuff :		00001000-10111010-11100100-00001001
//bs : <0011110-110|
bs.align();
//bs : <00-11110110| , now writeBits() will succeed
//After alignment (compare with Experiment : 1) : 
bs.readBits(rbuff,9,0);//rbuff :		10111100-00000001-11100100-00001001
//bs : <0| // writeBits() still succeeds

if(cond1){
	BYTE dbuff[1];
	dbuff[0]=51; //00110011
	bs.writeBits(dbuff,6);
	//bs :<0110011|
	dbuff[0]=51; //00110011 (dbuff[0] can be corrupted once written into stream)
	bs.writeBits(dbuff,1);
	//bs :<01100111|
}
else if(cond2){
	BYTE dbuff[1];
	dbuff[0]=51; //00110011
	bs.writeBits(dbuff,6);
	//bs :<0110011|
	dbuff[0]=51; //00110011 (dbuff[0] can be corrupted once written into stream)
	bs.writeBits(dbuff,3);
	//bs :<0110011-110|
}
else
	bs.readBits(rbuff,1,0);//rbuff :		00000000-00000001-11100100-00001001
//bs : <|
```
#### Experiment : 3
```C++
BYTE rbuff[4]; //rbuff: 			00000000-00000000-00000000-00000000
//Endian-ness only works with BYTE_SIZE multiples
bs.readBits(rbuff,16,1);//rbuff:	01111001-00010100-00000000-00000000
//bs :<110|
```
Hope the experiments will clarify the usage further. Cheers!
## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
[MIT](https://choosealicense.com/licenses/mit/)