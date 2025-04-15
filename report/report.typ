#import "@preview/numbly:0.1.0": numbly

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


= #text(font: ("Libertinus Serif", "STZhongsong"))[编译原理 Project 3 实验报告]

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

我在项目中实现了编译原理 Project 3 的全部内容，包括：
- 中间代码的程序内表示和输出转换（@ir[Sec]）；
- 源代码到中间代码的转换（@genir[Sec]）；

=== 中间代码的程序内表示和输出转换 <ir>

在程序内我希望使用一种简单的编码方式来进行中间代码的表示和保存，从而可以方便后续的处理。我采用的是两层抽象，第一层抽象“地址”（即下文中的 `IREntity`），第二层则抽象 IR（即下文中的 `IR`）:

```c
typedef enum IREntityKind {
    IREntityInvalid,
    IREntityImmInt, // starts with #
    IREntityVar,    // starts with v
    IREntityAddr,   // starts with &v
    IREntityDeref,  // starts with *v
    IREntityLabel,  // starts with l
    IREntityFun,    // fun_name->name
} IREntityKind;

typedef struct IREntity {
    IREntityKind kind;
    union {
        int imm_int;             // for IREntityImmInt
        usize var_idx;           // for IREntityVar, IREntityAddr, IREntityDeref
        usize label_idx;         // for IREntityLabel
        struct String *fun_name; // for IREntityFun
    };
} IREntity;

typedef struct IR {
    IRKind kind;
    IREntity e1;
    IREntity e2;
    IREntity ret;

    union {
        ArithopKind arithop_val;
        RelopKind relop_val;
    };
} IR;
```

之后，使用 `Vector`（详见 Project 1 实验报告）来维护 IR 的列表。为了将 IR 输出，我实现了成员函数

```c
void MTD(IR, build_str, /, struct String *builder);
```

它可以递增地将当前 IR 产生的字符串接到 `builder` 后（`builder` 的类型是基于 `Vector` 的可变长 `String`）。

=== 源代码到中间代码的转换 <genir>

为了在类型检查的基础上，继续实现中间代码的生成，我在类型检查*之后*，再遍历了一次 Parse Tree。为了方便效率的提高，对于某些结点（如
`ExpDecList`, `ParamDec`, `Dec`, `FunDec` 和一些 `Exp`），我在类型检查的过程中记录了它们的符号表 `Entry` 信息，从而避免了在生成 IR 的过程中再次查询符号表。

== 程序编译指令 <compile>

对于根目录如图的项目:

#figure(```plain
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
```)

运行 `make -C ./Code` 即可编译项目，得到的可执行文件位于 `./Code/parser`。其中 Makefile 和下发的 Makefile 一致。

== 其它亮点 <other>

=== "CALL Matcher in C" neovim 插件的开发 <neovim>

在 Project 1 中，我提到，为了方便程序的编写，我在 C 中用宏实现了一套 OOP 原语，且使用类似 Rust 的移动语义来实现了（手动的）内存管理与 RAII。这一套语法系统主要包括：

- `CALL(class, name, method, /, params)` 用来进行方法调用，类似于 `name.method(params)` 的用法，其中 `name` 的类型是 `class`；
- `NSCALL(class, method, /, params)` 用来进行静态方法调用，类似于 `class::method(params)` 的用法；
- `MTD(class, method, /, params)` 用来进行方法声明和定义，在方法定义内部可以使用类型为 `class *` 的变量 `self` 来指代当前对象；
- `NSMTD(class, method, /, params)` 用来进行静态方法声明和定义。

然而这一套宏在使用过程中可读性较差。为了方便程序的编写，我开发了对这种"DSL"的自动解析的 neovim 插件 "Call Matcher in C"#footnote[#link("https://github.com/rijuyuezhu/call-matcher-in-C.nvim")]，它可以将上述宏对应的 OOP 写法显示在 neovim 的虚拟行中，效果类似于：

#figure(image("1.png", width: 70%))

这可以方便地解析这一种 DSL。


== 实验感想 <thought>

这一次实验较为简单，再次遍历 Parse Tree 的过程和 Type Checking 的过程类似，中间代码的生成的过程也有详细的讲义。三次 Project 的代码增量（含 `.c`, `.h`, `.l`, `.y` 的、非自动生成的文件）如下：

- Project 1: 2328 行；
- Project 2: 2203 行；
- Project 3: 1643 行。
