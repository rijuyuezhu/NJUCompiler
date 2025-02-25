#import "@preview/numbly:0.1.0": numbly
#import "@preview/cuti:0.2.1": show-cn-fakebold
#show: show-cn-fakebold

#set text(font: ("Libertinus Serif", "Noto Serif CJK SC"))

#set heading(
  numbering: numbly(
    none,
    "{2:1.}",
    "{2:1.}{3:1.}",
  ),
)

#set page(
  paper: "a4",
  numbering: "1",
)
#set par(linebreaks: "optimized")
#show heading.where(level: 1): it => [
  #align(center)[
    #it
  ]
]

#show heading.where(level: 2): it => {
  it
  v(-10pt)
  line(length: 100%)
}

#let indent = h(2em)
#set underline(offset: .1em, stroke: .05em, evade: false)


= #text(font: ("Libertinus Serif", "STZhongsong"))[编译原理 Project 1 实验报告]

#align(center)[
  #v(1em)
  #text(font: "Kaiti")[黄文睿] \
  221180115
]

== 概览 <intro>

- 程序实现功能：实现了所有需实现内容，详见 @impl[Sec]。
- 程序编译指令：和下发的 Makefile 保持一致，详见 @compile[Sec]。
- 其它亮点：详见 @other[Sec]。
- 实验感想：详见 @thought[Sec]。

== 程序实现功能 <impl>

我在项目中实现了编译原理 Project 1 的全部内容，包括：
- 词法分析器；完成了 Flex 代码的编写；
- 语法分析器；完成了 Bison 代码的编写，其中包含了一些常见语法错误的错误处理；
- AST 的建立与输出：在 Bison 源代码中，插入了建立 AST 的代码；在 parse 后即输出。

== 程序编译指令 <compile>

对于根目录如图的项目:

#figure(
```plain
.
├── Code
│   ├── ast.c
│   ├── ast.h
│   ├── ...
│   └── utils.h
├── parser
├── README
├── report.pdf
└── Test
    ├── test1.cmm
    └── test2.cmm
```
)

运行 `make -C ./Code` 即可编译项目，得到的可执行文件位于 `./Code/parser`。其中 Makefile 和下发的 Makefile 一致。

== 其它亮点 <other>

为了方便这个 C 项目的管理，我在项目中实现了方法调用与 RAII（@oop[Sec]），并在此基础上实现了常用容器 `Vec` 和 `Mapping` (@container[Sec])；
另外，我也实现了项目的单元测试和综合测试（@test[Sec]）

=== 方法调用和 RAII 在 C 中的手动实现<oop>

为了方便在 C 语言中进行一些面向对象编程（不包括多态，主要目的是将数据和函数联系起来形成命名空间），我用宏实现了基本的方法定义和方法调用，并借此实现了手动的 RAII。

```c
typedef struct TaskEngine {
    const char *input_file;

    AstNode *ast_root;
    bool ast_error;
} TaskEngine;

void MTD(TaskEngine, init, /, const char *file);
void MTD(TaskEngine, drop, /);
void MTD(TaskEngine, parse_ast, /);
void MTD(TaskEngine, print_ast, /);
```

调用时使用类似于如下的语法：
```c
TaskEngine engine = CREOBJ(TaskEngine, /, argv[1]);
CALL(TaskEngine, engine, parse_ast, /);
if (engine.ast_root && !engine.ast_error) { // No error in parsing
    CALL(TaskEngine, engine, print_ast, /);
}
DROPOBJ(TaskEngine, engine);
```

'`/`' 用于分割方法名和参数，`CREOBJ` 用于创建对象，`CALL` 用于调用方法，`DROPOBJ` 用于销毁对象。
语法比较丑，但读多了还是比较直观的。在 C 中进行内存管理比较麻烦。为了方便内存管理，我借用了 Rust 的赋值的语义：
- 每一个变量都表示栈上某个值；变量间的赋值永远是浅拷贝（按字节的直接复制），不会涉及到堆上的数据，语义为变量移动；
- 提供了 `MTD(T, init, /, ...)`, `MTD(T, drop, /)`, `MTD(T, clone, /)`, `MTD(T, clone_from, /, const T *other)` 四个接口方法，
  用于初始化、销毁、深拷贝、从另一个对象深拷贝；
- 所有的值的可用性，以及移动语义、内存释放的正确性靠手动维护。

=== 常用容器实现 <container>

借助 OOP 和 RAII 的相关框架，以及 C 语言的宏，我实现了可变长数组 `Vec`（类似 C++ 的 `std::vector`）和映射 `Mapping`（类似 C++ 的 `std::map`）：

```c
// in .h
DECLARE_PLAIN_VEC(VecI32, i32, FUNC_EXTERN);
// in .c
DEFINE_PLAIN_VEC(VecI32, i32, FUNC_EXTERN);
...
// in .c
static VecI32 gen_range(usize n) {
    VecI32 v = CREOBJ(VecI32, /);
    for (usize i = 0; i < n; i++) {
        CALL(VecI32, v, push_back, /, (i32)i);
    }
    return v;
}
```
使用了宏作为泛型的实现机制，并实现了大部分常用的方法；`Vec` 的实现使用了常用的动态扩容（在满时扩容到原来空间的两倍，从而均摊单次插入 $O(1)$）。在此基础上，我实现了 `String`（类似 C++ 中的 `std::string`）。

```c
// in .h
DECLARE_MAPPING(MapSS, String, String, FUNC_EXTERN, GENERATOR_CLASS_KEY,
                GENERATOR_CLASS_VALUE, GENERATOR_CLASS_COMPARATOR);
// in .c
DEFINE_MAPPING(MapSS, String, String, FUNC_EXTERN);
...
// in .c
MapSS m = CREOBJ(MapSS, /);
String k = NSCALL(String, from_raw, /, "hello");
String v = NSCALL(String, from_raw, /, "yes");
MapSSInsertResult res = CALL(MapSS, m, insert, /, k, v);
ASSERT(res.inserted);
DROPOBJ(MapSS, m);
```

使用了 Treap 作为了 `Mapping` 的内层平衡二叉树实现。

这些容器为实现 AST，以及之后符号表等的实现奠定了基础。

=== 单元测试和综合测试 <test>

我在项目中实现了单元测试，和综合测试。

- 单元测试中，将所有源代码（除 `main.c`）编译成静态库，然后由另一个 `tests/` 下的项目使用。每次开始测试前会进行 fork，以保证测试的独立性。
- 综合测试则使用了 Python 驱动，运行测试用例并和期望输出对比。

```console
[rijuyuezhu@rjyz-linux:~/Code/Compiler on A1] 
% ./init test
===== Unit        Tests =====
tests to run    : plain_vec class_vec map hstr 
tests filtered  : 

:: Start running test: plain_vec
:: End   running test: plain_vec
:: Start running test: class_vec
:: End   running test: class_vec
:: Start running test: map
:: End   running test: map
:: Start running test: hstr
:: End   running test: hstr

===== Integration Tests =====
tests to run    : cmm1 cmm2
tests filtered  : 
tests not found : 

:: Start running test: cmm1
:: End   running test: cmm1
:: Start running test: cmm2
:: End   running test: cmm2

All tests finished
```

这些测试能够提高程序的可靠性。

== 实验感想 <thought>

1. 使用 C 进行内存管理、写代码确实是一件非常麻烦的事情，写编译器的过程中会受到很多与编译器主要逻辑无关的杂音的干扰。写 C 时，内存安全的正确性主要依靠 asan、valgrind 等进行检查。如果可以，还是希望编译原理实验尽早换成一个更高级的语言（如 #box[C++], Rust）。
2. Bison 中的错误处理感觉常常没有一个比较好的标准，希望讲义在这方面能够更加详细一些；目前完善错误处理的方式是不断地进行 benchmark 以提高错误处理能力。
