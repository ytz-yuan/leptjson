/*
leptjson 的实现文件（implementation file），含有内部的类型声明和函数实现。此文件会编译成库。
*/
#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */

#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

#define EXPECT(c, ch) do { assert(*c->json == (ch)); c->json++; } while(0)

/*
为了减少解析函数之间传递多个参数，把要解析的JSON数据都放进一个 lept_context 结构体
*/
typedef struct {
    const char* json;
}lept_context;

/*
解析空白，空白只能由零或多个空格符（space U+0020）、
制表符（tab U+0009）、换行符（LF U+000A）、回车符（CR U+000D）所组成。
*/
static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

/*
重构合并
*/
static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    size_t i; //size_t 类型表示C中任何对象所能达到的最大长度，它是无符号整数。注意在 C 语言中，数组长度、索引值最好使用 size_t 类型
    EXPECT(c, literal[0]);
    for(i = 0; literal[i+1]; i++) {
        if(c->json[i] != literal[i+1]) {
            return LEPT_PARSE_INVALID_VALUE;
        }
        c->json += 1;
        v->type = type;
        return LEPT_PARSE_OK;
    }
}

/*
解析空白之后null值，如果空白之后是。
static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE; //若一个值之后，在空白之后还有其他字符，返回
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

解析空白之后true值，如果空白之后是。
static int lept_parse_true(lept_context* c, lept_value* v) {
    EXPECT(c, 't');
    if(c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_TRUE;
    return LEPT_PARSE_OK;
}

解析空白之后false值，如果空白之后是。
static int lept_parse_false(lept_context*c, lept_value* v) {
    EXPECT(c, 'f');
    if(c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 4;
    v->type = LEPT_FALSE;
    return LEPT_PARSE_OK;
}
*/

static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* p = c->json;
    if(*p == '-') p++;
    if(*p == '0') p++;
    else {
        if(!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }
    if(*p == '.') {
        p++;
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }
    if(*p == 'e' || *p == 'E') {
        p++;
        if(*p == '+' || *p == '-') p++;
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->n = strtod(c->json, NULL);
    if(errno == ERANGE && v->n == HUGE_VAL) return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}

/*
空白之后的value,
value = null/true/false
*/
static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        default:   return lept_parse_number(c, v); //若值不是那三种字面值，返回
        case '\0': return LEPT_PARSE_EXPECT_VALUE; //若一个 JSON 只含有空白返回
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if(*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert( v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}