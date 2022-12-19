/*
 * utilities.c
 *
 *  Author: kevvvch
 */ 

#include "utilities.h"

uint32_t string_writeStr(uint8_t *strDest, uint8_t *strSrc)
{
    return string_writeStr_fromPos(strDest, strSrc, 0);
}

uint32_t string_writeStr_fromPos(uint8_t *strDest, uint8_t *strSrc, uint32_t pos)
{
    uint32_t i = 0;

    if(strDest != NULL && strSrc != NULL) {
        while(strSrc[i] != '\0') {
            strDest[pos+i] = strSrc[i];
            i++;
        }
    }

    strDest[pos+i] = '\0';

    return i;
}

uint32_t string_writeNum(uint8_t *strDest, uint32_t num)
{
    return string_writeNum_fromPos(strDest, num, 0);
}

uint32_t string_writeNum_fromPos(uint8_t *strDest, uint32_t num, uint32_t pos)
{
    uint32_t i = 0;
    uint32_t digits = 0;
    
    digits = number_getDigits(num);

    if(strDest != NULL) {
        for(i = 0; i < digits; i++) {
            strDest[pos + digits - 1 - i] = num % 10 + '0';
            num /= 10;
        }

        strDest[pos + digits] = '\0';
    }

    return i;
}

uint32_t string_length(uint8_t *str)
{
    uint32_t len = 0;

    if(str != NULL) {
        while(str[len] != '\0') {
            len++;
        }
    }

    return len;
}

uint32_t string_mirrorStr(uint8_t *strDest, uint8_t *strSrc)
{
    uint32_t i = 0;
    uint32_t len = string_length(strSrc);

    if(strDest != NULL && strSrc != NULL) {
        for(i = 0; i < len; i ++) {
            strDest[i] = strSrc [len - 1 - i];
        }
    }

    return i;
}

uint32_t string_mirror(uint8_t *str)
{
    uint32_t i = 0;
    uint32_t len = string_length(str);
    uint8_t c;

    if(str != NULL) {
        for(i = 0; i < len/2; i ++) {
            c = str[i];
            str[i] = str [len - 1 - i];
            str [len - 1 - i] = c;
        }
    }

    return i;
}

uint8_t string_contains(uint8_t *str, uint8_t *subStr)
{
	uint32_t pos = 0;
	uint32_t i = 0;
    uint8_t contains = 0;
    uint32_t lenSubStr;
    
    if(str != NULL && subStr != NULL) {
        lenSubStr = string_length(subStr);

        while(str[pos] != '\0' && contains == 0) {
            i = 0;
            
            while(str[pos + i] != '\0') {
                if(str[pos + i] != subStr[i]) {
                    contains = 0;
                    break;
                } 
                else if ((lenSubStr - 1) == i) {
                    contains = 1;
                    break;
                }
                
                i++;
            }

            pos++;
		}
    }

    return contains;
}

uint8_t string_containsWithinLength(uint8_t *str, uint8_t *subStr, uint32_t lenStr)
{
	uint32_t pos = 0;
	uint32_t i = 0;
    uint8_t contains = 0;
    uint32_t lenSubStr;
    
    if(subStr != NULL && lenStr != 0) {
        lenSubStr = string_length(subStr);

        if(lenSubStr <= lenStr) {
            while(pos < lenStr && contains == 0) {
                i = 0;

                while((pos + i) < lenStr) {
                    if(str[pos + i] != subStr[i]) {
                        contains = 0;
                        break;
                    }
                    else if ((lenSubStr - 1) == i) {
                        contains = 1;
                        break;
                    }

                    i++;
                }

                pos++;
    		}
        }
    }

    return contains;
}


uint8_t string_equals(uint8_t *str1, uint8_t *str2)
{
	uint32_t i = 0;
    uint8_t equals = 1;

    if(str1 != NULL && str2 != NULL) {
        while(str2[i] != '\0') {
			if(str1[i] != str2[i]) {
                equals = 0;
				break;
			}
			
			i++;
		}
    }
    else {
        equals = 0;
    }

    return equals;
}

uint32_t string_concat(uint8_t *str1, uint8_t *str2)
{
    uint32_t i = 0;
    uint32_t lenStr1 = 0;
    uint32_t lenStr2 = 0;

    if(str1 != NULL && str2 != NULL) {
        lenStr1 = string_length(str1);
        lenStr2 = string_length(str2);

        for(i = 0; i < lenStr2; i++) {
            str1[lenStr1 + i] = str2[i];
        }

    }

    return lenStr1 + lenStr2;
}

uint8_t string_charAt(uint8_t *str, uint32_t pos)
{
    return str[pos];
}

uint32_t string_toUpperCase(uint8_t *str)
{
    uint32_t i = 0;

    if(str != NULL) {
        while(str[i] != '\0') {
            if(str[i] >= 'a' && str[i] <= 'z') {
                str[i] = str[i] - 'a' + 'A';
            }

            i++;
        }
    }

    return i;
}

uint32_t string_toLowerCase(uint8_t *str)
{
    uint32_t i = 0;

    if(str != NULL) {
        while(str[i] != '\0') {
            if(str[i] >= 'A' && str[i] <= 'Z') {
                str[i] = str[i] - 'A' + 'a';
            }

            i++;
        }
    }

    return i;
}

uint8_t string_startsWith(uint8_t *str, uint8_t c)
{
    uint8_t starts = 0;

    if(str != NULL) {
        starts = (str[0] == c) ? 1 : 0;
    }

    return starts;
}

uint32_t string_indexOf(uint8_t *str, uint8_t c)
{
    uint32_t i = 0;
    uint32_t len;

    if(str != NULL) {
        len = string_length(str);

        while(i < len) {
            if(str[i] == c) {
                break;
            }

            i++;
        }

        i = (str[i] == c) ? i : 0;
    }

    return i;
}

uint32_t string_appendChar(uint8_t *str, uint8_t c)
{
    uint32_t i = 0;
    uint32_t len;
    
    if(str != NULL) {
        len = string_length(str);

        str[len] = c;
        str[len+1] = '\0';

        i = len++;
    }

    return i;
}

uint32_t string_appendString(uint8_t *str1, uint8_t *str2)
{
    uint32_t i = 0;
    uint32_t lenStr1;
    uint32_t lenStr2;
    
    if(str1 != NULL && str2 != NULL) {
        lenStr1 = string_length(str1);
        lenStr2 = string_length(str2);

        while(i < lenStr2) {
            str1[lenStr1 + i] = str2[i];

            i++;
        }
    }

    return i;
}

uint32_t string_split(uint8_t *str, uint8_t c, uint8_t *strOut)
{
    uint32_t strLen = 0;
    uint32_t pos = 0;

    if(str != NULL && strOut != NULL) {
        strLen = string_length(str);

        pos = string_indexOf(str, c);

        if(pos > 0) {
            for(uint32_t i = 0; i < pos; i++) {
                strOut[i] = str[i];
            }
            strOut[pos] = '\0';

            for(uint32_t i = pos; i < strLen; i++) {
                str[i - pos] = str[i + 1];
            }
            str[strLen - pos] = '\0';
        }
    }

    return pos;
}

uint32_t ascii_convertNum(uint8_t *strAscii, uint32_t num)
{
    uint32_t i = 0;
    uint32_t digits = 0;
    digits = number_getDigits(num);

    if(strAscii != NULL) {
        for(i = 0; i < digits; i++) {
            strAscii[digits - 1 - i] = num % 10 + '0';
            num /= 10;
        }

        strAscii[digits] = '\0';
    }

    return i;
}





uint32_t number_getDigits(uint32_t num)
{
    uint32_t digits = 0;

	do {
		num /= 10;
		digits++;
	} while(num != 0);

	return digits;
}


uint32_t number_convertDec_toHexa(uint8_t *numHexa, uint32_t numDec)
{
    uint32_t i = 0;
    uint32_t digits = 0;
    uint8_t c;

    if(numHexa != NULL) {
        digits = number_getDigits(numDec);

        for(i = 0; i < digits ; i++) {
            c = numDec % 16;
            c = (c <= 9) ? c + '0' : c - 10 + 'A';
            numHexa[i] = c;

            if(i == digits - 1 && c == '0') {
                numHexa[i] = '\0';
            }

            numDec /= 16;
        }

        numHexa[digits] = '\0';

        string_mirror(numHexa);
    }

    return digits;
}


uint32_t number_convertHexa_toDec(uint32_t *numDec, uint8_t *numHexa)
{
    uint32_t i = 0;
    uint32_t len = 0;

    if(numHexa != NULL && number_isHexa(numHexa)) {
        len = string_length(numHexa);

        for(i = 0; i < len; i++) {
            *numDec *= 16;
            *numDec += numHexa[i] <= '9' ? numHexa[i] - '0' : numHexa[i] - 'A' + 10;

        }
    }

    return len;
}


uint32_t number_removeLeftZeros(uint8_t *num)
{
    uint32_t i = 0;
    uint32_t n;
    uint32_t len;
    uint32_t digits;


    if(num != NULL) {
        n = number_convertAscii_toDec(num);
        digits = number_getDigits(n);
        len = string_length(num);

        for(i = 0; i < digits; i++) {
            num[i] = num[len - digits + i];
        }

        num[i] = '\0';
    }

    return i;
}

uint32_t number_convertAscii_toDec(uint8_t *numAscii)
{
    uint32_t i = 0;
    uint32_t len;
    uint32_t n = 0;

    if(numAscii != NULL) {
        len = string_length(numAscii);

        for(i = 0; i < len; i++) {
            n *= 10;
            n += numAscii[i] - '0';
        }
    }

    return n;
}

uint8_t number_isHexa(uint8_t *numHexa)
{
    uint8_t isHexa = 1;
    uint32_t len = 0;

    if(numHexa != NULL) {
        len = string_length(numHexa);

        for(uint32_t i = 0; i < len; i++) {
            if(numHexa[i] < '0' || (numHexa[i] > '9' && numHexa[i] < 'A') || numHexa[i] > 'F') {
                isHexa = 0;
                break;
            }
        }
    }
    else {
        isHexa = 0;
    }

    return isHexa;
}

uint8_t number_isOct(uint8_t *numOct)
{
    uint8_t isOct = 1;
    uint32_t len = 0;

    if(numOct != NULL) {
        len = string_length(numOct);

        for(uint32_t i = 0; i < len; i++) {
            if(numOct[i] < '0' || numOct[i] > '7') {
                isOct = 0;
                break;
            }
        }
    }
    else {
        isOct = 0;
    }

    return isOct;
}

uint8_t number_isDec(uint8_t *numDec)
{
    uint8_t isDec = 1;
    uint32_t len = 0;

    if(numDec != NULL) {
        len = string_length(numDec);

        for(uint32_t i = 0; i < len; i++) {
            if(numDec[i] < '0' || numDec[i] > '9') {
                isDec = 0;
                break;
            }
        }
    }
    else {
        isDec = 0;
    }

    return isDec;
}


uint8_t number_isBin(uint8_t *numBin)
{
    uint8_t isBin = 1;
    uint32_t len = 0;

    if(numBin != NULL) {
        len = string_length(numBin);

        for(uint32_t i = 0; i < len; i++) {
            if(numBin[i] != '0' && numBin[i] != '1') {
                isBin = 0;
                break;
            }
        }
    }
    else {
        isBin = 0;
    }

    return isBin;
}





uint32_t arrayOfInt_getMax(uint32_t *array, uint32_t len)
{
    uint32_t n = 0;

    if(array != NULL) {
        for(uint32_t i = 0; i < len; i++) {
            if(array[i] > n) {
                n = array[i];
            }
        }
    }

    return n;
}

uint32_t arrayOfInt_getMin(uint32_t *array, uint32_t len)
{
    uint32_t n = -1;

    if(array != NULL) {
        for(uint32_t i = 0; i < len; i++) {
            if(array[i] < n) {
                n = array[i];
            }
        }
    }

    return n;
}

float arrayOfFloat_getMax(float *array, uint32_t len)
{
    float n = 0;

    if(array != NULL) {
        for(uint32_t i = 0; i < len; i++) {
            if(array[i] > n) {
                n = array[i];
            }
        }
    }

    return n;
}

float arrayOfFloat_getMin(float *array, uint32_t len)
{
    float n = 0;

    if(array != NULL) {
        for(uint32_t i = 0; i < len; i++) {
            if(array[i] < n) {
                n = array[i];
            }
        }
    }

    return n;
}

float arrayOfFloat_getAverage(float *array, uint32_t len)
{
    float n = 0;

    if(array != NULL) {
        for(uint32_t i = 0; i < len; i++) {
            if(array[i] < n) {
                n = array[i]/len;
            }
        }
    }

    return n;
}

uint32_t arrayOfUint8_indexOf(uint8_t *array, uint8_t c, uint32_t arrayLen)
{
    uint32_t i = 1;

	while(i < arrayLen) {
		if(array[i-1] == c) {
			break;
		}

		i++;
	}

	i = (array[i-1] == c) ? i : 0;

	return i;
}


uint32_t arrayOfUint8_split(uint8_t *array, uint8_t c, uint32_t arrayLen, uint8_t *arrayOut)
{
    uint32_t pos = 0;


	pos = arrayOfUint8_indexOf(array, c, arrayLen);

	if(pos > 0) {
        pos -= 1;

		for(uint32_t i = 0; i < pos + 1; i++) {
			arrayOut[i] = array[i];
		}
		arrayOut[pos + 1] = '\0';

		for(uint32_t i = pos; i < arrayLen; i++) {
			array[i - pos] = array[i + 1];
		}
		array[arrayLen - pos] = '\0';

        pos += 1;
	}

    return pos;
}

float numberFloat_getLinearValue(float x, float table[][2], uint32_t size)
{
    float y = 0;
    float m = 0;
    uint32_t nSegment = 0;

    while(x > table[nSegment][0] && nSegment < size) {
        nSegment++;
    }

    if(nSegment == 0) {
        nSegment = 1;
    }
    else if(nSegment == size) {
        nSegment = size - 1;
    }

    m = (table[nSegment][1]-table[nSegment-1][1]) / (table[nSegment][0]-table[nSegment-1][0]);
    y = m * (x - table[nSegment-1][0]) + table[nSegment-1][1];

    return y;
}

void arrayOfUint8_zeros(uint8_t *array, uint32_t arrayLen)
{
	for(uint32_t i = 0; i < arrayLen; i++) {
		array[i] = 0;
	}
}


uint32_t ringBufferOfUint8_indexOf(uint8_t *rb, uint32_t ptrStart, uint32_t ptrEnd, uint8_t c, uint32_t rbLen)
{
    uint32_t i = ptrStart;
    uint8_t matched = 0;

	while( i != ptrEnd) {

		if(rb[i] == c) {
            matched = 1;
			break;
		}

		i++;
        i%=rbLen;
	}

	i = (matched == 1) ? i+1 : 0;

	return i;
}

uint32_t ringBufferOfUint8_popChunk(uint8_t *rb, uint32_t ptrStart, uint32_t ptrEnd, uint8_t c, uint32_t rbLen, uint8_t *chunk)
{
    uint32_t pos = 0;
    uint32_t chunkPos = 0;

	pos = ringBufferOfUint8_indexOf(rb, ptrStart, ptrEnd, c, rbLen);

	if(pos > 0) {
		pos -= 1;

		for(uint32_t i = ptrStart; i != pos + 1; i++, chunkPos++) {
			i%=rbLen;
			chunk[chunkPos] = rb[i];
		}
		chunk[chunkPos] = '\0';
	}

	return chunkPos;
}
