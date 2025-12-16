/*
  Copyright (c) 2009 Dave Gamble
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
 
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
 
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef cJSON__h
#define cJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

/* cJSON 类型定义 */
#define cJSON_False 0
#define cJSON_True 1
#define cJSON_NULL 2
#define cJSON_Number 3
#define cJSON_String 4
#define cJSON_Array 5
#define cJSON_Object 6
	
#define cJSON_IsReference 256
#define cJSON_StringIsConst 512

/* cJSON 结构体定义 */
typedef struct cJSON {
    struct cJSON *next, *prev;  /* 用于遍历数组或对象链表。也可以使用 GetArraySize/GetArrayItem/GetObjectItem */
    struct cJSON *child;        /* 数组或对象项将有一个指向数组或对象中项链表的子指针 */

    int type;                   /* 项的类型，如上定义 */

    char *valuestring;          /* 如果类型为 cJSON_String，则为该项的字符串 */
    int valueint;               /* 如果类型为 cJSON_Number，则为该项的整数 */
    double valuedouble;         /* 如果类型为 cJSON_Number，则为该项的双精度浮点数 */

    char *string;               /* 如果该项是对象的子项，则为该项的名称字符串 */
} cJSON;

/* 自定义内存分配函数结构体 */
typedef struct cJSON_Hooks {
    void *(*malloc_fn)(size_t sz);
    void (*free_fn)(void *ptr);
} cJSON_Hooks;

/* 提供自定义的 malloc, realloc 和 free 函数给 cJSON */
extern void cJSON_InitHooks(cJSON_Hooks* hooks);

/* 解析 JSON 字符串并返回一个可以查询的 cJSON 对象。使用完毕后调用 cJSON_Delete */
extern cJSON *cJSON_Parse(const char *value);
/* 将 cJSON 实体渲染为文本用于传输或存储。使用完毕后释放 char* */
extern char  *cJSON_Print(cJSON *item);
/* 将 cJSON 实体渲染为文本用于传输或存储，不带任何格式化。使用完毕后释放 char* */
extern char  *cJSON_PrintUnformatted(cJSON *item);
/* 使用缓冲策略将 cJSON 实体渲染为文本。prebuffer 是对最终大小的猜测，猜测得当可以减少重新分配。fmt=0 表示不带格式化，fmt=1 表示带格式化 */
extern char *cJSON_PrintBuffered(cJSON *item, int prebuffer, int fmt);
/* 删除 cJSON 实体及其所有子实体 */
extern void   cJSON_Delete(cJSON *c);

/* 返回数组（或对象）中的项数 */
extern int	  cJSON_GetArraySize(cJSON *array);
/* 从数组 "array" 中检索第 "item" 项。如果失败则返回 NULL */
extern cJSON *cJSON_GetArrayItem(cJSON *array, int item);
/* 从对象中获取 "string" 项。不区分大小写 */
extern cJSON *cJSON_GetObjectItem(cJSON *object, const char *string);

/* 用于分析解析失败的情况。当 cJSON_Parse() 返回 0 时定义，返回解析错误的指针。当 cJSON_Parse() 成功时为 0 */
extern const char *cJSON_GetErrorPtr(void);
	
/* 这些调用会创建适当类型的 cJSON 项 */
extern cJSON *cJSON_CreateNull(void);
extern cJSON *cJSON_CreateTrue(void);
extern cJSON *cJSON_CreateFalse(void);
extern cJSON *cJSON_CreateBool(int b);
extern cJSON *cJSON_CreateNumber(double num);
extern cJSON *cJSON_CreateString(const char *string);
extern cJSON *cJSON_CreateArray(void);
extern cJSON *cJSON_CreateObject(void);

/* 这些工具会创建包含 count 项的数组 */
extern cJSON *cJSON_CreateIntArray(const int *numbers, int count);
extern cJSON *cJSON_CreateFloatArray(const float *numbers, int count);
extern cJSON *cJSON_CreateDoubleArray(const double *numbers, int count);
extern cJSON *cJSON_CreateStringArray(const char **strings, int count);

/* 将项附加到指定的数组/对象 */
extern void cJSON_AddItemToArray(cJSON *array, cJSON *item);
extern void	cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
extern void	cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item);	/* 当字符串是常量（例如字面量）并且肯定能存活到 cJSON 对象之外时使用 */
/* 将项的引用附加到指定的数组/对象。当你想将现有的 cJSON 添加到新的 cJSON 但不想破坏现有的 cJSON 时使用 */
extern void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);
extern void	cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item);

/* 从数组/对象中分离或删除项 */
extern cJSON *cJSON_DetachItemFromArray(cJSON *array, int which);
extern void   cJSON_DeleteItemFromArray(cJSON *array, int which);
extern cJSON *cJSON_DetachItemFromObject(cJSON *object, const char *string);
extern void   cJSON_DeleteItemFromObject(cJSON *object, const char *string);
	
/* 更新数组项 */
extern void cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem);	/* 将现有项向右移动 */
extern void cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem);
extern void cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem);

/* 复制 cJSON 项 */
extern cJSON *cJSON_Duplicate(cJSON *item, int recurse);
/* 复制将创建一个新的与传入项相同的 cJSON 项，位于新的内存中，需要释放。
   如果 recurse!=0，则会复制任何连接到该项的子项。
   从 Duplicate 返回时，item->next 和 ->prev 指针总是零 */

/* ParseWithOpts 允许你要求（并检查）JSON 是否以空终止，并检索最终解析字节的指针 */
extern cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated);

extern void cJSON_Minify(char *json);

/* 快速创建项的宏 */
#define cJSON_AddNullToObject(object, name)		cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object, name)		cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object, name)		cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddBoolToObject(object, name, b)	cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define cJSON_AddNumberToObject(object, name, n)	cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject(object, name, s)	cJSON_AddItemToObject(object, name, cJSON_CreateString(s))

/* 当分配整数值时，也需要将其传播到 valuedouble */
#define cJSON_SetIntValue(object, val)			((object) ? (object)->valueint = (object)->valuedouble = (val) : (val))
#define cJSON_SetNumberValue(object, val)		((object) ? (object)->valueint = (object)->valuedouble = (val) : (val))

#ifdef __cplusplus
}
#endif

#endif
