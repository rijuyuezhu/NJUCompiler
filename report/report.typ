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


= #text(font: ("Libertinus Serif", "STZhongsong"))[编译原理 Project 4 实验报告]

#align(center)[
  #v(1em)
  #text(font: "Kaiti")[黄文睿] \
  221180115
]

== 概览 <intro>

- 程序实现功能：实现了所有需实现内容，详见 @impl[Sec]。
- 程序编译指令：和下发的 Makefile 保持一致，详见 @compile[Sec]。
- 实验感想：详见 @thought[Sec]。

== 程序实现功能 <impl>

我在项目中实现了编译原理 Project 4 的全部内容，即从 IR 生成能够在 spim 上运行的 Mips32 汇编代码。

+ *指令选择*: 主要采用了模板法进行指令选择，对每一种 IR 生成相应的汇编：
  #table(
    columns: (1fr, 1.8fr),
    align: horizon,
    table.header[*IR*][*MIPS instructions*],
    [`LABEL l1:`], [`_L1:`],
    [`FUNCTION foo:`],
    [
      `_Ffoo:`
      #footnote[此处加上前缀 `_F` 是因为 spim 的一些 parse 问题，比如 label 名为 `add` 会报错。`_R` 则标记了 Epilogue 的起始处，用于 return 的跳转。]\
      #h(1em)Prologue\
      #h(1em)Asm of body\
      `_Rfoo:`\
      #h(1em)Epilogue],

    [`x := y`], [`move reg(x), reg(y)`],
    [`x := #imm`], [`li reg(x), imm`],
    [`x := &y`], [`addi reg(x), $fp, offset(y)`],
    [`x := *y`], [`lw reg(x), 0(reg(y))`],
    [`*x := y`], [`sw reg(y), 0(reg(x))`],
    [
      `x := oprd1 op oprd2`\
      其中 `oprd1` and `oprd2` 可以是变量或立即数；\
      `op` 可以是 `+`, `-`, `*`, `/`。\
    ],
    [
      对于立即数操作数，先用 `li` 指令将其加载到寄存器中。\
      `add/sub/mul reg(x), reg(oprd1), reg(oprd2)`\
      对于除法：\
      `div reg(oprd1), reg(oprd2)`\
      `mflo reg(x)`
    ],

    [`GOTO l1`], [`j _L1`],
    [
      `IF oprd1 rop oprd2 GOTO l1`\
      其中 `oprd1` and `oprd2` 可以是变量或立即数。
    ],
    [
      对于立即数操作数，先用 `li` 指令将其加载到寄存器中。\
      `beq/bne/bgt/blt/bge/ble reg(oprd1), reg(oprd2), _L1`\
    ],

    [`RETURN x`], [`move $v0, reg(x)`\ `j _Rfoo`],
    [`DEC x size`], [生成 `offset(x)`，且分配 `size` 大小的空间],
    [`ARG x`], [$"arglist"."append"(x)$],
    [`x := CALL bar`],
    [
      保存 Caller saved registers;\
      减少 `sp` 用于存放 $"pos">= 5$ 的参数\
      从 $"arglist"$ 末尾弹出 $"arity"("bar")$ 个参数\
      将参数放置到 `$a0~$a3` 寄存器和栈上\
      `jal _Fbar`\
      恢复 `sp`
    ],

    [`PARAM x`], [按照 $x$ 的编号，记录其在寄存器/栈上的地址],
    [`READ, WRITE`], [包装成函数处理],
  )
+ *寄存器分配*: 采用了局部寄存器分配的方法，在每一个基块中进行寄存器的分配（使用 LRU）；在基块末尾进行写回。


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

== 实验感想 <thought>

这一次实验较为简单，只需要按照 Mips32 的 ABI 进行代码的生成即可，也没有写复杂的寄存器分配策略。四次 Project 的代码增量（含 `.c`, `.h`, `.l`, `.y` 的、非自动生成的文件）如下：

- Project 1: 2328 行；
- Project 2: 2204 行；
- Project 3: 1643 行。
- Project 4: 846 行。
