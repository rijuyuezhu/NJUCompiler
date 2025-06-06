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

#show link: set text(fill: blue)

= #text(font: ("Libertinus Serif", "STZhongsong"))[编译原理 Project 5 实验报告]

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

我在项目中实现了编译原理 Project 5 的全部内容，包括:

/ 选做 (6.1): 基于数据流分析的全局优化，包括常量折叠（基于常量传播），复制折叠（基于复制传播），公用表达式消除（基于可用表达式传播），死代码消除（基于活跃变量分析），控制流优化（基于常量传播和 DFS）。
/ 选做 (6.2): 循环不变式外提，主要参考了 #link("https://www.cs.cmu.edu/afs/cs/academic/class/15745-s19/www/lectures/L9-LICM.pdf")。
/ 选做 (6.3): 循环归纳变量强度削减，只要参考了 #link("https://www.cs.cmu.edu/afs/cs/academic/class/15745-s19/www/lectures/L8-Induction-Variables.pdf")。
/ 其他: 除此以外，还实现了一些窥孔优化和 Label 的消除等，进一步提升了基本块内的优化效果。


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

本次实验我并未使用框架，而是从头实现了所有部分，所有代码（不含自动生成的代码）共 7825 行。

大部分优化都可以按部就班地按照定义去实现，故在此省略。

值得一提的是全局的优化管线，我最终使用了如下的优化管线：

```c
void MTD(IROptimizer, optimize_func, /, IRFunction *func) {
    const usize TIMES = 6;
    for (usize i = 0; i < TIMES; i++) {
        if (i >= TIMES / 2) {
            CALL(IROptimizer, *self, optimize_func_peephole, /, func);
            CALL(IROptimizer, *self, optimize_func_copy_prop, /, func);
            CALL(IROptimizer, *self, optimize_func_loop_ind_var, /, func);
            CALL(IROptimizer, *self, optimize_func_licm, /, func);
            CALL(IROptimizer, *self, optimize_func_dead_code_eliminate, /,
                 func);
        }
        CALL(IROptimizer, *self, optimize_func_const_prop, /, func);
        CALL(IROptimizer, *self, optimize_func_control_flow, /, func);
        CALL(IROptimizer, *self, optimize_func_simple_redundant_ops, /, func);
        CALL(IROptimizer, *self, optimize_func_peephole, /, func);
        CALL(IROptimizer, *self, optimize_func_simple_redundant_ops, /, func);
        CALL(IROptimizer, *self, optimize_func_copy_prop, /, func);
        CALL(IROptimizer, *self, optimize_func_avali_exp, /, func);
        CALL(IROptimizer, *self, optimize_func_copy_prop, /, func);
        CALL(IROptimizer, *self, optimize_func_dead_code_eliminate, /, func);
    }

    CALL(IROptimizer, *self, optimize_func_simple_redundant_ops, /, func);
    CALL(IROptimizer, *self, optimize_func_peephole, /, func);
    CALL(IROptimizer, *self, optimize_func_simple_redundant_ops, /, func);
    CALL(IROptimizer, *self, optimize_func_copy_prop, /, func);

    usize UP_LIMIT = 50;
    for (usize i = 0; i < UP_LIMIT; i++) {
        bool updated = CALL(IROptimizer, *self,
                            optimize_func_dead_code_eliminate, /, func);
        if (!updated) {
            break;
        }
    }

    for (usize i = 0; i < 5; i++) {
        CALL(IROptimizer, *self, optimize_func_useless_label_strip, /, func);
        CALL(IROptimizer, *self, optimize_func_dead_code_eliminate, /, func);
    }
}
```
