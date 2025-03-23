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


= #text(font: ("Libertinus Serif", "STZhongsong"))[编译原理 Project 2 实验报告]

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

我在项目中实现了编译原理 Project 2 的全部内容，包括：
- 符号表与类型系统的建立（@symtab[Sec]）；
- 语义分析的实现（@semantic[Sec]）。

=== 符号表与类型系统 <symtab>

符号表的实现使用了在 Project 1 中就预先实现的平衡二叉搜索树（用 Treap 实现），实现了从 `HString` 到 `SymbolEntry` 的映射：

```c
typedef struct HString {
    String s;
    u64 stored_hash;
} HString;

typedef struct SymbolEntry {
    SymbolEntryKind kind;
    usize type_idx;

    struct SymbolTable *table;
    String *name;
    union {
        struct {
            bool is_defined;
            int first_decl_line_no;
        } as_fun;
    };
} SymbolEntry;
```
其中 `HString` 是缓存了哈希值的 `String`（`String` 是可变长的、类似于 C++ 中 `std::string` 的字符串，是以在 Project 1 中实现的、以宏作为模板的 `Vector` 为基础实现的），缓存哈希主要是为了能够则平衡二叉搜索树中快速比较；`SymbolEntry` 主要包括了一个表示该符号对应类型的 `type_idx`，以及其他所需信息。

而*每个*符号表则是以从 `HString` 到 `SymbolEntry` 的 Mapping 为基础实现的，不同的符号表构成父子关系，用以支持变量作用域的嵌套：
```c
typedef struct SymbolTable {
    usize parent_idx; // father symtab
    MapSymtab mapping;
} SymbolTable;
```

而类型系统则是以 `Type` 结构体为基础实现的：
```c
typedef struct Type {
    TypeKind kind;
    usize repr_val; // representive index; for fast equal test. See Sec 4.2
    union {
        struct {
            usize size, dim, subtype_idx;
        } as_array;
        struct {
            VecUSize field_idxes;
            usize symtab_idx;
        } as_struct;
        struct {
            VecUSize ret_par_idxes; // the 0th element is the return type
        } as_fun;
    };
} Type;
```

值得注意的是，在符号表和类型系统中，指代其他的符号表/类型的索引均使用 `usize`；这是因为我使用了可变长的 `Vector` 来进行符号表/类型管理。该 `Vector` 可能由于扩容而导致指针失效，故使用整数索引较为稳妥。

=== 语义分析 <semantic>

基于这一套符号表和类型系统，我实现了语义分析，主要包括了：

- 遍历 AST，在该过程中按照语义，传递各种 attributes，从而适当地进行符号表嵌套、类型生成（`Struct` 和 `Fun` 类型）和符号表填充；
- 当遇到语法错误时，按照规定进行报错。


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

=== 应用“空类型”减少非本质报错

在计算过程中，若发现了错误，我希望能够在报错时尽可能减少因该错误而连带引起的错误。解决方法是引入空类型 `void`（其实相当于 bottom 类型 $bot$)：在语义分析出现错误时，程序会报错，并将可能产生的“结果”（如 Exp 的求值，StructSpecifier 的解析等）的类型设为 `void`。在类型相容性检查时给予 `void` 较高的兼容性以减少连锁报错。

=== 高效进行类型相容性检查

由于按照讲义的编译器实现中，对类型相容性检查需要递归进行（例如对两个 `Struct` 的相容性判断需要递归地对每个 Field 依次判断，对两个 `Fun` 的相容性判断需要递归地对参数和返回值进行判断），在一些极端情况下会导致低效。例如，在以下构造的样例中会导致指数型的判断递归：

```c
struct A0{
  int val;
};
struct A1{
	struct A0 v0, v1;
};
struct A2{
	struct A1 v0, v1;
};
// ...
struct A2999{
	struct A2998 v0, v1;
};
int main(){
  struct A2999 a, b; struct A2997 c; struct A2997 d; struct A2998 e;
  a = b; a = c; b = c; a = d; b = d; c = d; a = e; b = e; c = e; d = e;
}
```

为了解决这个问题，我为每个类型引入了一个 `repr_val`，用以快速判断两个类型是否相容。所有相容的类型都有相同的 `repr_val`。这使得能够在常数时间内判断两个类型是否相同。

具体做法是，维护一个 `Type` 到 `usize` 的 Map（使用平衡二叉搜索树实现；实际键值为 `HType`，即缓存了哈希的 `Type`，用以提高搜索效率），用以维护不同类型的等价类（以相容性为等价关系）。在一个类型构造完成时（如 `Struct` 添加了所有的 Field 后，或是 `Fun` 添加了所有返回值和参数后），会试图在该 Map 中寻找是否有与之相容的类型；若找到了，则将 `repr_val` 设为 Map 中对应的值；否则新建一个独特的值并插入 Map 中。而“在该 Map 中寻找是否有与之相容的类型”的操作则也较为简单：对于普通类型，直接在 Map 中进行比对即可；对于包含其他类型的复合类型，如 `Array`, `Struct` 和 `Fun`，则将其所包含的所有子类型先改写成其对应的 `repr_val`，再将改写后的类型插入 Map。该算法其实是龙书第六章对应算法的一个实现。

== 实验感想 <thought>

这次实验相比上一次实验，正确性的不确定性小了很多（语义分析有着更为明确的规范），但是实现的复杂度也有所增加。我在 Project 1 中实现的 `Vector`, `Mapping`, `String` 等基础设施，以及一套在 C 语言中实现的面向对象原语派上了用场，能够实现在任何输入下的*无内存泄漏*、*高正确性*、*高效率*的编译器。两次实验我的工作量增量大概如下(含 `.c`, `.h`, `.l`, `.y` 的、非自动生成的文件)：
- Project 1: 2328 行；
- Project 2: 2252 行。

使用 C 进行编译器实现，经常会出现 Segmentation Fault；使用 gdb 能较为高效地进行查错。
