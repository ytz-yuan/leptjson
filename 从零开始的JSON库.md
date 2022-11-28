# 从零开始的JSON库

## 一 启程

JSON（JavaScript Object Notation）是一个用于数据交换的文本格式，虽然 JSON 源至于 JavaScript 语言，但它只是一种数据格式，可用于任何编程语言。现时具类似功能的格式有 XML、YAML，当中以 JSON 的语法最为简单。

例如，一个动态网页想从服务器获得数据时，服务器从数据库查找数据，然后把数据转换成 JSON 文本格式：

```json
{
    "title": "Design Patterns",
    "subtitle": "Elements of Reusable Object-Oriented Software",
    "author": [
        "Erich Gamma",
        "Richard Helm",
        "Ralph Johnson",
        "John Vlissides"
    ],
    "year": 2009,
    "weight": 1.8,
    "hardcover": true,
    "publisher": {
        "Company": "Pearson Education",
        "Country": "India"
    },
    "website": null
}
```

网页的脚本代码就可以把此 JSON 文本解析为内部的数据结构去使用。从此例子可看出，JSON 是树状结构，而 JSON 只包含 6 种数据类型：

- null: 表示为 null
- boolean: 表示为 true 或 false
- number: 一般的浮点数表示方式，在下一单元详细说明
- string: 表示为 "..."，即用双引号囊括
- array: 表示为 [ ... ]
- object: 表示为 { ... }

需要实现的JSON库，主要是完成3个需求：

1. 把JSON文本解析为一个树状数据结构（Parse，语法分析）
2. 提供接口访问该数据结构（Access）
3. 把数据结构转换成JSON文本（Stringify）

![img](C:\Users\20100\Desktop\尘世星曜\figure\75eecb0312e129d64dd3b028e1479e3d_r.jpg)

### 1.头文件与API设计

C 语言有头文件的概念，需要使用 #include 去引入头文件中的类型声明和函数声明。但由于头文件也可以 #include 其他头文件，为避免重复声明，通常会利用宏加入 #include 防范（include guard）：

```c
#ifndef LEPTJSON_H__
#define LEPTJSON_H__

/* ... */

#endif /* LEPTJSON_H__ */
```

#### #include防范

在[C](https://zh.wikipedia.org/wiki/C語言)和[C++](https://zh.wikipedia.org/wiki/C%2B%2B)编程语言中，**#include防范**，有时被称作**宏防范**，用于处理`#include` [指令](https://zh.wikipedia.org/w/index.php?title=編譯器指導指令&action=edit&redlink=1)时，可避免**重复引入**的问题。在[头文件](https://zh.wikipedia.org/wiki/標頭檔)加入#include防范是一种让文件[等幂](https://zh.wikipedia.org/wiki/等冪)的方法。

*重复引入*

以下的C语言程序展示了缺少#include防范时会出现的问题：

文件“grandfather.h：

```c
struct foo {
    int member;
};
```

文件“father.h”：

```c
#include "grandfather.h"
```

文件“child.c”：

```c
#include "grandfather.h"
#include "father.h"
```

此处**child.c**间接引入了两份**grandfather.h**[头文件](https://zh.wikipedia.org/wiki/標頭檔)中的内容。明显可以看出，`foo`结构被定义两次，因此会造成编译错误。

*使用#include防范*

文件“grandfather.h：

```c
#ifndef GRANDPARENT_H
#define GRANDPARENT_H
 
struct foo {
    int member;
};
 
#endif
```

文件“father.h”：

```c
#include "grandfather.h"
```

文件“child.c”：

```c
#include "grandfather.h"
#include "father.h"
```

此处**grandfather.h**第一次被引入时会定义宏**GRANDPARENT_H**。当**father.h**再次引入**grandfather.h**时，`#ifndef`测试失败，[编译器](https://zh.wikipedia.org/wiki/編譯器)会直接跳到`#endif`的部分，也避免了第二次定义`foo`结构。程序也就能够正常编译。

为了让**#include防范**正确运作，每个防范都必须检验并且有条件地设置不同的[前置处理](https://zh.wikipedia.org/w/index.php?title=前置處理&action=edit&redlink=1)宏。因此，使用了**#include防范**的方案必须制订一致性的命名方法，并确定这个方法不会和其他的[头文件](https://zh.wikipedia.org/wiki/標頭檔)或任何可见的全局变量冲突。

为了解决这个问题，许多C和C++程序开发工具提供非标准的指令`#pragma once`。在[头文件](https://zh.wikipedia.org/wiki/標頭檔)中加入这个指令，能够保证这个文件只会被引入一次。不过这个方法会被潜在性显著的困难阻挠，无论`#include`指令是否在不同的地方，但实际上起源于相同的开头（举例，请参考[符号链接](https://zh.wikipedia.org/w/index.php?title=符號連結&action=edit&redlink=1)）。同样的，因为`#pragma once`不是一个标准的指令，它的语义在不同的程序开发工具中也许会有微妙的不同。

宏的名字必须是唯一的，通常习惯以 *_H__* 作为后缀。由于 leptjson 只有一个头文件，可以简单命名为 LEPTJSON_H\_\_。如果项目有多个文件或目录结构，可以用 项目名称\_目录\_文件名称_H__ 这种命名方式。



如前所述，JSON 中有 6 种数据类型，如果把 true 和 false 当作两个类型就是 7 种，我们为此声明一个枚举类型（enumeration type）：

```c
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;
```

因为 C 语言没有 C++ 的命名空间（namespace）功能，一般会使用项目的简写作为标识符的前缀。通常枚举值用全大写（如 LEPT_NULL），而类型及函数则用小写（如 lept_type）。

接下来，我们声明 JSON 的数据结构。JSON 是一个树形结构，我们最终需要实现一个树的数据结构，每个节点使用 lept_value 结构体表示，我们会称它为一个 JSON 值（JSON value）。 在此单元中，我们只需要实现 null, true 和 false 的解析，因此该结构体只需要存储一个 lept_type。之后的单元会逐步加入其他数据。

```c
typedef struct {
    lept_type type;
}lept_value;
```

C 语言的结构体是以 struct X {} 形式声明的，定义变量时也要写成 struct X x;。为方便使用，上面的代码使用了 typedef。

***typedef*：**

在C和C++编程语言中，`typedef`是一个[关键字](https://zh.m.wikipedia.org/wiki/保留字)。它用来对一个[数据类型](https://zh.m.wikipedia.org/wiki/資料類型)取一个别名，目的是为了使源代码更易于阅读和理解。它通常用于简化声明复杂的类型组成的结构 ，但它也常常在各种长度的整数资料类型中看到，例如`size_t`和`time_t`。
`typedef`的语法是 : `typedef typedeclaration`;

创建 `Length` 作为 `int` 的别名 :

```c
typedef int Length;
```

创建 `PFI` 作为一个指向 "一个拥有两个 `char *` 当作参数并回传 `int` 的函数" 的指针的别名：

```c
typedef int (*PFI)(char *, char *);
```

1. `typedef`会被用来指定一种资料类型的意义。

   例如 : 以下示范一个普通的声明，速度及分数都被声明为`int`

   ```c
   int current_speed ; 
   int high_score ; 
   
   void congratulate(int your_score) 
   { 
       if (your_score > high_score) 
           ...
   ```

   通过`typedef`来指定新的资料类型的意义:

   ```c
   typedef int km_per_hour ;
   typedef int points ;
   
   km_per_hour current_speed ;  //"km_per_hour" is synonymous with "int" here,
   points high_score ;          //and thus, the compiler treats our new variables as integers.
   
   
   void congratulate(points your_score) {
       if (your_score > high_score)
           ...
   ```

   前面两段代码运作状况一样，但是使用`typedef`的第二段代码更清楚的表示了两个变量(score和speed)，虽然资料类型都是`int`，却是各自代表不同的意义，且他们的资料并不兼容。 但请注意，其清楚的表示不同意义只是对于工程师而言，C/C++的编译器认为两个变量都是`int`时，并不会显示警告或错误，如: 以下代码，使用声明为速度的变量作为`congratulate`函数的参数 :

   ```c
   void foo() 
   { 
       km_per_hour km100 = 100; 
       congratulate(km100);
   ```

   但是，虽然在上面的代码中，编译器认为"km_per_hour"等于`int`，但在使用前缀 `unsigned`, `long`, `signed`时，两者并不能互换使用。

   ```c
   void foo() {
       unsigned int a;         // Okay
       unsigned km_per_hour b; // Compiler complains 编译器警报
       long int c;             // Okay
       long km_per_hour d;     // Compiler complains
   ```

2. 简化声明语法

   ```c
   struct var {
       int data1;
       int data2;
       char data3;
   };
   ```

   此处用户定义一个数据类型 *`var`*。

   像这样创建一个 *`var`* 类型的变量，代码必须写为（注意，在 C++ 中声明一个 `struct` 时，同时也隐含了 `typedef`，C 则没有）：

   ```c
   struct var a;
   ```

   在例子的最末处加入一行语句：

   ```c
   typedef struct var newtype;
   ```

   现在要创建类型 *`var`* 的变量时，代码可以写为：

   ```c
   newtype a;
   ```

3. [更多使用方法]([typedef - 维基百科，自由的百科全书 (wikipedia.org)](https://zh.m.wikipedia.org/wiki/Typedef))



然后，我们现在只需要两个 API 函数，一个是解析 JSON：

```c
int lept_parse(lept_value* v, const char* json);
```

传入的 JSON 文本是一个 C 字符串（空结尾字符串／null-terminated string），由于我们不应该改动这个输入字符串，所以使用 const char* 类型。

另一注意点是，传入的根节点指针 v 是由使用方负责分配的，所以一般用法是：

```c
lept_value v;
const char json[] = ...;
int ret = lept_parse(&v, json);
```

返回值是以下这些枚举值，无错误会返回 LEPT_PARSE_OK，其他值在下节解释。

```c
enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR
};
```

***enum***：

**枚举（enumerated type）**用来声明一组整数常量。默认情况下，枚举声明格式为enum *type* {value1,value2,...,valuen}；此时value1,value2分别为0，1，直到n-1。事实上，枚举类型在C语言实现中是以int类型储存的。以下是枚举的一个声明：

```c
enum a { b , c , d };
```

在此之后，便可以以如下方式使用：

```c
enum a foo;
foo = b;
if(foo != c) //等同于if(foo != 1)
{
    do_something();
}
```

而此时的b，c，d分别为0，1，2。 另外，也可以手动为枚举列表中的常量赋值。下面是一个例子：

```c
enum colour {red = 100,blue = 700,yellow = 200};
```



现在我们只需要一个访问结果的函数，就是获取其类型：

```c
lept_type lept_get_type(const lept_value* v);
```
### 2.JSON语法子集

下面是此单元的 JSON 语法子集，使用 [RFC7159](https://link.zhihu.com/?target=http%3A//rfc7159.net/rfc7159) 中的 [ABNF](https://link.zhihu.com/?target=https%3A//tools.ietf.org/html/rfc5234) 表示：

```c
JSON-text = ws value ws
ws = *(%x20 / %x09 / %x0A / %x0D)
value = null / false / true 
null  = "null"
false = "false"
true  = "true"
```

当中 %xhh 表示以 16 进制表示的字符，/ 是多选一，* 是零或多个，( ) 用于分组。

那么第一行的意思是，JSON 文本由 3 部分组成，首先是空白（whitespace），接着是一个值，最后是空白。

第二行告诉我们，所谓空白，是由零或多个空格符（space U+0020）、制表符（tab U+0009）、换行符（LF U+000A）、回车符（CR U+000D）所组成。

第三行是说，我们现时的值只可以是 null、false 或 true，它们分别有对应的字面值（literal）。

我们的解析器应能判断输入是否一个合法的 JSON。如果输入的 JSON 不合符这个语法，我们要产生对应的错误码，方便使用者追查问题。

在这个 JSON 语法子集下，我们定义 3 种错误码：

* 若一个 JSON 只含有空白，传回 LEPT_PARSE_EXPECT_VALUE。
* 若一个值之后，在空白之后还有其他字符，传回 LEPT_PARSE_ROOT_NOT_SINGULAR。
* 若值不是那三种字面值，传回 LEPT_PARSE_INVALID_VALUE。

### 3. 单元测试

许多同学在做练习或刷题时，都是以 printf／cout 打印结果，再用肉眼对比结果是否乎合预期。但当软件项目越来越复杂，这个做法会越来越低效。一般我们会采用自动的测试方式，例如单元测试（unit testing）。单元测试也能确保其他人修改代码后，原来的功能维持正确（这称为回归测试／regression testing）。

常用的单元测试框架有 xUnit 系列，如 C++ 的 [Google Test](https://link.zhihu.com/?target=https%3A//github.com/google/googletest)、C# 的 [NUnit](https://link.zhihu.com/?target=http%3A//www.nunit.org/)。我们为了简单起见，会编写一个极简单的单元测试方式。

一般来说，软件开发是以周期进行的。例如，加入一个功能，再写关于该功能的单元测试。但也有另一种软件开发方法论，称为测试驱动开发（test-driven development, TDD），它的主要循环步骤是：

1. 加入一个测试。
2. 运行所有测试，新的测试应该会失败。
3. 编写实现代码。
4. 运行所有测试，若有测试失败回到3。
5. 重构代码。
6. 回到 1。

TDD 是先写测试，再实现功能。好处是实现只会刚好满足测试，而不会写了一些不需要的代码，或是没有被测试的代码。

但无论我们是采用 TDD，或是先实现后测试，都应尽量加入足够覆盖率的单元测试。

回到 leptjson 项目，test.c 包含了一个极简的单元测试框架：

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leptjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

static void test_parse_null() {
    lept_value v;
    v.type = LEPT_TRUE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

/* ... */

static void test_parse() {
    test_parse_null();
    /* ... */
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
```

现时只提供了一个 EXPECT_EQ_INT(expect, actual) 的宏，每次使用这个宏时，如果 expect != actual（预期值不等于实际值），便会输出错误信息。 若按照 TDD 的步骤，我们先写一个测试，如上面的 test_parse_null()，而 lept_parse() 只返回 LEPT_PARSE_OK：

```
/Users/miloyip/github/json-tutorial/tutorial01/test.c:27: expect: 0 actual: 1
1/2 (50.00%) passed
```

第一个返回 LEPT_PARSE_OK，所以是通过的。第二个测试因为 lept_parse() 没有把 v.type 改成 LEPT_NULL，造成失败。我们再实现 lept_parse() 令到它能通过测试。

### 4.宏的编写技巧

有些同学可能不了解 EXPECT_EQ_BASE 宏的编写技巧，简单说明一下。反斜线代表该行未结束，会串接下一行。而如果宏里有多过一个语句（statement），就需要用 do { /**...**/ } while(0) 包裹成单个语句，否则会有如下的问题：

```c
#define M() a(); b()
if (cond)
    M();
else
    c();

/* 预处理后 */

if (cond)
    a(); b();
else /* <- else 缺乏对应 if */
    c();
```

只用 {} 也不行：

```c
#define M() { a(); b(); }
/* 预处理后 */
if (cond)
    { a(); b(); }; /* 最后的分号代表 if 语句结束 */
else               /* else 缺乏对应 if */
    c();
```

用 do while 就行了：

```c
#define M() do { a(); b(); } while(0)
/* 预处理后 */
if (cond)
    do { a(); b(); } while(0);
else
    c();
```

### 5.实现解析器

有了 API 的设计、单元测试，终于要实现解析器了。

首先为了减少解析函数之间传递多个参数，我们把这些数据都放进一个 lept_context 结构体：

```c
typedef struct {
    const char* json;
}lept_context;

/* ... */

/* 提示：这里应该是 JSON-text = ws value ws，*/
/* 以下实现没处理最后的 ws 和 LEPT_PARSE_ROOT_NOT_SINGULAR */
int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    return lept_parse_value(&c, v);
}
```

暂时我们只储存 json 字符串当前位置，之后的单元我们需要加入更多内容。

lept_parse() 若失败，会把 v 设为 null 类型，所以这里先把它设为 null，让 lept_parse_value() 写入解析出来的根值。

leptjson 是一个手写的递归下降解析器（recursive descent parser）。由于 JSON 语法特别简单，我们不需要写分词器（tokenizer），只需检测下一个字符，便可以知道它是哪种类型的值，然后调用相关的分析函数。对于完整的 JSON 语法，跳过空白后，只需检测当前字符：

- n ➔ null
- t ➔ true
- f ➔ false
- " ➔ string
- 0-9/- ➔ number
- [ ➔ array
- { ➔ object

所以，我们可以按照 JSON 语法一节的 EBNF 简单翻译成解析函数：

```c
#define EXPECT(c, ch) do { assert(*c->json == (ch)); c->json++; } while(0)

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

/* null  = "null" */
static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

/* value = null / false / true */
/* 提示：下面代码没处理 false / true，将会是练习之一 */
static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_null(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return LEPT_PARSE_INVALID_VALUE;
    }
}
```

由于 lept_parse_whitespace() 是不会出现错误的，返回类型为 void。其它的解析函数会返回错误码，传递至顶层。

### 6.关于断言

断言（assertion）是 C 语言中常用的防御式编程方式，减少编程错误。最常用的是在函数开始的地方，检测所有参数。有时候也可以在调用函数后，检查上下文是否正确。

C 语言的标准库含有 assert() 这个宏（需 #include ），提供断言功能。当程序以 release 配置编译时（定义了 NDEBUG 宏），assert() 不会做检测；而当在 debug 配置时（没定义 NDEBUG 宏），则会在运行时检测 assert(cond) 中的条件是否为真（非 0），断言失败会直接令程序崩溃。

初使用断言的同学，可能会把有副作用的代码放在 assert() 中：

```c
assert(x++ == 0); /* 这是错误的! */
```

因为这样会导致 debug 和 release 版的行为不一样。

另一个问题是，初学者可能会难于分辨何时使用断言，何时处理运行时错误（如返回错误值或在 C++ 中抛出异常）。简单的答案是，如果那个错误是由于程序员错误编码所造成的（例如传入不合法的参数），那么应用断言；如果那个错误是程序员无法避免，而是由运行时的环境所造成的，就要处理运行时错误（例如开启文件失败）。

### 7.总结与练习

本文介绍了如何配置一个编程环境，单元测试的重要性，以至于一个 JSON 解析器的子集实现。如果你读到这里，还未动手，建议你快点试一下。以下是本单元的练习，很容易的，但我也会在稍后发出解答篇。

1. 修正关于 LEPT_PARSE_ROOT_NOT_SINGULAR 的单元测试，若 json 在一个值之后，空白之后还有其它字符，则要返回 LEPT_PARSE_ROOT_NOT_SINGULAR。
2. 参考 test_parse_null()，加入 test_parse_true()、test_parse_false() 单元测试。
3. 参考 lept_parse_null() 的实现和调用方，解析 true 和 false 值。

## 二 解析数字

### 1.初探重构

在讨论解析数字之前，我们再补充 TDD 中的一个步骤──重构（refactoring）。根据[1]，重构是一个这样的过程：

> 在不改变代码外在行为的情况下，对代码作出修改，以改进程序的内部结构。

在 TDD 的过程中，我们的目标是编写代码去通过测试。但由于这个目标的引导性太强，我们可能会忽略正确性以外的软件品质。在通过测试之后，代码的正确性得以保证，我们就应该审视现时的代码，看看有没有地方可以改进，而同时能维持测试顺利通过。我们可以安心地做各种修改，因为我们有单元测试，可以判断代码在修改后是否影响原来的行为。

那么，哪里要作出修改？Beck 和 Fowler（[1] 第 3 章）认为程序员要培养一种判断能力，找出程序中的坏味道。例如，在第一单元的练习中，可能大部分人都会复制 lept_parse_null() 的代码，作一些修改，成为 lept_parse_true() 和lept_parse_false()。如果我们再审视这 3 个函数，它们非常相似。这违反编程中常说的 DRY（don't repeat yourself）原则。本单元的第一个练习题，就是尝试合并这 3 个函数。

另外，我们也可能发现，单元测试代码也有很重复的代码，例如 test_parse_invalid_value() 中我们每次测试一个不合法的 JSON 值，都有 4 行相似的代码。我们可以把它用宏的方式把它们简化：

```c
#define TEST_ERROR(error, json)\
    do {\
        lept_value v;\
        v.type = LEPT_FALSE;\
        EXPECT_EQ_INT(error, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));\
    } while(0)

static void test_parse_expect_value() {
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}
```

上述把 3 个函数合并后，优点是减少重复的代码，维护较容易，但缺点可能是带来性能的少量影响。

### 2.JSON数字语法

本单元的重点在于解析 JSON number 类型。我们先看看它的语法：

```
number = [ "-" ] int [ frac ] [ exp ]
int = "0" / digit1-9 *digit
frac = "." 1*digit
exp = ("e" / "E") ["-" / "+"] 1*digit
```

number 是以十进制表示，它主要由 4 部分顺序组成：负号、整数、小数、指数。只有整数是必需部分。注意和直觉可能不同的是，正号是不合法的。

整数部分如果是 0 开始，只能是单个 0；而由 1-9 开始的话，可以加任意数量的数字（0-9）。也就是说，0123 不是一个合法的 JSON 数字。

小数部分比较直观，就是小数点后是一或多个数字（0-9）。

JSON 可使用科学记数法，指数部分由大写 E 或小写 e 开始，然后可有正负号，之后是一或多个数字（0-9）。

SON 标准 [ECMA-404](https://link.zhihu.com/?target=http%3A//www.ecma-international.org/publications/files/ECMA-ST/ECMA-404.pdf) 采用图的形式表示语法，也可以更直观地看到解析时可能经过的路径：

![img](../尘世星曜/figure/v2-de5a6e279cbac2071284bfa7bb1e5730_r.jpg)

上一单元的 null、false、true 在解析后，我们只需把它们存储为类型。但对于数字，我们要考虑怎么存储解析后的结果。

### 3.数字表达方式

从 JSON 数字的语法，我们可能直观地会认为它应该表示为一个浮点数（floating point number），因为它带有小数和指数部分。然而，标准中并没有限制数字的范围或精度。为简单起见，leptjson 选择以双精度浮点数（C 中的 double 类型）来存储 JSON 数字。

我们为 lept_value 添加成员：

```c
typedef struct {
    double n;
    lept_type type;
}lept_value;
```

仅当 type == LEPT_NUMBER 时，n 才表示 JSON 数字的数值。所以获取该值的 API 是这么实现的：

```c
double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
```

使用者应确保类型正确，才调用此 API。我们继续使用断言来保证。

### 4.单元测试

我们定义了 API 之后，按照 TDD，我们可以先写一些单元测试。这次我们使用多行的宏的减少重复代码：

```c
#define TEST_NUMBER(expect, json)\
    do {\
        lept_value v;\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));\
    } while(0)

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */
}
```

以上这些都是很基本的测试用例，也可供调试用。大部分情况下，测试案例不能穷举所有可能性。因此，除了加入一些典型的用例，我们也常会使用一些边界值，例如最大值等。练习中会让同学找一些边界值作为用例。

除了这些合法的 JSON，我们也要写一些不合语法的用例：

```c
static void test_parse_invalid_value() {
    /* ... */
    /* invalid number */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
}
```

### 5.十进制转换至二进制

我们需要把十进制的数字转换成二进制的 double。这并不是容易的事情 [2]。为了简单起见，leptjson 将使用标准库的[strtod()](https://link.zhihu.com/?target=http%3A//en.cppreference.com/w/c/string/byte/strtof) 来进行转换。strtod() 可转换 JSON 所要求的格式，但问题是，一些 JSON 不容许的格式，strtod() 也可转换，所以我们需要自行做格式校验。

```c
#include <stdlib.h>  /* NULL, strtod() */

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    /* \TODO validate number */
    v->n = strtod(c->json, &end);
    if (c->json == end)
        return LEPT_PARSE_INVALID_VALUE;
    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}
```

加入了 number 后，value 的语法变成：

```c
value = null / false / true / number
```

记得在第一单元中，我们说可以用一个字符就能得知 value 是什么类型，有 11 个字符可判断 number：

* 0-9/- ➔ number

但是，由于我们在 lept_parse_number() 内部将会校验输入是否正确的值，我们可以简单地把余下的情况都交给lept_parse_number()：

```c
static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_true(c, v);
        case 'f':  return lept_parse_false(c, v);
        case 'n':  return lept_parse_null(c, v);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}
```

### 6.总结与练习

本单元讲述了 JSON 数字类型的语法，以及 leptjson 所采用的自行校验＋strtod()转换为 double 的方案。实际上一些 JSON 库会采用更复杂的方案，例如支持 64 位带符号／无符号整数，自行实现转换。以我的个人经验，解析／生成数字类型可以说是 RapidJSON 中最难实现的部分，也是 RapidJSON 高效性能的原因，有机会再另外撰文解释。

此外我们谈及，重构与单元测试是互相依赖的软件开发技术，适当地运用可提升软件的品质。之后的单元还会有相关的话题。

1. 重构合并 lept_parse_null()、lept_parse_false()、lept_parse_true 为 lept_parse_literal()。
2. 加入 [维基百科双精度浮点数](https://link.zhihu.com/?target=https%3A//en.wikipedia.org/wiki/Double-precision_floating-point_format%23Double-precision_examples) 的一些边界值至单元测试，如 min subnormal positive double、max double 等。
3. 去掉 test_parse_invalid_value() 和 test_parse_root_not_singular 中的 #if 0 ... #endif，执行测试，证实测试失败。按 JSON number 的语法在 lept_parse_number() 校验，不符合标准的程况返回LEPT_PARSE_INVALID_VALUE　错误码。
4. 去掉 test_parse_number_too_big 中的 #if 0 ... #endif，执行测试，证实测试失败。仔细阅读 [strtod()](https://link.zhihu.com/?target=http%3A//en.cppreference.com/w/c/string/byte/strtof)，看看怎样从返回值得知数值是否过大，以返回 LEPT_PARSE_NUMBER_TOO_BIG 错误码。（提示：这里需要 #include 额外两个标准库头文件。）

以上最重要的是第 3 条题目，就是要校验 JSON 的数字语法。建议可使用以下两个宏去简化一下代码：

```c
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
```

另一提示，在校验成功以后，我们不再使用 end 指针去检测 strtod() 的正确性，第二个参数可传入 NULL。

### 7.参考

[1] Fowler, Martin. Refactoring: improving the design of existing code. Pearson Education India, 2009. 中译本：《重构：改善既有代码的设计》，熊节译，人民邮电出版社，2010年。 

[2] Gay, David M. "Correctly rounded binary-decimal and decimal-binary conversions." Numerical Analysis Manuscript 90-10 (1990).
