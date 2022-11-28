/*
leptjson 的头文件（header file），含有对外的类型和 API 函数声明。
*/
#ifndef LEPTJSON_H__
#define LEPTJSON_H__ //防止重复引用

typedef enum { //声明一个枚举类型，JSON的7种数据类型
    LEPT_NULL, 
    LEPT_FALSE, 
    LEPT_TRUE, 
    LEPT_NUMBER, 
    LEPT_STRING, 
    LEPT_ARRAY, 
    LEPT_OBJECT 
} lept_type; //该枚举类型名为lept_type

/*
JSON 是一个树形结构，我们最终需要实现一个树的数据结构，
每个节点使用 lept_value 结构体表示，我们会称它为一个 JSON 值（JSON value）。 
在此单元中，我们只需要实现 null, true 和 false 的解析，
因此该结构体只需要存储一个 lept_type。之后的单元会逐步加入其他数据。
*/
typedef struct {
    double n;
    lept_type type;
} lept_value; //JSON树形结构的节点

enum { //lept_parse函数的返回值
    LEPT_PARSE_OK = 0, //无错误返回
    LEPT_PARSE_EXPECT_VALUE, //若一个 JSON 只含有空白返回
    LEPT_PARSE_INVALID_VALUE, //若值不是那三种字面值，返回
    LEPT_PARSE_ROOT_NOT_SINGULAR, //若一个值之后，在空白之后还有其他字符，返回
    LEPT_PARSE_NUMBER_TOO_BIG //数值太大，返回
};

/*
API函数，作用为解析JSON。
传入的 JSON 文本是一个C字符串（空结尾字符串／null-terminated string），
由于我们不应该改动这个输入字符串，所以使用 const char* 类型。
另一注意点是，传入的根节点指针 v 是由使用方负责分配的，所以一般用法是：
lept_value v;
const char json[] = ...;
int ret = lept_parse(&v, json);
return：设定的枚举值，无错误会返回 LEPT_PARSE_OK
*/
int lept_parse(lept_value* v, const char* json); //传入JSON文本进行解析

/*
访问结果的函数，就是获取其类型
即传入一个JSON树节点，获取其类型。
*/
lept_type lept_get_type(const lept_value* v);

/*
获取数值的API
*/
double lept_get_number(const lept_value* v);

#endif /* LEPTJSON_H__ */