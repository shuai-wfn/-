#include "shell.h"

osSemaphoreId uart_rx_sem;
osThreadId shellTaskHandle;
struct finsh_shell shell;
struct finsh_syscall *_syscall_table_begin  = NULL;
struct finsh_syscall *_syscall_table_end    = NULL;
static uint8_t recv_ch;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    //portBASE_TYPE taskWoken = pdFALSE;

	if(huart->Instance==USART1)
	{
        // osSemaphoreRelease(uart_rx_sem);
        osSignalSet(shellTaskHandle, 0x01);
        // xSemaphoreGiveFromISR(uart_rx_sem, &taskWoken);
        recv_data_cache_input(recv_ch);
		HAL_UART_Receive_IT(&huart1, &recv_ch, 1);
	}
}

struct finsh_syscall* finsh_syscall_next(struct finsh_syscall* call)
{
    unsigned int *ptr;
    ptr = (unsigned int*) (call + 1);
    while ((*ptr == 0) && ((unsigned int*)ptr < (unsigned int*) _syscall_table_end))
        ptr ++;

    return (struct finsh_syscall*)ptr;
}

static int zns_split(char *cmd, uint8_t length, char *argv[FINSH_ARG_MAX])
{
    char *ptr;
    uint8_t position;
    uint8_t argc;
    uint8_t i;

    ptr = cmd;
    position = 0; argc = 0;

    while (position < length)
    {

        while ((*ptr == ' ' || *ptr == '\t') && position < length)
        {
            *ptr = '\0';
            ptr ++; position ++;
        }

        if(argc >= FINSH_ARG_MAX)
        {

            for(i = 0; i < argc; i++)
            {
                //LOG_zns("%s ", argv[i]);
            }
            //LOG_zns("");
            break;
        }

        if (position >= length) break;

        if (*ptr == '"')
        {
            ptr ++; position ++;
            argv[argc] = ptr; argc ++;

            while (*ptr != '"' && position < length)
            {
                if (*ptr == '\\')
                {
                    if (*(ptr + 1) == '"')
                    {
                        ptr ++; position ++;
                    }
                }
                ptr ++; position ++;
            }
            if (position >= length) break;

            *ptr = '\0'; ptr ++; position ++;
        }
        else
        {
            argv[argc] = ptr;
            argc ++;
            while ((*ptr != ' ' && *ptr != '\t') && position < length)
            {
                ptr ++; position ++;
            }
            if (position >= length) break;
        }
    }

    return argc;
}

static cmd_function_t zns_get_cmd(char *cmd, int size)
{
    struct finsh_syscall *index;
    cmd_function_t cmd_func = NULL;

    for (index = _syscall_table_begin;index < _syscall_table_end;index++)
    {
        if (strncmp(index->name, "__cmd_", 6) != 0) continue;

        if (strncmp(&index->name[6], cmd, size) == 0 && index->name[6 + size] == '\0')
        {
            cmd_func = (cmd_function_t)index->func;

            break;
        }
    }

    return cmd_func;
}

static int _zns_exec_cmd(char *cmd, uint8_t length, int *retp)
{
    int argc;
    uint8_t cmd0_size = 0;
    cmd_function_t cmd_func;
    char *argv[FINSH_ARG_MAX];

    while ((cmd[cmd0_size] != ' ' && cmd[cmd0_size] != '\t') && cmd0_size < length)
    cmd0_size ++;

    if (cmd0_size == 0)
        return -1;

    cmd_func = zns_get_cmd(cmd, cmd0_size);
    if (cmd_func == NULL)
        return -1;

    memset(argv, 0x00, sizeof(argv));
    argc = zns_split(cmd, length, argv);

    if (argc == 0)
        return -1;

    *retp = cmd_func(argc, argv);

    return 0;
}

static int zns_exec(char *cmd, uint8_t length)
{
    int cmd_ret;

    if (length==0)
    {
        return -1;
    }

    while ((length > 0) && (*cmd  == ' ' || *cmd == '\t'))
    {
        cmd++;
        length--;
    }

    if (_zns_exec_cmd(cmd, length, &cmd_ret) == 0)
    {
        return cmd_ret;
    }

    printf("cmd:%s\r\n", cmd);
    printf("command not found...\r\n");

    return -1;
}

static int str_common(const char *str1, const char *str2)
{
    const char *str = str1;

    while ((*str != 0) && (*str2 != 0) && (*str == *str2))
    {
        str ++;
        str2 ++;
    }

    return (str - str1);
}

static void zns_auto_complete(char *prefix)
{
    int length, min_length;
    const char *name_ptr, *cmd_name;
    struct finsh_syscall *index;

    min_length = 0;
    name_ptr = NULL;

    if (*prefix == '\0')
    {
        help(0, NULL);
        return;
    }

    /* checks in internal command */
    {
        for (index = _syscall_table_begin; index < _syscall_table_end; FINSH_NEXT_SYSCALL(index))
        {
            /* skip finsh shell function */
            if (strncmp(index->name, "__cmd_", 6) != 0) continue;

            cmd_name = (const char *) &index->name[6];
            if (strncmp(prefix, cmd_name, strlen(prefix)) == 0)
            {
                if (min_length == 0)
                {
                    /* set name_ptr */
                    name_ptr = cmd_name;
                    /* set initial length */
                    min_length = strlen(name_ptr);
                }

                length = str_common(name_ptr, cmd_name);
                if (length < min_length)
                    min_length = length;

                printf("%s\r\n", cmd_name);
            }
        }
    }

    /* auto complete string */
    if (name_ptr != NULL)
    {
        strncpy(prefix, name_ptr, min_length);
    }

    return ;
}

static void shell_auto_complete(char *prefix)
{
    printf("\r\n");

    zns_auto_complete(prefix);

    printf("zns >%s", prefix);
}

void shellTask(void const * argument)
{

    for(;;)
    {
        // osSemaphoreWait(uart_rx_sem,osWaitForever);
        osSignalWait (0x00, osWaitForever);
        while(cache.ptr > 0)
        {
            // if (shell.recv_data < 0)
            // {
            //     continue;
            // }

            /*
            * handle control key
            * up key  : 0x1b 0x5b 0x41
            * down key: 0x1b 0x5b 0x42
            * right key:0x1b 0x5b 0x43
            * left key: 0x1b 0x5b 0x44
            */
            recv_data_cache_output(&shell.recv_data);
            if (shell.recv_data == 0x1b)
            {
                shell.stat = WAIT_SPEC_KEY;
                continue;
            }
            else if (shell.stat == WAIT_SPEC_KEY)
            {
                if (shell.recv_data == 0x5b)
                {
                    shell.stat = WAIT_FUNC_KEY;
                    continue;
                }

                shell.stat = WAIT_NORMAL;
            }
            else if (shell.stat == WAIT_FUNC_KEY)
            {
                shell.stat = WAIT_NORMAL;

                if (shell.recv_data == 0x41) /* up key */
                {
                    printf("recv up key\r\n");
                    continue;
                }
                else if (shell.recv_data == 0x42) /* down key */
                {
                    printf("recv down key\r\n");
                    continue;
                }
                else if (shell.recv_data == 0x44) /* left key */
                {
                    printf("recv left key\r\n");
                    continue;
                }
                else if (shell.recv_data == 0x43) /* right key */
                {
                    printf("recv right key\r\n");
                    continue;
                }
            }

            /* received null or error */
            if (shell.recv_data == '\0' || shell.recv_data == 0xFF) continue;
            /* handle tab key */
            else if (shell.recv_data == '\t')
            {
                int i;
                /* move the cursor to the beginning of line */
                for (i = 0; i < shell.line_curpos; i++)
                    printf("\b");

                /* auto complete */
                shell_auto_complete((char *)&shell.line[0]);
                /* re-calculate position */
                shell.line_curpos = shell.line_position = strlen((const char *)shell.line);

                continue;
            }
            /* handle backspace key */
            else if (shell.recv_data == 0x7f || shell.recv_data == 0x08)
            {
                /* note that shell.line_curpos >= 0 */
                if (shell.line_curpos == 0)
                    continue;

                shell.line_position--;
                shell.line_curpos--;

                if (shell.line_position > shell.line_curpos)
                {
                    int i;

                    memmove(&shell.line[shell.line_curpos],
                            &shell.line[shell.line_curpos + 1],
                            shell.line_position - shell.line_curpos);
                    shell.line[shell.line_position] = 0;

                    printf("\b%s  \b", &shell.line[shell.line_curpos]);

                    /* move the cursor to the origin position */
                    for (i = shell.line_curpos; i <= shell.line_position; i++)
                        printf("\b");
                }
                else
                {
                    printf("\b \b");
                    shell.line[shell.line_position] = 0;
                }

                continue;
            }

            /* handle end of line, break */
            if (shell.recv_data == '\r' || shell.recv_data == '\n')
            {
                // printf("handle end of line\r\n");
                // shell_push_history(&shell);
#if SHELL_ECHO_MODE == 1
                    printf("\r\n");
#endif
                zns_exec((char *)shell.line, shell.line_position);

                printf(FINSH_PROMPT);
                memset(shell.line, 0, sizeof(shell.line));
                shell.line_curpos = shell.line_position = 0;

                continue;
            }
            /* it's a large line, discard it */
            if (shell.line_position >= FINSH_CMD_SIZE)
                shell.line_position = 0;

            /* normal character */
            if (shell.line_curpos < shell.line_position)
            {
                int i;

                memmove(&shell.line[shell.line_curpos + 1],
                        &shell.line[shell.line_curpos],
                        shell.line_position - shell.line_curpos);
                shell.line[shell.line_curpos] = shell.recv_data;
#if SHELL_ECHO_MODE == 1
                printf("%s", &shell.line[shell.line_curpos]);
#endif

                /* move the cursor to new position */
                for (i = shell.line_curpos; i < shell.line_position; i++)
                    printf("\b");
            }
            else
            {
                shell.line[shell.line_position] = shell.recv_data;
#if SHELL_ECHO_MODE == 1
                    printf("%c", shell.recv_data);
#endif
            }

            shell.recv_data = 0;
            shell.line_position ++;
            shell.line_curpos++;
            if (shell.line_position >= FINSH_CMD_SIZE)
            {
                /* clear command line */
                shell.line_position = 0;
                shell.line_curpos = 0;
            }
            // printf("%c", shell.recv_data);
            // osDelay(1000);
        }
        
    }
}

static void finsh_system_function_init(const void *begin, const void *end)
{
    _syscall_table_begin = (struct finsh_syscall *) begin;
    _syscall_table_end = (struct finsh_syscall *) end;
}

const unsigned char head[11][16] =
    {
       {0x00,0x00,0x00,0x7E,0x06,0x04,0x0C,0x08,0x10,0x30,0x20,0x60,0x7E,0x7E,0x00,0x00},/*"Z",0*/

       {0x00,0x00,0x00,0x42,0x62,0x62,0x72,0x52,0x5A,0x4A,0x4E,0x46,0x46,0x46,0x00,0x00},/*"N",1*/

       {0x00,0x00,0x00,0x10,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},/*"'",2*/

       {0x00,0x00,0x00,0x3C,0x66,0x46,0x60,0x30,0x1C,0x06,0x42,0x42,0x66,0x3C,0x00,0x00},/*"S",3*/

       {0x00,0x00,0x00,0x40,0x40,0x40,0x40,0x5E,0x62,0x42,0x42,0x42,0x42,0x42,0x00,0x00},/*"h",4*/

       {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x46,0x7E,0x40,0x42,0x66,0x1C,0x00,0x00},/*"e",5*/

       {0x00,0x00,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00},/*"l",6*/

       {0x00,0x00,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00},/*"l",7*/
};
//===============================================================

/**
 * This function will show the version of rt-thread rtos
 */
void show_version(void)
{
    {
        unsigned char i = 0, j = 0, k = 0;

        for (k = 0; k < 16; k++)
        {
            printf(" ");
            for (j = 0; j < 11; j++)
            {
                for (i = 0; i < 8; i++)
                {
                    if (head[j][k] & (0X80 >> i))
                        printf("%c", "ZN'Shell"[j]);
                    else
                        printf(" ");
                }
            }
            printf(" \r\n");
        }
    }
		
		printf("             ZN'Shell (ZNS) v2.31beta    Powered By YuZhennan\r\n\r\n");
}

void shell_init(void)
{
	//   HAL_Delay(8000);
	  
    show_version();
    // printf("shell_init\r\n");
    data_cache_init();

    shell.stat = WAIT_NORMAL;
    finsh_system_function_init(&FSymTab$$Base, &FSymTab$$Limit);

    osSemaphoreDef(uart_rx_sem);
    uart_rx_sem = osSemaphoreCreate(osSemaphore(uart_rx_sem),1);

    osThreadDef(shellTask, shellTask, osPriorityNormal, 0, 128);
    shellTaskHandle = osThreadCreate(osThread(shellTask), NULL);
	
	  printf("here! %d \r\n",shellTaskHandle);

    HAL_UART_Receive_IT(&huart1, &shell.recv_data, 1);
}
