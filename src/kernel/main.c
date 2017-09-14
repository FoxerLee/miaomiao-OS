
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


PRIVATE void Toy_shell	(const char * tty_name);
PRIVATE int  help       ();
PRIVATE int pro_details ();
PRIVATE int currentTime ();
PRIVATE int get_commands(int argc, char* argv[]);
PRIVATE int showLicense ();
PRIVATE int pause		(int argc, char * argv[]);
PRIVATE int resume		(int argc, char * argv[]);
PRIVATE int kill		(int argc, char * argv[]);
/*****************************************************************************
 *                               kernel_main
 *****************************************************************************/
/**
 * jmp from kernel.asm::_start. 
 * 
 *****************************************************************************/
PUBLIC int kernel_main()
{
	//disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
	//	 "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	int i, j, eflags, prio;
        u8  rpl;
        u8  priv; /* privilege */

	struct task * t;
	struct proc * p = proc_table;

	char * stk = task_stack + STACK_SIZE_TOTAL;

	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++,t++) {
		if (i >= NR_TASKS + NR_NATIVE_PROCS) {
			p->p_flags = FREE_SLOT;
			continue;
		}

	    if (i < NR_TASKS) {     /* TASK */
            t       = task_table + i;
            priv    = PRIVILEGE_TASK;
            rpl     = RPL_TASK;
            eflags  = 0x1202;/* IF=1, IOPL=1, bit 2 is always 1 */
			prio    = RR_TIME;
			p->priority = 1;
        }
        else {                  /* USER PROC */
            t	    = user_proc_table + (i - NR_TASKS);
            priv	= PRIVILEGE_USER;
            rpl     = RPL_USER;
            eflags  = 0x202;	/* IF=1, bit 2 is always 1 */
			prio    = RR_TIME * 4;
			p->priority = 0;
        }

		strcpy(p->name, t->name);	/* name of the process */
		p->p_parent = NO_TASK;

		if (strcmp(t->name, "INIT") != 0) {
			p->ldts[INDEX_LDT_C]  = gdt[SELECTOR_KERNEL_CS >> 3];
			p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			/* change the DPLs */
			p->ldts[INDEX_LDT_C].attr1  = DA_C   | priv << 5;
			p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
		}
		else {		/* INIT process */
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret == 0);
			init_desc(&p->ldts[INDEX_LDT_C], 0, (k_base + k_limit) >> LIMIT_4K_SHIFT,
				      DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

			init_desc(&p->ldts[INDEX_LDT_RW], 0, (k_base + k_limit) >> LIMIT_4K_SHIFT,
				      DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
		}

		p->regs.cs = INDEX_LDT_C << 3  | SA_TIL | rpl;
		p->regs.ds = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.es = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.fs = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p->regs.eip	= (u32)t->initial_eip;
		p->regs.esp	= (u32)stk;
		p->regs.eflags	= eflags;

		p->prio = prio;
		p->ticks = RR_TIME;
		

		p->p_flags = 0;
		p->p_msg = 0;
		p->p_recvfrom = NO_TASK;
		p->p_sendto = NO_TASK;
		p->has_int_msg = 0;
		p->q_sending = 0;
		p->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p->filp[j] = 0;

		stk -= t->stacksize;
	}


	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

	init_clock();
    init_keyboard();

	restart();

	while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}

struct time get_time_RTC()
{
	struct time t;
	MESSAGE msg;
	msg.type = GET_RTC_TIME;
	msg.BUF= &t;
	send_recv(BOTH, TASK_SYS, &msg);
	return t;
}


/**
 * @struct posix_tar_header
 * Borrowed from GNU `tar'
 */
struct posix_tar_header
{				/* byte offset */
	char name[100];		/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
	/* 500 */
};

/*****************************************************************************
 *                                untar
 *****************************************************************************/
/**
 * Extract the tar file and store them.
 * 
 * @param filename The tar file.
 *****************************************************************************/
void untar(const char * filename)
{
	printf("[extract `%s'\n", filename);
	int fd = open(filename, O_RDWR);
	assert(fd != -1);

	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);

	while (1) {
		read(fd, buf, SECTOR_SIZE);
		if (buf[0] == 0)
			break;

		struct posix_tar_header * phdr = (struct posix_tar_header *)buf;

		/* calculate the file size */
		char * p = phdr->size;
		int f_len = 0;
		while (*p)
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */

		int bytes_left = f_len;
		int fdout = open(phdr->name, O_CREAT | O_RDWR);
		if (fdout == -1) {
			printf("    failed to extract file: %s\n", phdr->name);
			printf(" aborted]");
			return;
		}
		printf("    %s (%d bytes)\n", phdr->name, f_len);

		while (bytes_left) {
			int iobytes = min(chunk, bytes_left);
			read(fd, buf,
			     ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			write(fdout, buf, iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);
	}

	close(fd);

	printf(" done]\n");
}

/*****************************************************************************
 *                                Toy_shell
 *****************************************************************************/
/**
 * rewrite the shell by Foxerlee
 * 
 * @param tty_name  TTY file name.
 *****************************************************************************/
PRIVATE void Toy_shell(const char * tty_name) {
	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];

	while (1) {
		write(1, "$ ", 2);
		int r = read(0, rdbuf, 70);
		rdbuf[r] = 0;

		int argc = 0;
		char * argv[PROC_ORIGIN_STACK];
		char * p = rdbuf;
		char * s;
		int word = 0;
		char ch;
		do {
			ch = *p;
			if (*p != ' ' && *p != 0 && !word) {
				s = p;
				word = 1;
			}
			if ((*p == ' ' || *p == 0) && word) {
				word = 0;
				argv[argc++] = s;
				*p = 0;
			}
			p++;
		} while(ch);
		argv[argc] = 0;

		int ret = get_commands(argc, argv);
		if(ret != -2) continue;

		int fd = open(argv[0], O_RDWR);
		if (fd == -1) {
			if (rdbuf[0]) {
				write(1, rdbuf, r);
				write(1, ": command not found\n", 20);
			}
		}
		else {
			close(fd);
			int pid = fork();
			if (pid != 0) { /* parent */
				int s;
				wait(&s);
			}
			else {	/* child */
				execv(argv[0], argv);
			}
		}
	}

	close(1);
	close(0);
}

/***********************************************************************
				                clear 
************************************************************************/
PUBLIC void clear() {
	int i = 0;
	disp_pos = 0;
	for(i=0;i<console_table[current_console].cursor;i++){
		disp_str(" ");
	}
	disp_pos = 0;

	console_table[current_console].crtc_start = 0;
	console_table[current_console].cursor = 0;
}

/***********************************************************************
				           printInfo
************************************************************************/
PRIVATE void printInfo() {
	clear();
	 
	char *info ="\n\n"
				"             **                                **                   \n"
	            "             //                                //                    \n"
	            " **********  **  ******    ******  **********  **  ******    ****** \n"
	            "//**//**//**/** //////**  **////**//**//**//**/** //////**  **////**\n"
	            " /** /** /**/**  ******* /**   /** /** /** /**/**  ******* /**   /**\n"
				" /** /** /**/** **////** /**   /** /** /** /**/** **////** /**   /**\n"
				" *** /** /**/**//********//******  *** /** /**/**//********//****** \n"
				"///  //  // //  ////////  //////  ///  //  // //  ////////  //////  \n"
				"                          *******    ********\n"
				"                         **/////**  **////// \n"
				"                        **     //**/**       \n"
				"                       /**      /**/*********\n"
				"                       /**      /**////////**\n"
				"                      //**     **        /**\n"
				"                       //*******   ********  \n"
				"                       ///////   ////////  \n"
	"\n"
	"miaomiao`OS, version 0.8.3-alpha\n"
	"Developed by 1552674 liyuan\n"
	"             1552672 wukefei\n"
	"\n"
	"Press F2 or F3 to console\n";
	 
	

	int ch = 0;
	for(ch = 0; ch < strlen(info); ch++) {
		printf("%c", info[ch]);
		milli_delay(10);
	}


}

/*****************************************************************************
 *                                Init
 *****************************************************************************/
/**
 * The hen.
 * 
 *****************************************************************************/
void Init() {
	int fd_stdin  = open("/dev_tty0", O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	assert(fd_stdout == 1);

	printf("Init() is running ...\n");

	/* extract `cmd.tar' */
	untar("/cmd.tar");

	char * tty_list[] = {"/dev_tty1", "/dev_tty2"};

	int i;
	for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++) {
		int pid = fork();
		if (pid != 0) { /* parent process */
			printf("[parent is running, child pid:%d]\n", pid);
		}
		else {	/* child process */
			//printf("[child is running, pid:%d]\n", getpid());
			close(fd_stdin);
			close(fd_stdout);
			
			Toy_shell(tty_list[i]);
			assert(0);
		}
	}

	milli_delay(10000);
	printInfo();


	while (1) {
		int s;
		int child = wait(&s);
		printf("child (%d) exited with status: %d.\n", child, s);
	}

	assert(0);
}


/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	for(;;) {
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	for(;;) {
		//printf("b");
		//disp_str("b");
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
	for(;;) {
		//printf("c");
		//disp_str("c");
	};
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}


PRIVATE int help() {
	printf("=============================================================================\n"
			"miaomiao`OS, version 0.8.3-alpha\n"
			"These shell commands are defined internally.  Type 'help' to see this list.\n"
			"=============================================================================\n"
			"help      :  See the list of shell commands.\n"
			"license   :  See the license of miaomiao`OS.\n"
			"echo      :  Print the arguments to the screen.\n"
			"time      :  Print OS time.\n"
			"ps        :  Print the status of processes.\n"
			"cpuinfo   :  Show the infomation of CPU.\n"
			"pause     :  Pause a process.\n"
			"resume    :  Resume a process.\n"
			"kill      :  Kill a process.\n"
			"touch     :  Create a file. Only '.txt' file is allowed.\n"
			"rm        :  Delete a file. Can`t delete the system file.\n"
			"mv        :  Move a file.\n"
			"ls        :  List all files.\n"
			"cat       :  Show the content of file.\n"
			"vi        :  File editor.\n"
			"escape    :  A stupid little game.\n"		
			"=============================================================================\n");
	return 0;
}



 PRIVATE int get_commands(int argc, char* argv[]) {
	if(strcmp(argv[0], "help") == 0)
		return help();
	else if(strcmp(argv[0], "time") == 0)
		return currentTime();
	else if(strcmp(argv[0], "ps") == 0)
		return pro_details();
	else if(strcmp(argv[0], "pause") == 0)
		return pause(argc, argv);
	else if(strcmp(argv[0], "kill") == 0)
		return kill(argc, argv);
	else if(strcmp(argv[0], "resume") == 0)
		return resume(argc, argv);
	else if(strcmp(argv[0], "license") == 0)
		return showLicense();
	


	return -2;
}


PRIVATE int currentTime() {
	struct time t = get_time_RTC();
	printf("%d/%d/%d %d:%d:%d\n", t.year, t.month, t.day, t.hour, t.minute, t.second);
	return 0;
}

PRIVATE int showLicense() {
	char *info = "THE miaomiao`OS LICENSE\n"
	"As long as you retain this notice\n" 
	"you can do whatever you want with this stuff"
	"If we meet some day, and you think, this stuff is worth it, \n"
	"you can buy me a Cola in return.\n";

	int ch = 0;
	for(ch = 0; ch < strlen(info); ch++) {
		printf("%c", info[ch]);
		milli_delay(10);
	}

	return 0;
}

PRIVATE int pause(int argc, char * argv[]) {
	struct proc * p = proc_table;
	//char* proc_name = [128];
	//memset(proc_name, 0, 128);
	if (argc == 1) {
		printf("Please input proc name, you can use 'ps' command to see\n");
		return 0;
	}
	int i;
	//for (i = 0; i < strlen(argv[1]); i++) {
	//	proc_name[i] = argv[i];
	//}
	for (i = 0; i < NR_TASKS + NR_NATIVE_PROCS; i++, p++) {
		if (strcmp(argv[1], p->name) == 0) {
			if (i < NR_TASKS) {
				printf("Can`t pause system task!\n");
				return 0;
			}
			else {
				p->p_flags = -1;
				printf("Success!\n");
				return 0;
			}
		}
	}

	printf("Please input correct proc name, you can use 'ps' command to see\n");
	return 0;

}

PRIVATE int resume(int argc, char * argv[]) {
	struct proc * p = proc_table;
	//char* proc_name = [128];
	//memset(proc_name, 0, 128);
	if (argc == 1) {
		printf("Please input proc name, you can use 'ps' command to see\n");
		return 0;
	}
	int i;
	//for (i = 0; i < strlen(argv[1]); i++) {
	//	proc_name[i] = argv[i];
	//}
	for (i = 0; i < NR_TASKS + NR_NATIVE_PROCS; i++, p++) {
		if (strcmp(argv[1], p->name) == 0) {
			if (p->p_flags == 0) {
				printf("%s proc is running!\n", p->name);
				return 0;
			}
			else if (p->priority == -1) {
				printf("%s proc has been killed, can`t resume!\n", p->name);
				return 0;
			}
			else {
				p->p_flags = 0;
				printf("Success!\n");
				return 0;
			}
		}
	}

	printf("Please input correct proc name, you can use 'ps' command to see\n");
	return 0;
}

PRIVATE int kill(int argc, char * argv[]) {
	struct proc * p = proc_table;
	//char* proc_name = [128];
	//memset(proc_name, 0, 128);
	if (argc == 1) {
		printf("Please input proc name, you can use 'ps' command to see\n");
		return 0;
	}
	int i;
	//for (i = 0; i < strlen(argv[1]); i++) {
	//	proc_name[i] = argv[i];
	//}
	for (i = 0; i < NR_TASKS + NR_NATIVE_PROCS; i++, p++) {
		if (strcmp(argv[1], p->name) == 0) {
			if (i < NR_TASKS) {
				printf("Can`t kill system task!\n");
				return 0;
			}
			else {
				p->p_flags = -1;
				p->priority = -1;
				printf("Success!\n");
				return 0;
			}
		}
	}

	printf("Please input correct proc name, you can use 'ps' command to see\n");
	return 0;
}

 PRIVATE int pro_details() {
    struct proc * p = proc_table;
    int i;
	printf("===================== Process Viewer ====================\n");
	printf("    pid    |    name    |    running?    |    kill?\n");

    for (i = 0; i < NR_TASKS; i++,p++) {             
        printf("     %d    |     %s        |    %d     |    %d\n", i, p->name, p->p_flags, p->priority);
    }
    printf("===================== User Process ====================\n");      
    for (; i < NR_TASKS + NR_NATIVE_PROCS; i++,p++) {	 
        printf("     %d    |     %s        |    %d     |    %d\n", i, p->name, p->p_flags, p->priority);     
    }
    return 0;
}