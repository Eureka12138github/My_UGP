/*
  版权所有 (c) 2009 Dave Gamble
 特此免费授予任何获得本软件及其相关文档文件（统称为“软件”）副本的人，在不违反以下条件的情况下，对软件进行使用、复制、修改、合并、发布、分发、 再许可和/或销售软件副本，并允许向获得软件的人分发软件副本的权利。上述版权声明和本许可声明应包含在软件的所有副本或软件的实质性部分中。该软件按“原样”提供，不附带任何明示或暗示的保证，包括但不限于适销性、适用于特定目的和不侵权的保证。在任何情况下，作者或版权持有者均不对因软件或使用或与软件相关的其他交易而引起的任何索赔、损害或其他责任负责，无论该责任是基于合同、侵权行为或其他方式。
*/

/* cJSON */
/* JSON parser in C. */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "cJSON.h"

static const char *ep;

/**
 * 获取JSON解析错误指针
 * 
 * 当JSON解析失败时，本函数返回指向错误位置的指针，用于调试和错误处理
 * 
 * 返回:
 *   const char*: 指向JSON字符串中发生错误的位置如果解析成功或无错误信息，则返回NULL
 */
const char *cJSON_GetErrorPtr(void) {return ep;}

/**
 * @brief 比较两个字符串是否相等，不区分大小写
 * 
 * 该函数通过比较两个字符串的每个字符（转换为小写）来判断字符串是否相等
 * 它继续比较直到遇到字符串的结尾，或者找到不匹配的字符
 * 
 * @param s1 第一个字符串，如果为NULL，则仅当s2也为NULL时返回0，否则返回1
 * @param s2 第二个字符串，如果为NULL，则仅当s1也为NULL时返回0，否则返回1
 * @return int 返回两个字符串的差异，如果字符串相等（不区分大小写），则返回0
 */
static int cJSON_strcasecmp(const char *s1,const char *s2)
{
    // 检查s1是否为NULL，如果s1和s2都为NULL，则返回0，否则返回1
    if (!s1) return (s1==s2)?0:1;
    // 如果s2为NULL，则返回1，因为此时s1不为NULL
    if (!s2) return 1;
    // 遍历字符串，比较每个字符（转换为小写），直到找到不匹配的字符或到达字符串结尾
    for(; tolower(*s1) == tolower(*s2); ++s1, ++s2) 
        // 如果当前字符为字符串结尾（'\0'），则返回0，表示字符串相等
        if(*s1 == 0)	return 0;
    // 返回两个不匹配字符的差异，表示字符串不相等
    return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

static void *(*cJSON_malloc)(size_t sz) = malloc;
static void (*cJSON_free)(void *ptr) = free;

/**
 * cJSON_strdup函数用于复制一个字符串。
 * 它是cJSON库中的一个辅助函数，用于处理字符串复制操作。
 * 
 * @param str 需要复制的字符串指针。
 * 如果输入的字符串为NULL，函数将返回NULL。
 * 
 * @return 返回一个指向新分配的字符串的指针，内容与输入字符串相同。
 * 如果内存分配失败，函数将返回NULL。
 * 
 * 注意：调用者负责释放返回的字符串指针以避免内存泄漏。
 */
static char* cJSON_strdup(const char* str)
{
    // 计算输入字符串的长度，包括结尾的空字符
    size_t len;
    char* copy;

    len = strlen(str) + 1;
    // 尝试分配足够的内存来存储字符串副本
    if (!(copy = (char*)cJSON_malloc(len))) return 0;
    // 复制原字符串到新分配的内存中
    memcpy(copy,str,len);
    // 返回新字符串的指针
    return copy;
}

/**
 * cJSON_InitHooks initializes memory operation hooks.
 * 
 * This function allows the user to customize memory allocation and deallocation functions.
 * If a custom hook is not provided, it defaults to the standard malloc and free functions.
 * 
 * @param hooks Pointer to the cJSON_Hooks structure containing custom memory operation functions.
 *              If NULL, it resets to the default memory operations.
 */
void cJSON_InitHooks(cJSON_Hooks* hooks)
{
    // Check if the hooks parameter is NULL
    if (!hooks) { /* Reset hooks */
        cJSON_malloc = malloc;
        cJSON_free = free;
        return;
    }

    // Set custom memory allocation and deallocation functions, default to standard functions if not provided
    cJSON_malloc = (hooks->malloc_fn)?hooks->malloc_fn:malloc;
    cJSON_free = (hooks->free_fn)?hooks->free_fn:free;
}

/* Internal constructor. */
/**
 * 创建一个新的cJSON节点并初始化
 * 
 * 该函数分配了一个cJSON结构体的内存，并将其所有成员初始化为0
 * 这是为了确保新创建的节点处于一个已知的、一致的状态
 * 
 * @return 返回一个新的cJSON节点指针，如果内存分配失败则返回NULL
 */
static cJSON *cJSON_New_Item(void)
{
    // 分配一个cJSON结构体的内存
    cJSON* node = (cJSON*)cJSON_malloc(sizeof(cJSON));
    // 如果内存分配成功，则将节点的所有成员初始化为0
    if (node) memset(node,0,sizeof(cJSON));
    // 返回新创建的节点指针，或在内存分配失败时返回NULL
    return node;
}

/* Delete a cJSON structure. */
/**
 * 释放cJSON结构体及其所有子结构体的内存。
 * 此函数通过递归遍历cJSON结构体及其子结构体，并释放与它们相关的内存。
 * 对于每个结构体，它会检查是否是引用类型，如果不是，则释放其子结构体、字符串和值。
 * 最后，释放结构体本身的内存。
 *
 * @param c 指向cJSON结构体的指针，表示要删除的JSON树的根节点。
 */
void cJSON_Delete(cJSON *c)
{
    cJSON *next;
    while (c)
    {
        next = c->next;
        // 如果当前节点不是引用类型并且有子节点，则递归删除子节点
        if (!(c->type & cJSON_IsReference) && c->child) cJSON_Delete(c->child);
        // 如果当前节点不是引用类型并且有值字符串，则释放值字符串的内存
        if (!(c->type & cJSON_IsReference) && c->valuestring) cJSON_free(c->valuestring);
        // 如果当前节点的字符串不是常量，则释放字符串的内存
        if (!(c->type & cJSON_StringIsConst) && c->string) cJSON_free(c->string);
        // 释放当前节点的内存
        cJSON_free(c);
        c = next;
    }
}

/* Parse the input text to generate a number, and populate the result into item. */
/**
 * 解析字符串中的数字，并将其存储在指定的cJSON项中。
 * 
 * @param item cJSON项，用于存储解析后的数字。
 * @param num 包含数字的字符串，解析将从这个字符串的当前位置开始。
 * @return 返回解析结束后字符串的当前位置。
 * 
 * 此函数支持解析整数部分、小数部分以及指数部分，并处理各自的正负号。
 * 它将解析得到的数字以double和int两种形式存储在cJSON项中，并将cJSON项的类型设置为cJSON_Number。
 */
static const char *parse_number(cJSON *item,const char *num)
{
    double n=0,sign=1,scale=0;int subscale=0,signsubscale=1;

    // 检查数字是否有符号位
    if (*num=='-') sign=-1,num++;    
    // 检查是否以0开头的数字
    if (*num=='0') num++;            
    // 解析整数部分
    if (*num>='1' && *num<='9') do n=(n*10.0)+(*num++ -'0'); while (*num>='0' && *num<='9');    
    // 解析小数部分
    if (*num=='.' && num[1]>='0' && num[1]<='9') {num++; do n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');}    
    // 解析指数部分
    if (*num=='e' || *num=='E')    
    {   
        num++; 
        if (*num=='+') num++; 
        else if (*num=='-') signsubscale=-1,num++;        
        while (*num>='0' && *num<='9') subscale=(subscale*10)+(*num++ - '0');    
    }

    // 计算最终的数字值
    n=sign*n*pow(10.0,(scale+subscale*signsubscale));    
    // 将解析的数字存储到cJSON项中
    item->valuedouble=n;
    item->valueint=(int)n;
    item->type=cJSON_Number;
    return num;
}

/**
 * 计算大于给定数字的最小2的幂。
 * 该函数用于找到大于指定数字x的最小2的幂。
 * 在数据结构大小需要是2的幂的情况下特别有用，例如在哈希表或二叉树的实现中。
 *
 * @param x 输入数字，用于查找大于此数字的最小2的幂。
 * @return 返回大于输入数字的最小2的幂。
 */
static int pow2gt (int x)	{
	// 对x进行预减1操作，以确保最终结果大于输入值。
	--x;
	// 通过多个步骤执行位操作，将x的最高位之后的所有位设置为1。
	x |= x >> 1; // 将次高位设置为1。
	x |= x >> 2; // 将第三和第四高位设置为1。
	x |= x >> 4; // 将第五到第八高位设置为1。
	x |= x >> 8; // 将第九到第十六高位设置为1。
	x |= x >> 16; // 将第十七到第三十二高位设置为1。
	// 返回结果加1，在这一点上x+1就是大于输入数字的最小2的幂。
	return x + 1;
}

typedef struct {char *buffer; int length; int offset; } printbuffer;

/**
 * 确保printbuffer有足够的空间来容纳指定大小的数据。
 * 如果当前缓冲区不足以容纳新数据，函数会分配一个新的更大缓冲区，并复制原有数据。
 * 
 * @param p printbuffer的指针，用于管理缓冲区。
 * @param needed 需要的额外空间大小。
 * @return 返回指向缓冲区中可写区域的指针，或者在失败时返回NULL。
 */
static char* ensure(printbuffer *p,int needed)
{
    char *newbuffer; // 用于存储新分配的缓冲区指针
    int newsize;     // 新缓冲区的大小
    
    // 检查传入的printbuffer及其缓冲区是否有效
    if (!p || !p->buffer) return 0;
    
    // 计算需要的总空间量
    needed+=p->offset;
    
    // 如果当前缓冲区足够大，直接返回当前缓冲区的指针
    if (needed<=p->length) return p->buffer+p->offset;

    // 计算新缓冲区的大小，确保它足够大以容纳所需数据
    newsize=pow2gt(needed);
    
    // 分配新的缓冲区
    newbuffer=(char*)cJSON_malloc(newsize);
    
    // 如果新缓冲区分配失败，释放当前缓冲区并返回NULL
    if (!newbuffer) {cJSON_free(p->buffer);p->length=0,p->buffer=0;return 0;}
    
    // 如果新缓冲区分配成功，将原有缓冲区的数据复制到新缓冲区
    if (newbuffer) memcpy(newbuffer,p->buffer,p->length);
    
    // 释放旧的缓冲区
    cJSON_free(p->buffer);
    
    // 更新printbuffer的缓冲区指针和长度
    p->length=newsize;
    p->buffer=newbuffer;
    
    // 返回新缓冲区中可写区域的指针
    return newbuffer+p->offset;
}

/**
 * 更新打印缓冲区的偏移量并返回新的偏移量。
 * 
 * 该函数的目的是计算在当前缓冲区中添加一定长度的字符串后新的偏移量。
 * 它通过将当前偏移量与新字符串的长度相加来实现这一点。
 * 
 * @param p 指向打印缓冲区的指针，包含缓冲区和当前偏移量信息。
 * @return 返回更新后的偏移量，表示在缓冲区中添加新字符串后的位置。
 */
static int update(printbuffer *p)
{
    // 定义一个字符指针，用于指向缓冲区中的当前位置。
    char *str;
    
    // 检查传入的打印缓冲区指针及其缓冲区是否有效。
    if (!p || !p->buffer) return 0;
    
    // 计算并更新指针位置，使其指向缓冲区中当前偏移量之后的位置。
    str=p->buffer+p->offset;
    
    // 返回更新后的偏移量，即当前偏移量加上从当前位置到缓冲区末尾的长度。
    return p->offset+strlen(str);
}

/* Render the number nicely from the given item into a string. */
/**
 * 将 cJSON 项中的数字转换为字符串表示形式。
 * 
 * 该函数根据 cJSON 项中的数值，将其转换为适当格式的字符串。
 * 它会根据数值的大小和范围，选择最适合的表示方式。
 * 
 * @param item cJSON 项，包含要打印的数字值。
 * @param p 用于输出的 printbuffer，如果为 NULL，则分配新的字符串。
 * @return 返回一个指向表示数字的字符串的指针。
 */
static char *print_number(cJSON *item,printbuffer *p)
{
    char *str=0;
    double d=item->valuedouble;
    
    // 处理数值为 0 的特殊情况
    if (d==0)
    {
        if (p)	str=ensure(p,2);
        else	str=(char*)cJSON_malloc(2);	/* special case for 0. */
        if (str) strcpy(str,"0");
    }
    // 处理数值在整数范围内的情况
    else if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
    {
        if (p)	str=ensure(p,21);
        else	str=(char*)cJSON_malloc(21);	/* 2^64+1 can be represented in 21 chars. */
        if (str)	sprintf(str,"%d",item->valueint);
    }
    // 处理数值为浮点数的情况
    else
    {
        if (p)	str=ensure(p,64);
        else	str=(char*)cJSON_malloc(64);	/* This is a nice tradeoff. */
        if (str)
        {
            // 根据数值的大小选择合适的格式化方式
            if (fabs(floor(d)-d)<=DBL_EPSILON && fabs(d)<1.0e60)sprintf(str,"%.0f",d);
            else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)			sprintf(str,"%e",d);
            else												sprintf(str,"%f",d);
        }
    }
    return str;
}

/**
 * 解析一个4位十六进制数的字符串表示，并将其转换为对应的无符号整数。
 * 该函数仅处理4位十六进制数，因此字符串应包含恰好4个十六进制字符。
 * 
 * @param str 一个指向表示十六进制数的字符串的指针。
 * @return 成功解析的无符号整数，如果输入字符串包含非十六进制字符，则返回0。
 */
static unsigned parse_hex4(const char *str)
{
    unsigned h=0;
    
    // 处理第一个十六进制字符
    if (*str>='0' && *str<='9') 
        h+=(*str)-'0'; 
    else if (*str>='A' && *str<='F') 
        h+=10+(*str)-'A'; 
    else if (*str>='a' && *str<='f') 
        h+=10+(*str)-'a'; 
    else 
        return 0; // 遇到非十六进制字符，返回0
    h=h<<4;str++; // 左移4位以准备添加下一个十六进制字符的值，并移动到下一个字符

    // 处理第二个十六进制字符
    if (*str>='0' && *str<='9') 
        h+=(*str)-'0'; 
    else if (*str>='A' && *str<='F') 
        h+=10+(*str)-'A'; 
    else if (*str>='a' && *str<='f') 
        h+=10+(*str)-'a'; 
    else 
        return 0; // 遇到非十六进制字符，返回0
    h=h<<4;str++; // 左移4位以准备添加下一个十六进制字符的值，并移动到下一个字符

    // 处理第三个十六进制字符
    if (*str>='0' && *str<='9') 
        h+=(*str)-'0'; 
    else if (*str>='A' && *str<='F') 
        h+=10+(*str)-'A'; 
    else if (*str>='a' && *str<='f') 
        h+=10+(*str)-'a'; 
    else 
        return 0; // 遇到非十六进制字符，返回0
    h=h<<4;str++; // 左移4位以准备添加下一个十六进制字符的值，并移动到下一个字符

    // 处理第四个十六进制字符
    if (*str>='0' && *str<='9') 
        h+=(*str)-'0'; 
    else if (*str>='A' && *str<='F') 
        h+=10+(*str)-'A'; 
    else if (*str>='a' && *str<='f') 
        h+=10+(*str)-'a'; 
    else 
        return 0; // 遇到非十六进制字符，返回0

    return h; // 返回解析后的无符号整数
}

/* Parse the input text into an unescaped cstring, and populate item. */
// Define a static array for the first byte mark of UTF-8 encoding to assist with character encoding identification and conversion.
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
// 解析JSON字符串，并将解析后的字符串存储在item中
static const char *parse_string(cJSON *item,const char *str)
{
    const char *ptr=str+1;char *ptr2;char *out;int len=0;unsigned uc,uc2;
    
    // 检查字符串是否以引号开头，如果不是，则返回错误
    if (*str!='\"') {ep=str;return 0;}	/* not a string! */
    
    // 计算字符串长度，跳过转义的引号
    while (*ptr!='\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++;	/* Skip escaped quotes. */
    
    // 分配存储解析后字符串的空间
    out=(char*)cJSON_malloc(len+1);	/* This is how long we need for the string, roughly. */
    if (!out) return 0;
    
    ptr=str+1;ptr2=out;
    // 解析字符串，处理转义字符和Unicode
    while (*ptr!='\"' && *ptr)
    {
        if (*ptr!='\\') *ptr2++=*ptr++;
        else
        {
            ptr++;
            switch (*ptr)
            {
                case 'b': *ptr2++='\b';	break;
                case 'f': *ptr2++='\f';	break;
                case 'n': *ptr2++='\n';	break;
                case 'r': *ptr2++='\r';	break;
                case 't': *ptr2++='\t';	break;
                case 'u':	 /* transcode utf16 to utf8. */
                    uc=parse_hex4(ptr+1);ptr+=4;	/* get the unicode char. */
                    
                    // 检查Unicode字符是否有效
                    if ((uc>=0xDC00 && uc<=0xDFFF) || uc==0)	break;	/* check for invalid.	*/
                    
                    // 处理UTF16代理对
                    if (uc>=0xD800 && uc<=0xDBFF)	/* UTF16 surrogate pairs.	*/
                    {
                        if (ptr[1]!='\\' || ptr[2]!='u')	break;	/* missing second-half of surrogate.	*/
                        uc2=parse_hex4(ptr+3);ptr+=6;
                        if (uc2<0xDC00 || uc2>0xDFFF)		break;	/* invalid second-half of surrogate.	*/
                        uc=0x10000 + (((uc&0x3FF)<<10) | (uc2&0x3FF));
                    }
                    
                    // 将Unicode字符转换为UTF8编码
                    len=4;if (uc<0x80) len=1;else if (uc<0x800) len=2;else if (uc<0x10000) len=3; ptr2+=len;
                    
                    switch (len) {
                        case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                        case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                        case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                        case 1: *--ptr2 =(uc | firstByteMark[len]);
                    }
                    ptr2+=len;
                    break;
                default:  *ptr2++=*ptr; break;
            }
            ptr++;
        }
    }
    *ptr2=0;
    // 检查字符串是否以引号结尾，如果是，则移动指针到引号后
    if (*ptr=='\"') ptr++;
    item->valuestring=out;
    item->type=cJSON_String;
    return ptr;
}

/* Render the cstring provided to an escaped version that can be printed. */
/**
 * 将字符串进行JSON转义并返回转义后的字符串指针。
 * 
 * 该函数会检查输入字符串中是否包含需要转义的字符，如果包含，则进行相应的转义处理。
 * 转义字符包括控制字符（ASCII值0-31）、双引号(")、反斜杠(\)等。
 * 
 * @param str 需要进行JSON转义的字符串。
 * @param p 用于存储转义后字符串的缓冲区对象。如果不需要缓冲区，则传入NULL。
 * 
 * @return 返回转义后字符串的指针，如果内存分配失败，则返回NULL。
 * 
 * 注意：调用者需要负责释放返回的字符串所占用的内存。
 */
static char *print_string_ptr(const char *str,printbuffer *p)
{
    const char *ptr;char *ptr2,*out;int len=0,flag=0;unsigned char token;
    
    // 遍历字符串，检查是否需要进行转义
    for (ptr=str;*ptr;ptr++) flag|=((*ptr>0 && *ptr<32)||(*ptr=='\"')||(*ptr=='\\'))?1:0;
    if (!flag)
    {
        len=ptr-str;
        if (p) out=ensure(p,len+3);
        else    out=(char*)cJSON_malloc(len+3);
        if (!out) return 0;
        ptr2=out;*ptr2++='\"';
        strcpy(ptr2,str);
        ptr2[len]='\"';
        ptr2[len+1]=0;
        return out;
    }
    
    if (!str)
    {
        if (p)  out=ensure(p,3);
        else    out=(char*)cJSON_malloc(3);
        if (!out) return 0;
        strcpy(out,"\"\"");
        return out;
    }
    ptr=str;while ((token=*ptr) && ++len) {if (strchr("\"\\\b\f\n\r\t",token)) len++; else if (token<32) len+=5;ptr++;}
    
    if (p)  out=ensure(p,len+3);
    else    out=(char*)cJSON_malloc(len+3);
    if (!out) return 0;

    ptr2=out;ptr=str;
    *ptr2++='\"';
    while (*ptr)
    {
        if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\') *ptr2++=*ptr++;
        else
        {
            *ptr2++='\\';
            switch (token=*ptr++)
            {
                case '\\':  *ptr2++='\\';  break;
                case '\"':  *ptr2++='\"';  break;
                case '\b':  *ptr2++='b';  break;
                case '\f':  *ptr2++='f';  break;
                case '\n':  *ptr2++='n';  break;
                case '\r':  *ptr2++='r';  break;
                case '\t':  *ptr2++='t';  break;
                default: sprintf(ptr2,"u%04x",token);ptr2+=5;  break;   /* escape and print */
            }
        }
    }
    *ptr2++='\"';*ptr2++=0;
    return out;
}
/* Invote print_string_ptr (which is useful) on an item. */
/**
 * 静态函数：print_string
 * 
 * @param item cJSON结构体指针，用于存储JSON数据
 * @param p printbuffer结构体指针，用于缓冲打印输出
 * 
 * @return 返回一个字符指针，指向打印的字符串
 * 
 * 此函数的作用是将cJSON结构体中的字符串值进行打印
 * 它通过调用print_string_ptr函数来实现，将item中的valuestring成员作为参数传递
 * 使用静态函数的原因是为了限制该函数的作用域在本文件内，避免其他文件调用
 */
static char *print_string(cJSON *item,printbuffer *p)	{return print_string_ptr(item->valuestring,p);}

/* Predeclare these prototypes. */
static const char *parse_value(cJSON *item,const char *value);
static char *print_value(cJSON *item,int depth,int fmt,printbuffer *p);
static const char *parse_array(cJSON *item,const char *value);
static char *print_array(cJSON *item,int depth,int fmt,printbuffer *p);
static const char *parse_object(cJSON *item,const char *value);
static char *print_object(cJSON *item,int depth,int fmt,printbuffer *p);

/* Utility to jump whitespace and cr/lf */
/**
 * 跳过输入字符串中的所有空白字符
 * 
 * @param in const char *, 输入的字符串指针
 * @return const char *, 返回跳过空白字符后的字符串指针
 *
 * 此函数的目的是在给定的字符串中跳过所有空白字符，直到遇到第一个非空白字符为止
 * 空白字符的定义是 ASCII 值小于或等于 32 的字符，这包括空格、制表符、换行符等
 * 如果输入字符串指针为 NULL 或者字符串为空，则直接返回 NULL
 * 这是一个静态函数，意味着它只能在本文件中被调用
 */
static const char *skip(const char *in) {
    // 循环条件：输入字符串指针不为 NULL，且当前字符不为空，且当前字符为空白字符
    while (in && *in && (unsigned char)*in <= 32) {
        // 移动指针到下一个字符
        in++;
    }
    // 返回跳过空白字符后的字符串指针
    return in;
}

/* Parse an object - create a new root, and populate. */
/**
 * 解析JSON字符串，并返回解析后的cJSON对象。
 * 
 * 本函数通过调用底层的解析函数来解析输入的JSON字符串，并根据解析结果返回一个cJSON对象。
 * 它还提供了选项来要求输入字符串是空终止的，并且可以返回解析结束的位置。
 * 
 * @param value 待解析的JSON字符串。
 * @param return_parse_end 如果非空，将通过此参数返回解析结束的位置。
 * @param require_null_terminated 是否要求JSON字符串是空终止的，并且后面没有垃圾数据。
 * @return 解析成功则返回一个cJSON对象，否则返回NULL。
 */
cJSON *cJSON_ParseWithOpts(const char *value,const char **return_parse_end,int require_null_terminated)
{
    // 用于存储解析结束位置的指针
    const char *end=0;
    // 创建一个新的cJSON对象
    cJSON *c=cJSON_New_Item();
    // 初始化全局错误位置指针
    ep=0;
    // 如果创建cJSON对象失败，则返回0
    if (!c) return 0;       /* memory fail */

    // 尝试解析JSON值，并跳过前导空白字符
    end=parse_value(c,skip(value));
    // 如果解析失败，则删除cJSON对象并返回0
    if (!end)	{cJSON_Delete(c);return 0;}	/* parse failure. ep is set. */

    // 如果我们要求空终止的JSON字符串，且没有附加的垃圾数据，则跳过末尾的空白字符并检查空终止符
    if (require_null_terminated) {end=skip(end);if (*end) {cJSON_Delete(c);ep=end;return 0;}}
    // 如果提供了return_parse_end参数，则通过此参数返回解析结束的位置
    if (return_parse_end) *return_parse_end=end;
    // 返回解析成功的cJSON对象
    return c;
}
/* Default options for cJSON_Parse */
/**
 * 解析JSON字符串
 * 该函数是cJSON_ParseWithOpts函数的简化版本，使用默认选项解析JSON字符串
 * 
 * @param value 待解析的JSON字符串
 * @return 解析成功的JSON对象，失败则返回NULL
 */
cJSON *cJSON_Parse(const char *value) {return cJSON_ParseWithOpts(value,0,0);}

/* Render a cJSON item/entity/structure to text. */
/**
 * cJSON_Print函数是一个用于打印JSON对象的函数。
 * 它接受一个指向cJSON结构的指针作为输入，该结构代表了一个解析后的JSON对象。
 * 函数的目的是将JSON对象转换为一个格式化的字符串表示形式，便于显示或存储。
 * 
 * @param item 一个指向cJSON结构的指针，表示一个已解析的JSON对象。
 *             这个参数是必需的，因为函数需要根据它来生成JSON字符串。
 * 
 * @return 返回一个指向字符数组的指针，该数组包含了格式化后的JSON字符串。
 *         如果输入的item为NULL或在打印过程中遇到错误，函数将返回NULL。
 * 
 * 注意: 这个函数实际上调用了另一个名为print_value的函数来完成实际的打印工作。
 *       传递给print_value的参数表明了打印的一些选项，如是否格式化输出等。
 *       该函数的实现细节（如参数的具体含义和打印逻辑）被封装在print_value函数内部。
 */
char *cJSON_Print(cJSON *item) { return print_value(item, 0, 1, 0); }
/**
 * 打印未格式化的JSON字符串
 * 该函数的主要作用是将cJSON结构体表示的JSON数据转换为一个未格式化的字符串形式
 * 主要用于调试或日志记录等场景，不需要格式化输出以提高可读性
 * 
 * @param item cJSON结构体指针，代表了要打印的JSON数据
 * 
 * @return 返回一个指向动态分配的字符数组的指针，该数组包含了未格式化的JSON字符串
 * 调用者需要负责释放这个内存以避免内存泄漏
 */
char *cJSON_PrintUnformatted(cJSON *item)	{return print_value(item,0,0,0);}

/**
 * cJSON_PrintBuffered函数用于将JSON对象转换为格式化的字符串表示形式。
 * 该函数通过预分配一个缓冲区来优化内存使用和性能。
 * 
 * @param item 指向cJSON结构体的指针，表示一个JSON对象。
 * @param prebuffer 预分配缓冲区的大小，用于存储格式化后的JSON字符串。
 * @param fmt 格式化标志，非零表示格式化输出，零表示紧凑输出。
 * 
 * @return 返回指向格式化后JSON字符串的指针，需要调用者释放内存。
 */
char *cJSON_PrintBuffered(cJSON *item,int prebuffer,int fmt)
{
    // 创建一个printbuffer结构体实例用于存储格式化过程中的JSON字符串
    printbuffer p;
    
    // 预分配缓冲区内存
    p.buffer=(char*)cJSON_malloc(prebuffer);
    p.length=prebuffer;
    p.offset=0;
    
    // 调用print_value函数将JSON对象转换为格式化的字符串
    // 注意：此处的return语句提前返回了函数，后面的return p.buffer;将不会被执行
    return print_value(item,0,fmt,&p);
    
    // 由于上面的return语句，这行代码实际上是不可达的
    return p.buffer;
}


/* Parser core - when encountering text, process appropriately. */
/**
 * 解析JSON值。
 * 
 * 本函数根据输入的字符串value解析JSON值，并将解析结果存储在item中。
 * 它支持解析null、true、false值，字符串、数字、数组和对象的解析则调用相应的解析函数。
 * 
 * @param item cJSON结构体指针，用于存储解析后的JSON值。
 * @param value 包含JSON值的字符串。
 * @return 成功解析后返回解析结束后的字符串指针，失败则返回0。
 */
static const char *parse_value(cJSON *item,const char *value)
{
    // 检查输入字符串是否为空，为空则解析失败
    if (!value)						return 0;	/* Fail on null. */
    
    // 解析null值
    if (!strncmp(value,"null",4))	{ item->type=cJSON_NULL;  return value+4; }
    // 解析false值
    if (!strncmp(value,"false",5))	{ item->type=cJSON_False; return value+5; }
    // 解析true值，并将其内部整数值设为1
    if (!strncmp(value,"true",4))	{ item->type=cJSON_True; item->valueint=1;	return value+4; }
    
    // 解析字符串，以"开头
    if (*value=='\"')				{ return parse_string(item,value); }
    // 解析数字，包括负数和正数
    if (*value=='-' || (*value>='0' && *value<='9'))	{ return parse_number(item,value); }
    // 解析数组，以[开头
    if (*value=='[')				{ return parse_array(item,value); }
    // 解析对象，以{开头
    if (*value=='{')				{ return parse_object(item,value); }

    // 如果以上条件都不满足，解析失败，返回原始字符串指针
    ep=value;return 0;	/* failure. */
}

/* Render a value to text. */
/**
 * 根据 cJSON 结构体生成对应的字符串表示。
 * 该函数主要用于 cJSON 数据结构的序列化，将其转换为 JSON 格式的字符串。
 * 
 * @param item cJSON 结构体指针，表示要打印的 JSON 数据。
 * @param depth 用于格式化输出的缩进深度，本函数中未使用。
 * @param fmt 表示是否格式化输出，本函数中未使用。
 * @param p printbuffer 结构体指针，用于高效地处理字符串输出。
 * @return 返回生成的字符串表示的指针，如果失败则返回 NULL。
 * 
 * 注意：该函数是 cJSON 库的一部分，用于内部数据结构的序列化。
 */
static char *print_value(cJSON *item,int depth,int fmt,printbuffer *p)
{
    char *out=0;
    if (!item) return 0;
    if (p)
    {
        switch ((item->type)&255)
        {
            case cJSON_NULL:	{out=ensure(p,5);	if (out) strcpy(out,"null");	break;}
            case cJSON_False:	{out=ensure(p,6);	if (out) strcpy(out,"false");	break;}
            case cJSON_True:	{out=ensure(p,5);	if (out) strcpy(out,"true");	break;}
            case cJSON_Number:	out=print_number(item,p);break;
            case cJSON_String:	out=print_string(item,p);break;
            case cJSON_Array:	out=print_array(item,depth,fmt,p);break;
            case cJSON_Object:	out=print_object(item,depth,fmt,p);break;
        }
    }
    else
    {
        switch ((item->type)&255)
        {
            case cJSON_NULL:	out=cJSON_strdup("null");	break;
            case cJSON_False:	out=cJSON_strdup("false");break;
            case cJSON_True:	out=cJSON_strdup("true"); break;
            case cJSON_Number:	out=print_number(item,0);break;
            case cJSON_String:	out=print_string(item,0);break;
            case cJSON_Array:	out=print_array(item,depth,fmt,0);break;
            case cJSON_Object:	out=print_object(item,depth,fmt,0);break;
        }
    }
    return out;
}

/* Build an array from input text. */
// 解析JSON数组
static const char *parse_array(cJSON *item,const char *value)
{
    cJSON *child;
    
    // 检查是否以'['开始，如果不是，则不是数组
    if (*value!='[')	{ep=value;return 0;}
    
    // 设置项类型为数组
    item->type=cJSON_Array;
    value=skip(value+1);
    
    // 检查是否为空数组
    if (*value==']') return value+1;
    
    // 创建数组的第一个元素
    item->child=child=cJSON_New_Item();
    if (!item->child) return 0;  // 内存分配失败
    
    // 解析第一个元素的值，并跳过任何空格
    value=skip(parse_value(child,skip(value)));	
    if (!value) return 0;  // 解析失败

    // 继续解析数组中的其他元素
    while (*value==',')
    {
        cJSON *new_item;
        
        // 创建新数组元素
        if (!(new_item=cJSON_New_Item())) return 0; 
        child->next=new_item;new_item->prev=child;child=new_item;
        
        // 解析新元素的值，并跳过任何空格
        value=skip(parse_value(child,skip(value+1)));
        if (!value) return 0;  // 解析失败
    }

    // 检查数组结束标志
    if (*value==']') return value+1;
    ep=value;return 0;  // 数组格式错误
}

/* Render an array to text */
/**
 * 打印 cJSON 数组的内容为字符串。
 * 
 * 该函数遍历 cJSON 数组中的每个元素，将其转换为字符串，并将这些字符串连接成一个输出字符串。
 * 它处理了直接打印到缓冲区和动态分配内存以生成输出字符串两种情况。
 * 
 * @param item 指向表示要打印的数组的 cJSON 对象的指针。
 * @param depth 数组的深度，用于格式化目的，但在本函数中未使用，保持与其他打印函数的一致性。
 * @param fmt 标志，指示是否格式化（添加空格和换行）输出字符串。
 * @param p 指向 printbuffer 对象的指针，用于直接打印到缓冲区，或 NULL 表示使用动态内存分配。
 * @return 返回指向输出字符串的指针，或 NULL 表示失败。
 */
static char *print_array(cJSON *item, int depth, int fmt, printbuffer *p)
{
    char **entries;
    char *out = 0, *ptr, *ret;
    int len = 5;
    cJSON *child = item->child;
    int numentries = 0, i = 0, fail = 0;
    size_t tmplen = 0;

    /* 计算数组中的条目数量 */
    while (child) numentries++, child = child->next;

    /* 显式处理 numentries == 0 的情况 */
    if (!numentries)
    {
        if (p) out = ensure(p, 3);
        else out = (char*)cJSON_malloc(3);
        if (out) strcpy(out, "[]");
        return out;
    }

    if (p)
    {
        /* 组合输出数组 */
        i = p->offset;
        ptr = ensure(p, 1); if (!ptr) return 0; *ptr = '['; p->offset++;
        child = item->child;
        while (child && !fail)
        {
            print_value(child, depth + 1, fmt, p);
            p->offset = update(p);
            if (child->next) { len = fmt ? 2 : 1; ptr = ensure(p, len + 1); if (!ptr) return 0; *ptr++ = ','; if (fmt) *ptr++ = ' '; *ptr = 0; p->offset += len; }
            child = child->next;
        }
        ptr = ensure(p, 2); if (!ptr) return 0; *ptr++ = ']'; *ptr = 0;
        out = (p->buffer) + i;
    }
    else
    {
        /* 分配一个数组来保存每个值 */
        entries = (char**)cJSON_malloc(numentries * sizeof(char*));
        if (!entries) return 0;
        memset(entries, 0, numentries * sizeof(char*));

        /* 获取所有结果 */
        child = item->child;
        while (child && !fail)
        {
            ret = print_value(child, depth + 1, fmt, 0);
            entries[i++] = ret;
            if (ret) len += strlen(ret) + 2 + (fmt ? 1 : 0); else fail = 1;
            child = child->next;
        }

        /* 如果没有失败，尝试分配输出字符串的内存 */
        if (!fail) out = (char*)cJSON_malloc(len);

        /* 如果分配失败，我们失败 */
        if (!out) fail = 1;

        /* 处理失败情况 */
        if (fail)
        {
            for (i = 0; i < numentries; i++) if (entries[i]) cJSON_free(entries[i]);
            cJSON_free(entries);
            return 0;
        }

        /* 组合输出数组 */
        *out = '[';
        ptr = out + 1; *ptr = 0;
        for (i = 0; i < numentries; i++)
        {
            tmplen = strlen(entries[i]); memcpy(ptr, entries[i], tmplen); ptr += tmplen;
            if (i != numentries - 1) { *ptr++ = ','; if (fmt) *ptr++ = ' '; *ptr = 0; }
            cJSON_free(entries[i]);
        }
        cJSON_free(entries);
        *ptr++ = ']'; *ptr++ = 0;
    }
    return out;
}

/* Build an object from the text. */
// 解析JSON对象的函数
// 参数item是一个指向cJSON结构的指针，用于存储解析后的JSON对象
// 参数value是一个指向JSON对象字符串表示的常量字符指针
// 返回值是指向JSON对象结束后的字符指针，如果解析失败则返回0
static const char *parse_object(cJSON *item,const char *value)
{
    cJSON *child;
    
    // 检查是否以'{'开始，如果不是，则不是JSON对象
    if (*value!='{')	{ep=value;return 0;}	/* not an object! */
    
    // 设置项的类型为JSON对象
    item->type=cJSON_Object;
    
    // 跳过开头的'{'
    value=skip(value+1);
    
    // 如果遇到'}'，则表示是空对象
    if (*value=='}') return value+1;	/* empty array. */
    
    // 创建第一个子项
    item->child=child=cJSON_New_Item();
    
    // 如果创建子项失败，返回0
    if (!item->child) return 0;
    
    // 解析子项的字符串，跳过任何空格
    value=skip(parse_string(child,skip(value)));
    
    // 如果解析失败，返回0
    if (!value) return 0;
    
    // 设置子项的字符串和值字符串
    child->string=child->valuestring;child->valuestring=0;
    
    // 如果不是':'，则解析失败
    if (*value!=':') {ep=value;return 0;}	/* fail! */
    
    // 跳过任何空格，解析子项的值
    value=skip(parse_value(child,skip(value+1)));	
    
    // 如果解析失败，返回0
    if (!value) return 0;
    
    // 继续解析后续的子项
    while (*value==',')
    {
        cJSON *new_item;
        
        // 创建新的子项，如果创建失败，返回0
        if (!(new_item=cJSON_New_Item()))	return 0; /* memory fail */
        
        // 将新子项链接到子项列表
        child->next=new_item;new_item->prev=child;child=new_item;
        
        // 解析新子项的字符串，跳过任何空格
        value=skip(parse_string(child,skip(value+1)));
        
        // 如果解析失败，返回0
        if (!value) return 0;
        
        // 设置子项的字符串和值字符串
        child->string=child->valuestring;child->valuestring=0;
        
        // 如果不是':'，则解析失败
        if (*value!=':') {ep=value;return 0;}	/* fail! */
        
        // 跳过任何空格，解析新子项的值
        value=skip(parse_value(child,skip(value+1)));	
        
        // 如果解析失败，返回0
        if (!value) return 0;
    }
    
    // 如果遇到'}'，表示对象解析结束
    if (*value=='}') return value+1;	/* end of array */
    
    // 如果解析到这一步出现问题，返回0
    ep=value;return 0;	/* malformed. */
}

/* Render an object to text. */
/**
 * 将 cJSON 对象打印为 JSON 字符串。
 *
 * 该函数接收一个 cJSON 对象，并将其转换为格式化的 JSON 字符串。支持缩进的美化输出。
 *
 * @param item 指向要打印的 cJSON 对象的指针。
 * @param depth 当前嵌套深度，用于缩进控制。
 * @param fmt 是否启用美化输出。非零值表示启用缩进和换行。
 * @param p 指向 printbuffer 结构的指针，用于缓冲区管理。
 * 
 * @return 返回生成的 JSON 字符串，失败时返回 NULL。
 */

static char *print_object(cJSON *item, int depth, int fmt, printbuffer *p)
{
    char **entries = 0, **names = 0;
    char *out = 0, *ptr, *ret, *str;
    int len = 7, i = 0, j;
    cJSON *child = item->child;
    int numentries = 0, fail = 0;
    size_t tmplen = 0;

    /* 统计对象中的条目数量 */
    while (child) {
        numentries++;
        child = child->next;
    }

    /* 处理空对象的情况 */
    if (!numentries) {
        if (p) {
            out = ensure(p, fmt ? depth + 4 : 3);
        } else {
            out = (char *)cJSON_malloc(fmt ? depth + 4 : 3);
        }
        if (!out) return 0;

        ptr = out;
        *ptr++ = '{';
        if (fmt) {
            *ptr++ = '\n';
            for (i = 0; i < depth - 1; i++) *ptr++ = '\t';
        }
        *ptr++ = '}';
        *ptr++ = 0;
        return out;
    }

    if (p) {
        /* 构建输出字符串 */
        i = p->offset;
        len = fmt ? 2 : 1;
        ptr = ensure(p, len + 1);
        if (!ptr) return 0;

        *ptr++ = '{';
        if (fmt) *ptr++ = '\n';
        *ptr = 0;
        p->offset += len;

        child = item->child;
        depth++;

        while (child) {
            if (fmt) {
                ptr = ensure(p, depth);
                if (!ptr) return 0;
                for (j = 0; j < depth; j++) *ptr++ = '\t';
                p->offset += depth;
            }

            print_string_ptr(child->string, p);
            p->offset = update(p);

            len = fmt ? 2 : 1;
            ptr = ensure(p, len);
            if (!ptr) return 0;
            *ptr++ = ':';
            if (fmt) *ptr++ = '\t';
            p->offset += len;

            print_value(child, depth, fmt, p);
            p->offset = update(p);

            len = (fmt ? 1 : 0) + (child->next ? 1 : 0);
            ptr = ensure(p, len + 1);
            if (!ptr) return 0;
            if (child->next) *ptr++ = ',';
            if (fmt) *ptr++ = '\n';
            *ptr = 0;
            p->offset += len;

            child = child->next;
        }

        ptr = ensure(p, fmt ? (depth + 1) : 2);
        if (!ptr) return 0;
        if (fmt) {
            for (i = 0; i < depth - 1; i++) *ptr++ = '\t';
        }
        *ptr++ = '}';
        *ptr = 0;
        out = (p->buffer) + i;
    } else {
        /* 分配空间用于存储名称和对象 */
        entries = (char **)cJSON_malloc(numentries * sizeof(char *));
        if (!entries) return 0;
        names = (char **)cJSON_malloc(numentries * sizeof(char *));
        if (!names) {
            cJSON_free(entries);
            return 0;
        }
        memset(entries, 0, sizeof(char *) * numentries);
        memset(names, 0, sizeof(char *) * numentries);

        /* 收集所有结果到数组中 */
        child = item->child;
        depth++;
        if (fmt) len += depth;

        while (child) {
            names[i] = str = print_string_ptr(child->string, 0);
            entries[i++] = ret = print_value(child, depth, fmt, 0);
            if (str && ret) {
                len += strlen(ret) + strlen(str) + 2 + (fmt ? 2 + depth : 0);
            } else {
                fail = 1;
            }
            child = child->next;
        }

        /* 尝试分配输出字符串 */
        if (!fail) {
            out = (char *)cJSON_malloc(len);
        }
        if (!out) fail = 1;

        /* 处理分配失败 */
        if (fail) {
            for (i = 0; i < numentries; i++) {
                if (names[i]) cJSON_free(names[i]);
                if (entries[i]) cJSON_free(entries[i]);
            }
            cJSON_free(names);
            cJSON_free(entries);
            return 0;
        }

        /* 构建输出字符串 */
        *out = '{';
        ptr = out + 1;
        if (fmt) *ptr++ = '\n';
        *ptr = 0;

        for (i = 0; i < numentries; i++) {
            if (fmt) {
                for (j = 0; j < depth; j++) *ptr++ = '\t';
            }
            tmplen = strlen(names[i]);
            memcpy(ptr, names[i], tmplen);
            ptr += tmplen;
            *ptr++ = ':';
            if (fmt) *ptr++ = '\t';
            strcpy(ptr, entries[i]);
            ptr += strlen(entries[i]);

            if (i != numentries - 1) *ptr++ = ',';
            if (fmt) *ptr++ = '\n';
            *ptr = 0;

            cJSON_free(names[i]);
            cJSON_free(entries[i]);
        }

        cJSON_free(names);
        cJSON_free(entries);

        if (fmt) {
            for (i = 0; i < depth - 1; i++) *ptr++ = '\t';
        }
        *ptr++ = '}';
        *ptr++ = 0;
    }

    return out;
}

/* Get Array size/item / object item. */
/**
 * 获取JSON数组的大小
 * 
 * @param array 指向JSON数组的指针
 * @return 数组中元素的数量
 * 
 * 此函数通过遍历JSON数组的链表来计算数组的大小
 * 它从数组的第一个子节点开始，通过循环遍历每个节点，直到达到链表的末尾
 * 每遍历一个节点，计数器增加1，最终返回计数器的值作为数组的大小
 */
int cJSON_GetArraySize(cJSON *array) {
    cJSON *c = array->child; // 指向数组的第一个子节点
    int i = 0; // 初始化计数器
    while (c) { // 遍历链表直到达到末尾
        i++; // 计数器增加1
        c = c->next; // 移动到下一个节点
    }
    return i; // 返回数组的大小
}
/**
 * 获取JSON数组中的指定元素
 * 
 * @param array cJSON * 类型的指针，代表JSON数组
 * @param item int 类型，代表要获取的数组元素的索引
 * 
 * @return cJSON * 类型的指针，指向数组中指定索引的元素，如果不存在则返回NULL
 * 
 * 此函数通过遍历数组的链表结构来查找指定索引的元素它首先指向数组的第一个子元素，
 * 然后根据索引值逐个遍历直到找到指定的元素或者遍历到数组末尾如果索引值大于数组长度，
 * 则返回NULL这个函数的设计充分利用了cJSON的链表结构，避免了数组的随机访问
 */
cJSON *cJSON_GetArrayItem(cJSON *array,int item) {
    cJSON *c = array->child;  // 指向数组的第一个子元素
    while (c && item > 0) {
        item--;
        c = c->next;  // 遍历到下一个元素
    }
    return c;  // 返回指定索引的元素，如果不存在则为NULL
}
/**
 * 获取 JSON 对象中指定名称的项
 * 
 * @param object JSON 对象指针，表示一个 JSON 对象
 * @param string 需要获取的 JSON 项的名称
 * @return 返回指向具有指定名称的 JSON 项的指针，如果未找到则返回 NULL
 *
 * 此函数通过遍历 JSON 对象的子项来查找具有指定名称的项
 * 它使用 strcasecmp 函数来比较项的名称和指定的字符串，以实现不区分大小写的比较
 * 当找到匹配的项时，返回该项的指针；如果遍历完所有子项都没有找到匹配的项，则返回 NULL
 */
cJSON *cJSON_GetObjectItem(cJSON *object,const char *string)
{
    cJSON *c = object->child;
    while (c && cJSON_strcasecmp(c->string, string))
        c = c->next;
    return c;
}

/* Utility for array list handling. */
/**
 * 将一个 cJSON 对象添加到另一个 cJSON 对象之后
 * 
 * @param prev 指向 cJSON 对象的指针，该对象将位于新对象之前
 * @param item 指向要添加的 cJSON 对象的指针
 * 
 * 此函数通过修改 prev 对象的 next 指针和 item 对象的 prev 指针来链接两个 cJSON 对象
 * 它不处理内存分配或释放，调用者需要负责管理 cJSON 对象的生命周期
 */
static void suffix_object(cJSON *prev, cJSON *item) {
    prev->next = item;
    item->prev = prev;
}
/* Utility for handling references. */
/**
 * 创建一个 cJSON 项的引用。
 * 
 * 本函数用于生成给定 cJSON 项的引用，而非复制。引用项将共享原始项的数据，
 * 但作为单独的 cJSON 结构体实例存在。这在需要多个地方引用同一数据时非常有用。
 * 
 * @param item 指向要引用的 cJSON 项。
 * @return 返回指向新创建的引用项的指针，如果内存分配失败则返回 NULL。
 * 
 * 注意：引用项的类型标志中会设置 cJSON_IsReference 位，以标识这是一个引用。
 *       引用项的 string 成员被设置为 NULL，因为引用项不直接拥有键名。
 *       引用项的 next 和 prev 成员被设置为 NULL，因为它不参与原始项的链表结构。
 */
static cJSON *create_reference(cJSON *item) {
    // 创建一个新的 cJSON 项作为引用
    cJSON *ref = cJSON_New_Item();
    // 如果内存分配失败，直接返回 NULL
    if (!ref) return 0;
    // 复制原始 cJSON 项的数据到引用项
    memcpy(ref, item, sizeof(cJSON));
    // 引用项不直接拥有键名，因此将其 string 成员设置为 NULL
    ref->string = 0;
    // 设置引用项的类型标志，标识这是一个引用
    ref->type |= cJSON_IsReference;
    // 引用项不参与原始项的链表结构，因此将其 next 和 prev 成员设置为 NULL
    ref->next = ref->prev = 0;
    // 返回新创建的引用项的指针
    return ref;
}

/* Add item to array/object. */
/**
 * 向JSON数组中添加一个JSON项
 * 
 * @param array JSON数组的指针，表示要添加项的数组
 * @param item 要添加到数组中的JSON项的指针
 * 
 * 此函数将一个JSON项添加到指定的JSON数组的末尾如果数组为空，则直接将该项设为数组的首个子项
 * 如果数组不为空，则遍历数组直到找到最后一个子项，并将该项添加为最后一个子项的后继
 * 
 * 注意：如果传入的item为NULL，函数将直接返回，不进行任何操作
 *       如果传入的array为NULL，将导致未定义行为，因为函数不检查array参数是否为NULL
 */
void cJSON_AddItemToArray(cJSON *array, cJSON *item) {
    // 检查传入的item是否为NULL，如果是，则直接返回
    if (!item) return;
    
    // 获取数组的首个子项
    cJSON *c = array->child;
    
    // 如果数组为空，则将item设为数组的首个子项
    if (!c) {
        array->child = item;
    } else {
        // 遍历数组，直到找到最后一个子项
        while (c && c->next) {
            c = c->next;
        }
        // 将item添加为最后一个子项的后继
        suffix_object(c, item);
    }
}
/**
 * 向JSON对象中添加一个JSON项
 * 
 * @param object 指向JSON对象的指针，表示要添加项的目标对象
 * @param string 键的名称，用于在对象中标识添加的项
 * @param item 指向要添加到对象的JSON项的指针
 * 
 * 此函数首先检查要添加的项（item）是否为NULL，如果为NULL，则直接返回，不进行任何操作
 * 如果要添加的项（item）的string成员不为NULL，则释放原有的string成员，以避免内存泄漏
 * 然后，将传入的键名称（string）复制并分配给item的string成员，以便在对象中标识该项
 * 最后，调用cJSON_AddItemToArray函数将item添加到object的子项列表中
 * 
 * 注意：这项操作会修改传入的item，因此在调用此函数后，item的拥有权可以视为已转移给object
 */
void cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item) {
    // 检查传入的item是否为NULL，如果为NULL，则直接返回
    if (!item) return;
    
    // 如果item的string成员不为NULL，释放原有的string成员，避免内存泄漏
    if (item->string) cJSON_free(item->string);
    
    // 复制传入的键名称并分配给item的string成员
    item->string=cJSON_strdup(string);
    
    // 调用函数将item添加到object的子项列表中
    cJSON_AddItemToArray(object,item);
}
/**
 * 向对象中添加一个带有常量字符串键的项目
 * 
 * @param object 指向 cJSON 对象的指针，表示要添加项目的 JSON 对象
 * @param string 常量字符串键，表示要添加的项目的键名
 * @param item 指向 cJSON 项目的指针，表示要添加到对象中的项目
 * 
 * 此函数首先检查 item 参数是否为 NULL，如果为 NULL，则直接返回，不做任何操作
 * 如果 item 不为 NULL，且 item 的字符串不是常量类型，则释放原有的字符串
 * 然后，将 item 的字符串指针指向传入的常量字符串，并设置 cJSON_StringIsConst 标志
 * 最后，将该项目添加到对象的项目数组中
 * 
 * 注意：该函数假设传入的字符串参数是常量，因此不会在函数内部对其进行释放
 *       如果传入的字符串不是常量，可能会导致内存泄漏
 */
void cJSON_AddItemToObjectCS(cJSON *object,const char *string,cJSON *item) {
    // 检查 item 参数是否为 NULL
    if (!item) return;
    
    // 如果 item 的字符串不是常量类型，则释放原有的字符串
    if (!(item->type&cJSON_StringIsConst) && item->string) cJSON_free(item->string);
    
    // 将 item 的字符串指针指向传入的常量字符串
    item->string=(char*)string;
    
    // 设置 cJSON_StringIsConst 标志
    item->type|=cJSON_StringIsConst;
    
    // 将该项目添加到对象的项目数组中
    cJSON_AddItemToArray(object,item);
}
/**
 * 向cJSON数组中添加项目引用
 * 
 * 本函数通过在数组中添加一个指向已有cJSON项目的引用，来实现项目共享或复用
 * 这对于构建复杂的JSON结构时非常有用，可以避免不必要的数据复制
 * 
 * @param array 指向cJSON数组的指针，该数组将接收新的项目引用
 * @param item 指向要添加到数组的cJSON项目的指针
 * 
 * 注意：这里使用了create_reference函数来创建item的引用，并将其添加到array中
 *       这意味着调用者需要确保item在使用引用期间保持有效，避免内存释放导致的错误
 */
void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item) {
    cJSON_AddItemToArray(array, create_reference(item));
}
/**
 * 向一个cJSON对象中添加一个指向另一个cJSON对象的引用。
 * 这个函数通过创建一个新引用，避免了实际对象的复制，从而节省内存。
 * 
 * @param object 要添加引用的cJSON对象。
 * @param string 引用的名称，作为对象中的键。
 * @param item   指向的cJSON对象，即引用的目标。
 */
void cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item) {
    cJSON_AddItemToObject(object, string, create_reference(item));
}

/**
 * 从JSON数组中分离指定的元素
 * 此函数将从给定的JSON数组中分离出指定索引位置的元素，并返回该元素
 * 分离后，原数组将不再包含该元素，但分离出的元素仍然有效，需要用户负责释放其内存
 * 
 * @param array JSON数组指针，表示我们要从中分离元素的数组
 * @param which 指定要分离的元素的索引位置
 * @return 返回分离出的JSON元素指针，如果指定的索引位置不存在，则返回NULL
 */
cJSON *cJSON_DetachItemFromArray(cJSON *array,int which) {
    // 从数组的首个子元素开始遍历
    cJSON *c = array->child;
    // 遍历数组，寻找指定索引位置的元素
    while (c && which > 0) {
        c = c->next;
        which--;
    }
    // 如果未找到指定索引位置的元素，则返回NULL
    if (!c) return 0;

    // 更新找到的元素的前后指针，将其从数组链表中分离出来
    if (c->prev) c->prev->next = c->next;
    if (c->next) c->next->prev = c->prev;
    // 如果分离的元素是数组的第一个元素，更新数组的子元素指针
    if (c == array->child) array->child = c->next;
    // 将分离出的元素的前后指针置为NULL，表示它已从数组中分离
    c->prev = c->next = 0;
    // 返回分离出的元素
    return c;
}
/**
 * 从JSON数组中删除指定索引的元素。
 * 
 * 该函数通过调用cJSON_DetachItemFromArray函数将指定索引的元素从数组中分离出来，
 * 然后使用cJSON_Delete函数来释放该元素所占用的内存。
 * 
 * @param array cJSON数组指针，表示要从中删除元素的数组。
 * @param which 要删除的元素在数组中的索引。
 * 
 * 注意：该函数修改传入的数组，调用后，指定索引位置的元素将被移除，
 * 并且该元素的内存会被释放。
 */
void cJSON_DeleteItemFromArray(cJSON *array, int which) {
    cJSON_Delete(cJSON_DetachItemFromArray(array, which));
}
/**
 * 从JSON对象中分离出指定名称的项
 * 此函数通过遍历JSON对象的子项，查找与指定名称匹配的项，然后将其从对象中分离出来
 * 如果找到匹配项，它将调用`cJSON_DetachItemFromArray`函数来执行分离操作
 * 
 * @param object JSON对象，即一个包含多个子项的JSON节点
 * @param string 要分离的JSON项的名称
 * @return 如果找到并成功分离指定名称的JSON项，则返回指向该JSON项的指针；否则返回NULL
 */
cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string) {
    // 初始化索引计数器和当前节点指针
    int i=0;
    cJSON *c=object->child;
    
    // 遍历对象的子项，直到找到名称匹配的项
    while (c && cJSON_strcasecmp(c->string,string)) {
        i++;
        c=c->next;
    }
    
    // 如果找到了匹配项，调用函数将其从对象中分离出来
    if (c) return cJSON_DetachItemFromArray(object,i);
    
    // 如果没有找到匹配项，返回NULL
    return 0;
}
/**
 * 从JSON对象中删除指定的项
 * 
 * @param object 指向JSON对象的指针，该对象是被删除项的父对象
 * @param string 表示要从对象中删除的项的名称
 * 
 * 此函数通过调用cJSON_DetachItemFromObject函数将指定的项从对象中分离出来，然后使用cJSON_Delete函数释放该项的内存
 * 它有效地从对象中移除了指定名称的项，确保了内存的正确释放，避免了内存泄漏
 */
void cJSON_DeleteItemFromObject(cJSON *object,const char *string) {
    cJSON_Delete(cJSON_DetachItemFromObject(object,string));
}

/* Replace array/object items with new ones. */
/**
 * 在JSON数组中插入一个新的项。
 * 
 * 本函数旨在将一个新的JSON项插入到指定数组中的特定位置。如果目标位置超出当前数组长度，
 * 则直接在数组末尾添加新项。这项操作会调整数组内部的链表结构，以适应新项的插入。
 * 
 * @param array 指向JSON数组的指针，表示要插入新项的数组。
 * @param which 指定新项应插入的位置。如果该位置超出数组当前长度，新项将被添加到数组末尾。
 * @param newitem 指向要插入的新JSON项的指针。
 * 
 * 注意: 本函数不负责分配或释放newitem的内存，调用者需要自行管理该项的生命周期。
 */
void cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem) {
    // 从数组的第一个子项开始遍历
    cJSON *c = array->child;
    // 遍历数组，直到达到指定的插入位置或数组末尾
    while (c && which > 0) {
        c = c->next;
        which--;
    }
    // 如果达到或超出数组末尾，直接在数组末尾添加新项
    if (!c) {
        cJSON_AddItemToArray(array, newitem);
        return;
    }
    // 将新项插入到链表中，更新链表的连接
    newitem->next = c;
    newitem->prev = c->prev;
    c->prev = newitem;
    // 如果新项插入在数组的起始位置，更新数组的首项指针
    if (c == array->child) {
        array->child = newitem;
    } else {
        // 否则，更新前一项的next指针，以指向新项
        newitem->prev->next = newitem;
    }
}
/**
 * cJSON_ReplaceItemInArray函数用于在cJSON数组中替换一个现有的项。
 * @param array 指向cJSON数组的指针。
 * @param which 指定要替换的项的索引位置。
 * @param newitem 指向将要插入数组的新项的指针。
 * 
 * 此函数首先遍历数组找到指定索引位置的项，然后用新项替换该项。
 * 如果指定索引位置的项不存在（即数组长度小于索引位置），则函数不进行任何操作。
 * 替换操作包括更新新项的前后指针，以及更新被替换项前后项的指针，最后删除被替换的项。
 */
void cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem) {
    // 从数组的第一个子项开始遍历
    cJSON *c = array->child;
    while (c && which > 0) {
        c = c->next;
        which--;
    }
    // 如果找到指定索引位置的项，则进行替换操作
    if (!c) return;

    // 更新新项的next和prev指针
    newitem->next = c->next;
    newitem->prev = c->prev;
    // 如果新项不是数组的最后一项，则更新下一项的prev指针
    if (newitem->next) newitem->next->prev = newitem;
    // 如果被替换的项是数组的第一项，则更新数组的child指针为新项
    if (c == array->child) array->child = newitem;
    // 否则，更新前一项的next指针为新项
    else newitem->prev->next = newitem;
    // 断开被替换项的next和prev指针
    c->next = c->prev = 0;
    // 删除被替换的项
    cJSON_Delete(c);
}
/**
 * cJSON_ReplaceItemInObject函数用于在JSON对象中替换一个子项。
 * 这个函数接受一个JSON对象指针、一个字符串作为键名，以及一个新的JSON项指针，
 * 并用新的JSON项替换对象中与给定键名匹配的子项。
 * 
 * @param object JSON对象的指针，表示要进行替换操作的父对象。
 * @param string 键名字符串，表示要替换的子项的键名。
 * @param newitem 新的JSON项的指针，用于替换原有的子项。
 * 
 * 注意：此函数假设调用者已经创建了newitem，并且负责在不再需要时释放其内存。
 *       如果没有找到匹配的子项，此函数不会进行任何操作。
 */
void cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem) {
    // 初始化索引计数器为0，并获取对象的第一个子项。
    int i = 0;
    cJSON *c = object->child;
    
    // 遍历对象的所有子项，寻找与给定键名匹配的子项。
    while (c && cJSON_strcasecmp(c->string, string)) {
        i++;
        c = c->next;
    }
    
    // 如果找到了匹配的子项，进行替换操作。
    if (c) {
        // 为新项分配新的键名字符串。
        newitem->string = cJSON_strdup(string);
        // 调用函数用新项替换旧项。
        cJSON_ReplaceItemInArray(object, i, newitem);
    }
}

/* Create basic types: */
/**
 * 创建一个表示空值的cJSON对象
 * 
 * 该函数分配内存并初始化一个cJSON对象结构体，将其类型设置为cJSON_NULL，
 * 以表示一个空值。这是JSON数据结构中的一种基本类型，用于表示空或者不存在的值。
 * 
 * @return 返回一个新的表示空值的cJSON对象指针，如果内存分配失败则返回NULL。
 */
cJSON *cJSON_CreateNull(void) {
    cJSON *item = cJSON_New_Item(); // 分配并初始化一个新的cJSON对象
    if (item) item->type = cJSON_NULL; // 如果对象成功创建，则将其类型设置为cJSON_NULL
    return item; // 返回新创建的对象指针
}
/**
 * 创建一个表示布尔值"真"的cJSON项
 * 
 * 该函数首先调用cJSON_New_Item来分配和初始化一个新的cJSON项如果分配成功，
 * 则将该项的类型设置为cJSON_True，表示这是一个布尔值"真"的项
 * 
 * @return 返回一个新的表示布尔值"真"的cJSON项，如果内存分配失败则返回NULL
 */
cJSON *cJSON_CreateTrue(void) {
    cJSON *item = cJSON_New_Item(); // 分配和初始化一个新的cJSON项
    if (item) {
        item->type = cJSON_True; // 设置项的类型为布尔值"真"
    }
    return item; // 返回新的cJSON项或NULL
}
/**
 * 创建一个表示布尔值"false"的cJSON项
 * 
 * 该函数首先调用cJSON_New_Item来创建一个新的cJSON项如果该项成功创建，
 * 则将该项的类型设置为cJSON_False，表示一个布尔值"false"然后返回这个cJSON项
 * 
 * @return 返回一个新的表示布尔值"false"的cJSON项如果内存不足，则返回NULL
 */
cJSON *cJSON_CreateFalse(void) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_False;
    }
    return item;
}
/**
 * 创建一个表示布尔值的 cJSON 对象
 * 
 * @param b 整型变量，非零表示真，零表示假
 * @return 返回一个新的 cJSON 对象，表示布尔值如果无法分配内存，则返回 NULL
 */
cJSON *cJSON_CreateBool(int b) {
    // 分配新的 cJSON 结构体空间
    cJSON *item = cJSON_New_Item();
    // 如果成功分配了空间，则根据参数 b 设置对象类型为真或假
    if (item) {
        item->type = b ? cJSON_True : cJSON_False;
    }
    // 返回新创建的 cJSON 对象，或在失败时返回 NULL
    return item;
}
/**
 * 创建一个表示数字的cJSON项
 * 
 * @param num 要表示的数字值，可以是整数或浮点数
 * 
 * @return 返回一个新的cJSON项，如果内存不足则返回NULL
 * 
 * 此函数首先通过调用cJSON_New_Item()在堆上分配一个新的cJSON项如果分配成功，
 * 则将该项的类型设置为cJSON_Number，表示它是一个数字接着，将传入的double型数字
 * 值赋给该项的valuedouble成员，同时将该数字值转换为整数并赋给valueint成员这样，
 * cJSON项就可以用来表示一个数字值了
 */
cJSON *cJSON_CreateNumber(double num) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_Number;
        item->valuedouble = num;
        item->valueint = (int)num;
    }
    return item;
}
/**
 * 创建一个包含字符串值的cJSON项
 * 
 * @param string 待添加到cJSON项中的字符串值
 * @return 返回新创建的cJSON项指针，如果内存分配失败则返回NULL
 * 
 * 本函数首先调用cJSON_New_Item()来创建一个新的cJSON项如果创建成功，
 * 则将该项的类型设置为cJSON_String，并为其valuestring成员分配内存，
 * 复制输入的字符串到新分配的内存中
 */
cJSON *cJSON_CreateString(const char *string)
{
    cJSON *item = cJSON_New_Item();
    if (item)
    {
        item->type = cJSON_String;
        item->valuestring = cJSON_strdup(string);
    }
    return item;
}
/**
 * 创建一个 cJSON 类型的数组
 * 
 * 返回值: 返回新创建的 cJSON 数组指针，如果内存分配失败则返回 NULL
 * 
 * 该函数首先通过调用 cJSON_New_Item() 创建一个新的 cJSON 项
 * 如果创建成功，则将该项的类型设置为 cJSON_Array，表示它是一个数组
 * 最后返回这个新创建的数组项指针
 */
cJSON *cJSON_CreateArray(void) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_Array;
    }
    return item;
}
/**
 * 创建一个空的JSON对象
 * 
 * 返回值: 返回新创建的JSON对象指针，如果内存分配失败则返回NULL
 */
cJSON *cJSON_CreateObject(void) {
    cJSON *item = cJSON_New_Item(); // 创建一个新的cJSON项
    if (item) item->type = cJSON_Object; // 如果项成功创建，则将其类型设置为JSON对象
    return item; // 返回新创建的JSON对象指针
}

/* Create Arrays: */
/**
 * 创建一个包含整数数组的cJSON对象
 * 
 * @param numbers 一个整数数组，用于填充新创建的cJSON数组
 * @param count 表示numbers数组中元素的数量
 * 
 * @return 返回一个新创建的包含整数数组的cJSON对象，如果内存不足则返回NULL
 * 
 * 此函数首先创建一个空的cJSON数组，然后遍历给定的整数数组，
 * 为每个整数创建一个cJSON数字对象，并将这些数字对象作为子元素添加到数组中
 */
cJSON *cJSON_CreateIntArray(const int *numbers,int count)		{
    // 初始化变量i用于循环计数，n和p用于指向当前和前一个cJSON对象，a用于指向新创建的cJSON数组
    int i;
    cJSON *n=0,*p=0,*a=cJSON_CreateArray();
    
    // 遍历整数数组，为每个整数创建一个cJSON数字对象，并将其添加到cJSON数组中
    for(i=0;a && i<count;i++){
        n=cJSON_CreateNumber(numbers[i]);
        // 如果是第一个元素，则将其设置为数组的子元素，否则将其作为前一个元素的兄弟元素添加
        if(!i)a->child=n;
        else suffix_object(p,n);
        p=n;
    }
    
    // 返回新创建的包含整数数组的cJSON对象，如果内存不足则返回NULL
    return a;
}
/**
 * 创建一个包含浮点数数组的cJSON对象
 * 
 * @param numbers 一个浮点数数组，包含要添加到JSON数组中的数字
 * @param count 表示numbers数组中元素的数量
 * @return 返回一个包含浮点数数组的cJSON对象，如果内存不足则返回NULL
 * 
 * 此函数遍历给定的浮点数数组，为每个元素创建一个cJSON数字对象，并将它们作为子元素添加到一个cJSON数组对象中
 * 如果在创建任何cJSON对象时内存不足，函数将停止执行并返回当前创建的cJSON数组对象
 */
cJSON *cJSON_CreateFloatArray(const float *numbers,int count)	{
    int i;
    cJSON *n=0,*p=0,*a=cJSON_CreateArray();
    for(i=0;a && i<count;i++){
        n=cJSON_CreateNumber(numbers[i]);
        if(!i)a->child=n;
        else suffix_object(p,n);
        p=n;
    }
    return a;
}
/**
 * 创建一个包含双精度浮点数的数组的 cJSON 结构
 * 
 * @param numbers 一个双精度浮点数的数组，包含要添加到 cJSON 数组中的数字
 * @param count 表示要添加到 cJSON 数组中的双精度浮点数的数量
 * @return 返回一个指向创建的 cJSON 数组的指针，如果创建失败则返回 NULL
 * 
 * 此函数遍历给定的双精度浮点数数组，为每个数字创建一个 cJSON 数字对象，并将它们作为孩子添加到一个 cJSON 数组对象中
 * 如果输入的数组为空或计数为零，则函数将返回 NULL
 * 如果在创建 cJSON 数组或任何 cJSON 数字对象时出现内存分配失败，函数将返回 NULL
 */
cJSON *cJSON_CreateDoubleArray(const double *numbers,int count){
    int i;
    cJSON *n=0,*p=0,*a=cJSON_CreateArray();
    for(i=0;a && i<count;i++){
        n=cJSON_CreateNumber(numbers[i]);
        if(!i)a->child=n;
        else suffix_object(p,n);
        p=n;
    }
    return a;
}
/**
 * 创建一个包含字符串数组的cJSON对象
 * 
 * @param strings 一个包含多个字符串的数组，用于填充新创建的数组对象
 * @param count 字符串数组中元素的数量
 * 
 * @return 返回一个新创建的cJSON数组对象，其中包含传入的字符串数组，
 *         如果内存不足或其他错误，返回NULL
 * 
 * 此函数首先创建一个空的cJSON数组对象，然后遍历传入的字符串数组，
 * 对每个字符串创建一个cJSON字符串对象，并将其作为子对象添加到数组对象中
 */
cJSON *cJSON_CreateStringArray(const char **strings,int count)	{
    // 初始化变量，n用于临时存储新创建的字符串对象，p用于跟踪上一个对象
    int i;cJSON *n=0,*p=0,*a=cJSON_CreateArray();
    // 遍历字符串数组，为每个字符串创建一个cJSON字符串对象，并链接到数组对象中
    for(i=0;a && i<count;i++){
        n=cJSON_CreateString(strings[i]);
        // 如果是第一个字符串对象，则直接作为数组的子对象
        if(!i)a->child=n;
        // 否则，将新创建的字符串对象链接到上一个对象之后
        else suffix_object(p,n);
        // 更新p为当前对象，为下一次链接做准备
        p=n;
    }
    // 返回新创建的数组对象
    return a;
}

/* Duplication */
/**
 * 复制一个cJSON项及其所有子项。
 * 
 * @param item 要复制的cJSON项指针。
 * @param recurse 是否递归复制子项。非零值表示递归复制。
 * @return 返回复制后的新cJSON项指针，如果复制失败则返回NULL。
 * 
 * 此函数创建一个新的cJSON项，并根据参数item的内容进行复制。如果recurse参数为非零，将递归复制所有子项。
 * 在复制过程中，如果遇到内存分配失败，将返回NULL。如果item参数为NULL，同样返回NULL。
 * 
 * 注意：此函数使用了cJSON_New_Item和cJSON_strdup函数进行内存分配，如果这些函数失败，将导致复制失败。
 */
cJSON *cJSON_Duplicate(cJSON *item,int recurse)
{
    cJSON *newitem,*cptr,*nptr=0,*newchild;
    
    // 检查传入的item指针是否为NULL，如果为NULL则直接返回NULL。
    if (!item) return 0;
    
    // 创建一个新的cJSON项，用于复制。
    newitem=cJSON_New_Item();
    if (!newitem) return 0;
    
    // 复制item的类型、整型值和双精度浮点型值。
    newitem->type=item->type&(~cJSON_IsReference),newitem->valueint=item->valueint,newitem->valuedouble=item->valuedouble;
    
    // 如果item有字符串值，则复制字符串值。
    if (item->valuestring) {
        newitem->valuestring=cJSON_strdup(item->valuestring);
        if (!newitem->valuestring) {
            cJSON_Delete(newitem); return 0;
        }
    }
    
    // 如果item有键名字符串，则复制键名字符串。
    if (item->string) {
        newitem->string=cJSON_strdup(item->string);
        if (!newitem->string) {
            cJSON_Delete(newitem); return 0;
        }
    }
    
    // 如果不需要递归复制，则直接返回新创建的item。
    if (!recurse) return newitem;
    
    // 遍历item的所有子项，递归复制它们。
    cptr=item->child;
    while (cptr) {
        newchild=cJSON_Duplicate(cptr,1); // 递归复制子项。
        if (!newchild) {cJSON_Delete(newitem); return 0;}
        
        // 如果已经有子项被复制，则将新的子项链接到子项链表中。
        if (nptr) {
            nptr->next=newchild,newchild->prev=nptr;nptr=newchild;
        } else { // 如果还没有子项被复制，则将新的子项设置为第一个子项。
            newitem->child=newchild;nptr=newchild;
        }
        
        cptr=cptr->next; // 继续遍历下一个子项。
    }
    
    // 返回复制后的新item。
    return newitem;
}

/**
 * 压缩JSON字符串，去除不必要的空白字符和注释。
 * 
 * 该函数遍历输入的JSON字符串，移除空格、制表符、回车、换行符、双斜杠注释和多行注释。
 * 它确保字符串字面量保持不变，包括处理字符串中的转义字符。压缩过程有助于减少JSON字符串的大小，
 * 使其更适合传输或存储。
 * 
 * @param json 指向要压缩的JSON字符串的指针。压缩操作是在原地进行的，直接修改输入字符串。
 * 
 * 注意：该函数不验证JSON结构。它假设输入是一个格式正确的JSON字符串。
 */
void cJSON_Minify(char *json)
{
    char *into=json; // 用于跟踪字符串中下一个字符应写入的位置的指针。
    while (*json)
    {
        // 跳过各种空白字符。
        if (*json==' ') json++;
        else if (*json=='\t') json++;
        else if (*json=='\r') json++;
        else if (*json=='\n') json++;
        // 跳过多行注释。
        else if (*json=='/' && json[1]=='/')  while (*json && *json!='\n') json++;
        // 跳过多行注释。
        else if (*json=='/' && json[1]=='*') {while (*json && !(*json=='*' && json[1]=='/')) json++;json+=2;}
        // 处理字符串字面量，包括转义字符。
        else if (*json=='\"'){*into++=*json++;while (*json && *json!='\"'){if (*json=='\\') *into++=*json++;*into++=*json++;}*into++=*json++;}
        // 复制所有其他字符。
        else *into++=*json++;
    }
    *into=0; // 以空字符结尾。
}
