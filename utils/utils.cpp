#include <windows.h>
#include <string>
 

//Взято отсюда: https://ru.stackoverflow.com/questions/783946/Конвертировать-в-кодировку-utf8

//Функция преобразования кодировки строки. По умолчанию из utf-8 в текущую OEM (обычно cp866).
std::string cpt(const char *str, unsigned int srcCP = CP_UTF8, unsigned int dstCP = CP_ACP){
	std::string res;	
	int result_u, result_c;

	result_u = MultiByteToWideChar(srcCP,	0,	str, -1, 0,	0);
	if (!result_u)	return 0;

	wchar_t *ures = new wchar_t[result_u];
	if(!MultiByteToWideChar(srcCP, 0, str, -1, ures, result_u))	{
		delete[] ures;
		return 0;
	}
	result_c = WideCharToMultiByte(dstCP, 0, ures, -1, 0, 0,	0, 0);
	if(!result_c)	{
		delete [] ures;
		return 0;
	}
	char *cres = new char[result_c];
	if(!WideCharToMultiByte(dstCP, 0, ures, -1, cres, result_c, 0, 0))	{
		delete[] cres;
		return 0;
	}
	delete[] ures;
	res.append(cres);
	delete[] cres;
	return res;
}

//Взято отсюда: https://ru.stackoverflow.com/questions/783946/Конвертировать-в-кодировку-utf8

//Функция проверки кодировки строки на utf-8.
bool is_valid_utf8(const char * string){
    if(!string){return true;}
    const unsigned char * bytes = (const unsigned char *)string;
    unsigned int cp;
    int num;
    while(*bytes != 0x00){
        if((*bytes & 0x80) == 0x00){
            // U+0000 to U+007F 
            cp = (*bytes & 0x7F);
            num = 1;
        }
        else if((*bytes & 0xE0) == 0xC0){
            // U+0080 to U+07FF 
            cp = (*bytes & 0x1F);
            num = 2;
        }
        else if((*bytes & 0xF0) == 0xE0){
            // U+0800 to U+FFFF 
            cp = (*bytes & 0x0F);
            num = 3;
        }
        else if((*bytes & 0xF8) == 0xF0){
            // U+10000 to U+10FFFF 
            cp = (*bytes & 0x07);
            num = 4;
        }
        else{return false;}
        bytes += 1;
        for(int i = 1; i < num; ++i){
            if((*bytes & 0xC0) != 0x80){return false;}
            cp = (cp << 6) | (*bytes & 0x3F);
            bytes += 1;
        }
        if( (cp > 0x10FFFF) ||
            ((cp <= 0x007F) && (num != 1)) || 
            ((cp >= 0xD800) && (cp <= 0xDFFF)) ||
            ((cp >= 0x0080) && (cp <= 0x07FF)  && (num != 2)) ||
            ((cp >= 0x0800) && (cp <= 0xFFFF)  && (num != 3)) ||
            ((cp >= 0x10000)&& (cp <= 0x1FFFFF)&& (num != 4)) ){return false;}
    }
    return true;
}