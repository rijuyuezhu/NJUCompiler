#pragma once

typedef enum ArithopKind {
    ArithopAdd,
    ArithopSub,
    ArithopMul,
    ArithopDiv,
} ArithopKind;

typedef enum RelopKind {
    RelopGT,
    RelopLT,
    RelopGE,
    RelopLE,
    RelopEQ,
    RelopNE,
} RelopKind;
