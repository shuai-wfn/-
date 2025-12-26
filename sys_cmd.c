#include "shell.h"

int help(int argc, char **argv)
{
    printf("shell commands:\r\n");
    {
        struct finsh_syscall *index;

        for (index = _syscall_table_begin;
                index < _syscall_table_end;
                FINSH_NEXT_SYSCALL(index))
        {
            if (strncmp(index->name, "__cmd_", 6) != 0) continue;
                printf("%-16s - %s\r\n", &index->name[6], index->desc);
        }
    }
    printf("\r\n");

    return 0;
}
ZNS_CMD_EXPORT(help, All supported commands);

//这里使用了二维码QR编码算法，感兴趣的可以加我要源码
//振南VX ZN_1234  TEL:13683697439

//★代码和SHELL中嵌入二维码，可以让调试更加方便★

const uint32_t qrcode[25]= {0XFEF2BF80,0X82C12080,0XBAE7AE80,0XBA5F2E80,0XBACEAE80,0X82B12080,0XFEAABF80,0X009E8000, \
	                          0X23C77B80,0X91212980,0X9E80C680,0X7D514D80,0X1E5DFD00,0X09BB9D80,0X9E9D3080,0XED10E180, \
	                          0X0FFDFF80,0X00FD8800,0XFEDCA980,0X82628C80,0XB931FF00,0XBA0CFD00,0XBAA20400,0X823B6980, \
	                          0XFE6E7280};

int iam(int argc, char **argv)
{	
  printf("I am ZN'SHELL (CLI),Here's ABOUT ME!\r\n");
  
	{
	 uint8_t i = 0 , j = 0;
		
	 printf("此二维码使用了活码技术，请放心扫码!\r\n\r\n");
		
	 printf("★ ★声明：此代码版权归 振南电子 及 PN学堂 所有★ ★ \r\n");
		
	 printf("www.znmcu.com   www.pnxuetang.com\r\n\r\n\r\n");
		
	 for(i=0;i<25;i++)
	 {
		printf("                ");
		for(j=0;j<32;j++)
		 printf((qrcode[i]&(0X80000000>>j))?"▄ ":"  ");
		 
		printf("\r\n");
	 }
	 
	 printf("\r\n\r\n");
	}

 return 0;
}

ZNS_CMD_EXPORT(iam, I am ZN_SHELL);
