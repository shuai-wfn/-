#ifndef _SHELL_H
#define _SHELL_H

#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "stdio.h"
#include "string.h"
#include "data_cache.h"

#define SHELL_ECHO_MODE 1
#ifndef FINSH_CMD_SIZE
#define FINSH_CMD_SIZE      80
#endif
#define FINSH_HISTORY_LINES 5
#define FINSH_ARG_MAX    8

#define FINSH_PROMPT "zn'shell\\>"

enum input_stat
{
    WAIT_NORMAL,
    WAIT_SPEC_KEY,
    WAIT_FUNC_KEY,
};
struct finsh_shell
{
    enum input_stat stat;

    uint8_t line[FINSH_CMD_SIZE + 1];
    uint16_t line_position;
    uint16_t line_curpos;

    uint16_t current_history;
    uint16_t history_count;
    char cmd_history[FINSH_HISTORY_LINES][FINSH_CMD_SIZE];

    uint8_t recv_data;
};

#define FINSH_NEXT_SYSCALL(index)  index=finsh_syscall_next(index)

typedef long (*syscall_func)(void);

/* system call table */
struct finsh_syscall
{
    const char*     name;       /* the name of system call */
    const char*     desc;       /* description of system call */
    syscall_func    func;      /* the function address of system call */
};

#define __USED__	__attribute__((__used__))
#define __SECTION(x)  __attribute__((section(x)))

#define ZNS_CMD_EXPORT(command, desc)   \
    FINSH_FUNCTION_EXPORT_CMD(command, __cmd_##command, desc)

#define FINSH_FUNCTION_EXPORT_CMD(name, cmd, desc)                      \
   const char __fsym_##cmd##_name[] __SECTION(".rodata.name") = #cmd;    \
   const char __fsym_##cmd##_desc[] __SECTION(".rodata.name") = #desc;   \
   __USED__ const struct finsh_syscall __fsym_##cmd __SECTION("FSymTab")= \
   {                        \
    __fsym_##cmd##_name,    \
    __fsym_##cmd##_desc,    \
    (syscall_func)&name     \
   };

typedef int (*cmd_function_t)(int argc, char **argv);

extern const int FSymTab$$Base;
extern const int FSymTab$$Limit;
extern UART_HandleTypeDef huart1;
extern struct finsh_syscall *_syscall_table_begin;
extern struct finsh_syscall *_syscall_table_end;

extern void shell_init(void);
struct finsh_syscall* finsh_syscall_next(struct finsh_syscall* call);
extern int help(int argc, char **argv);

#endif
