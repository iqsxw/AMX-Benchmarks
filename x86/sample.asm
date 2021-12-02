SECTION .data
SECTION .text

%macro DEFINE_ARG 2
    %define %1q r%2
    %define %1d e%2
    %define %1w %2
%endmacro

%macro DEFINE_ARGS 0-10+
%if %0
    DEFINE_ARG %1, di
%rotate 1
%endif
%if %0
    DEFINE_ARG %1, si
%rotate 1
%endif
%if %0
    DEFINE_ARG %1, dx
%rotate 1
%endif
%if %0
    DEFINE_ARG %1, cx
%rotate 1
%endif
%endmacro

%macro cglobal 1-2+
    global %1
    %1:
    DEFINE_ARGS %2
%endmacro

global add_avx512

cglobal AddAVX512, a, b, c
    vmovdqa32      zmm0,  [aq]
    vpaddd         zmm0,  zmm0, [bq]
    vmovdqa32      [cq],  zmm0
    ret

cglobal amx_matrix_mul_zero, cfg, a, b, c
    xor rax, rax
    mov rax, 64

    ldtilecfg      [cfgq]
    tilezero       tmm0

    tileloadd      tmm1, [aq + rax]
    tileloadd      tmm2, [bq + rax]
    tdpbsud        tmm0, tmm1, tmm2

    tilestored     [cq + rax], tmm0

    tilerelease
    ret

cglobal amx_matrix_mul, cfg, a, b, c
    xor rax, rax
    mov rax, 64

    ldtilecfg      [cfgq]
    tileloadd      tmm0, [cq + rax]

    tileloadd      tmm1, [aq + rax]
    tileloadd      tmm2, [bq + rax]
    tdpbsud        tmm0, tmm1, tmm2

    tilestored     [cq + rax], tmm0

    tilerelease
    ret

cglobal null_test, null
    ret
