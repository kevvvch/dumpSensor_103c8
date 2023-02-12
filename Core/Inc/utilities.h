/*
 * utilities.h
 *
 *  Author: kevvvch
 */ 

#include <stdint.h>
#include <stddef.h>

uint32_t string_writeStr(uint8_t *strDest, uint8_t *strSrc);
uint32_t string_writeStr_fromPos(uint8_t *strDest, uint8_t *strSrc, uint32_t pos);
uint32_t string_writeNum(uint8_t *strDest, uint32_t num);
uint32_t string_writeNum_fromPos(uint8_t *strDest, uint32_t num, uint32_t pos);
uint32_t string_length(uint8_t *str);
uint32_t string_mirrorStr(uint8_t *strDest, uint8_t *strSrc);
uint32_t string_mirror(uint8_t *str);
uint8_t string_contains(uint8_t *str, uint8_t *subStr);
uint8_t string_containsWithinLength(uint8_t *str, uint8_t *subStr, uint32_t lenStr);
uint8_t string_equals(uint8_t *str1, uint8_t *str2);
uint32_t string_concat(uint8_t *str1, uint8_t *str2);
uint8_t string_charAt(uint8_t *str, uint32_t pos);
uint32_t string_toUpperCase(uint8_t *str);
uint32_t string_toLowerCase(uint8_t *str);
uint8_t string_startsWith(uint8_t *str, uint8_t c);
uint32_t string_indexOf(uint8_t *str, uint8_t c);
uint32_t string_appendChar(uint8_t *str, uint8_t c);
uint32_t string_appendString(uint8_t *str1, uint8_t *str2);
uint32_t string_split(uint8_t *str, uint8_t c, uint8_t *strOut);



uint32_t ascii_convertNum(uint8_t *strAscii, uint32_t num);



uint32_t number_getDigits(uint32_t num);
uint32_t number_convertDec_toHexa(uint8_t *numHexa, uint32_t numDec);
uint32_t number_convertHexa_toDec(uint32_t *numDec, uint8_t *numHexa);
uint32_t number_convertAscii_toDec(uint8_t *numAscii);
uint32_t number_removeLeftZeros(uint8_t *num);
uint8_t number_isHexa(uint8_t *numHexa);
uint8_t number_isOct(uint8_t *numOct);
uint8_t number_isDec(uint8_t *numDec);
uint8_t number_isBin(uint8_t *numBin);

void number_convertUint32_toArrayOfUint8(uint8_t *hex, uint32_t value);


uint32_t arrayOfUint8_indexOf(uint8_t *array, uint8_t c, uint32_t arrayLen);
uint32_t arrayOfUint8_split(uint8_t *array, uint8_t c, uint32_t arrayLen, uint8_t *arrayOut);


float numberFloat_getLinearValue(float x, float table[][2], uint32_t size);

uint32_t arrayOfInt_getMax(uint32_t *array, uint32_t len);
uint32_t arrayOfInt_getMin(uint32_t *array, uint32_t len);
void arrayOfUint8_zeros(uint8_t *array, uint32_t arrayLen);

uint32_t ringBufferOfUint8_indexOf(uint8_t *rb, uint32_t ptrStart, uint32_t ptrEnd, uint8_t c, uint32_t rbLen);
uint32_t ringBufferOfUint8_popChunk(uint8_t *rb, uint32_t ptrStart, uint32_t ptrEnd, uint8_t c, uint32_t rbLen, uint8_t *chunk);
