
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Forrest Yu, 2005
Yakang Li, 2018
Jinrong Huang, 2018
Dinghow Yang, 2018
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

/*****************************************************************************
*                               kernel_main
*****************************************************************************/
/**
* jmp from kernel.asm::_start.
*
*****************************************************************************/

char currentUser[128] = "/";
char currentFolder[128] = "|";
char filepath[128] = "";
char users[2][128] = {"empty", "empty"};
char passwords[2][128];
char files[20][128];
char userfiles[20][128];
int filequeue[50];
int filecount = 0;
int usercount = 0;
int isEntered = 0;
int UserState = 0;
//int UserSwitch = 0;
int leiflag = 0;

PUBLIC int kernel_main()
{
    disp_str("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    int i, j, eflags, prio;
    u8 rpl;
    u8 priv; /* privilege */

    struct task *t;
    struct proc *p = proc_table;
    char *stk = task_stack + STACK_SIZE_TOTAL;

    for (i = 0; i < NR_TASKS + NR_PROCS; i++, p++, t++)
    {
        if (i >= NR_TASKS + NR_NATIVE_PROCS)
        {
            p->p_flags = FREE_SLOT;
            continue;
        }

        if (i < NR_TASKS)
        { /* TASK */
            t = task_table + i;
            priv = PRIVILEGE_TASK;
            rpl = RPL_TASK;
            eflags = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
            prio = 15;
        }
        else
        { /* USER PROC */
            t = user_proc_table + (i - NR_TASKS);
            priv = PRIVILEGE_USER;
            rpl = RPL_USER;
            eflags = 0x202; /* IF=1, bit 2 is always 1 */
            prio = 5;
        }

        strcpy(p->name, t->name); /* name of the process */
        p->p_parent = NO_TASK;

        if (strcmp(t->name, "INIT") != 0)
        {
            p->ldts[INDEX_LDT_C] = gdt[SELECTOR_KERNEL_CS >> 3];
            p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

            /* change the DPLs */
            p->ldts[INDEX_LDT_C].attr1 = DA_C | priv << 5;
            p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
        }
        else
        { /* INIT process */
            unsigned int k_base;
            unsigned int k_limit;
            int ret = get_kernel_map(&k_base, &k_limit);
            assert(ret == 0);
            init_desc(&p->ldts[INDEX_LDT_C],
                      0, /* bytes before the entry point
				   * are useless (wasted) for the
				   * INIT process, doesn't matter
				   */
                      (k_base + k_limit) >> LIMIT_4K_SHIFT,
                      DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

            init_desc(&p->ldts[INDEX_LDT_RW],
                      0, /* bytes before the entry point
				   * are useless (wasted) for the
				   * INIT process, doesn't matter
				   */
                      (k_base + k_limit) >> LIMIT_4K_SHIFT,
                      DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
        }

        p->regs.cs = INDEX_LDT_C << 3 | SA_TIL | rpl;
        p->regs.ds =
            p->regs.es =
                p->regs.fs =
                    p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
        p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
        p->regs.eip = (u32)t->initial_eip;
        p->regs.esp = (u32)stk;
        p->regs.eflags = eflags;

        p->ticks = p->priority = prio;
        strcpy(p->name, t->name); /* name of the process */
        p->pid = i;               /* pid */
        p->run_count = 0;
        p->run_state = 1;

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

    p_proc_ready = proc_table;

    init_clock();
    init_keyboard();

    restart();

    while (1)
    {
    }
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

/**
* @struct posix_tar_header
* Borrowed from GNU `tar'
*/
struct posix_tar_header
{                       /* byte offset */
    char name[100];     /*   0 */
    char mode[8];       /* 100 */
    char uid[8];        /* 108 */
    char gid[8];        /* 116 */
    char size[12];      /* 124 */
    char mtime[12];     /* 136 */
    char chksum[8];     /* 148 */
    char typeflag;      /* 156 */
    char linkname[100]; /* 157 */
    char magic[6];      /* 257 */
    char version[2];    /* 263 */
    char uname[32];     /* 265 */
    char gname[32];     /* 297 */
    char devmajor[8];   /* 329 */
    char devminor[8];   /* 337 */
    char prefix[155];   /* 345 */
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
void untar(const char *filename)
{
    printf("[extract `%s'\n", filename);
    int fd = open(filename, O_RDWR);
    assert(fd != -1);

    char buf[SECTOR_SIZE * 16];
    int chunk = sizeof(buf);
    int i = 0;
    int bytes = 0;

    while (1)
    {
        bytes = read(fd, buf, SECTOR_SIZE);
        assert(bytes == SECTOR_SIZE); /* size of a TAR file
									  * must be multiple of 512
									  */
        if (buf[0] == 0)
        {
            if (i == 0)
                printf("    need not unpack the file.\n");
            break;
        }
        i++;

        struct posix_tar_header *phdr = (struct posix_tar_header *)buf;

        /* calculate the file size */
        char *p = phdr->size;
        int f_len = 0;
        while (*p)
            f_len = (f_len * 8) + (*p++ - '0'); /* octal */

        int bytes_left = f_len;
        int fdout = open(phdr->name, O_CREAT | O_RDWR | O_TRUNC);
        if (fdout == -1)
        {
            printf("    failed to extract file: %s\n", phdr->name);
            printf(" aborted]\n");
            close(fd);
            return;
        }
        printf("    %s\n", phdr->name);
        while (bytes_left)
        {
            int iobytes = min(chunk, bytes_left);
            read(fd, buf,
                 ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
            bytes = write(fdout, buf, iobytes);
            assert(bytes == iobytes);
            bytes_left -= iobytes;
        }
        close(fdout);
    }

    if (i)
    {
        lseek(fd, 0, SEEK_SET);
        buf[0] = 0;
        bytes = write(fd, buf, 1);
        assert(bytes == 1);
    }

    close(fd);

    printf(" done, %d files extracted]\n", i);
}

/*****************************************************************************
*                                shabby_shell
*****************************************************************************/
/**
* A very very simple shell.
*
* @param tty_name  TTY file name.
*****************************************************************************/
PUBLIC void clear()
{
    int i = 0;
    for (i = 0; i < 30; i++)
        printf("\n");
}

void shabby_shell(const char *tty_name)
{
    int fd_stdin = open(tty_name, O_RDWR);
    assert(fd_stdin == 0);
    int fd_stdout = open(tty_name, O_RDWR);
    assert(fd_stdout == 1);

    char rdbuf[128]; //读取的命令
    char cmd[128];   //指令
    char arg1[128];  //参数1
    char arg2[128];  //参数2
    char buf[1024];

    initFs();
    while (1)
    {
        if (usercount == 0)
        {
            printf("Enter Admin Password:");
            char buf[128];
            int r = read(0, buf, 128);
            buf[r] = 0;
            if (strcmp(buf, "admin") == 0)
            {
                strcpy(currentUser, "/");
                UserState = 3;
                break;
            }
            else
                printf("Password Error!\n");
        }
        else
        {
            //printf("%d",usercount);
            int isGet = 0;
            printf("Enter User Name:");
            char buf[128];
            int r = read(0, buf, 128);
            buf[r] = 0;
            int i;
            for (i = 0; i < usercount; i++)
            {
                if (strcmp(buf, users[i]) == 0 && strcmp(buf, "empty") != 0)
                {
                    printf("Enter %s Password:");
                    char buf[128];
                    int r = read(0, buf, 128);
                    buf[r] = 0;
                    if (strcmp(buf, passwords[i]) == 0)
                    {
                        strcpy(currentUser, users[i]);
                        UserState = i + 1;
                        isGet = 1;
                        break;
                    }
                }
            }
            if (isGet)
                break;
            else
                printf("Password Error Or User Not Exist!\n");
        }
    }

    while (1)
    {
        //init char array
        clearArr(rdbuf, 128);
        clearArr(cmd, 128);
        clearArr(arg1, 128);
        clearArr(arg2, 128);
        clearArr(buf, 1024);
	if(UserState == 3)
		printf("[Admin@YuiOS]%s%s# ",currentUser,currentFolder);
	else	
		printf("[%s@YuiOS]/%s%s$ ",users[UserState-1],currentUser,currentFolder);
        //write(1, "$ ", 2);
        int r = read(0, rdbuf, 70);
        rdbuf[r] = 0;

        int argc = 0;
        char *argv[PROC_ORIGIN_STACK];
        char *p = rdbuf;
        char *s;
        int word = 0;
        char ch;
        do
        {
            ch = *p;
            if (*p != ' ' && *p != 0 && !word)
            {
                s = p;
                word = 1;
            }
            if ((*p == ' ' || *p == 0) && word)
            {
                word = 0;
                argv[argc++] = s;
                *p = 0;
            }
            p++;
        } while (ch);
        argv[argc] = 0;

        int fd = open(argv[0], O_RDWR);

        if (fd == -1)
        {
            if (rdbuf[0])
            {
                int i = 0, j = 0;
                /* get command */
                while (rdbuf[i] != ' ' && rdbuf[i] != 0)
                {
                    cmd[i] = rdbuf[i];
                    i++;
                }
                i++;
                /* get arg1 */
                while (rdbuf[i] != ' ' && rdbuf[i] != 0)
                {
                    arg1[j] = rdbuf[i];
                    i++;
                    j++;
                }
                i++;
                j = 0;
                /* get arg2 */
                while (rdbuf[i] != ' ' && rdbuf[i] != 0)
                {
                    arg2[j] = rdbuf[i];
                    i++;
                    j++;
                }

                //Process the command to format"cmd arg1 arg2"
                if (strcmp(cmd, "help") == 0)
                {
                    showhelp();
                }
                else if (strcmp(cmd, "clear") == 0)
                {
                    clear();
                    welcome();
                } 
                else if (strcmp(cmd, "sudo") == 0)
                {
                    printf("Enter Admin Password:");
                    char buf[128];
                    int r = read(0, buf, 128);
                    buf[r] = 0;
                    if (strcmp(buf, "admin") == 0)
                    {
                        strcpy(currentUser, "/");
                        UserState = 3;
                    }
                    else
                        printf("Password Error!\n");
                }
                else if (strcmp(cmd, "adduser") == 0)
                {
                    addUser(arg1, arg2);
                }
                else if (strcmp(cmd, "deluser") == 0)
                {
                    moveUser(arg1, arg2);
                }
                else if (strcmp(cmd, "su") == 0)
                {
                    shift(arg1, arg2);
                }
                else if (strcmp(cmd, "mkfile") == 0)
                {
                    createFilepath(arg1);
                    createFile(filepath, arg2, 1);
                    clearArr(filepath, 128);
                }
		else if(strcmp(cmd, "mkdir") == 0)
		{
			createFilepath(strcat(arg1,"*"));
			createFolder(filepath, 1);
			clearArr(filepath, 128);
		}
		else if (strcmp(cmd, "cd") == 0) 
		{
			createFilepath(arg1);
			openFolder(filepath,arg1);
		}
                else if (strcmp(cmd, "rd") == 0)
                {
                    createFilepath(arg1);
                    readFile(filepath);
                    clearArr(filepath, 128);
                }
                /* edit a file appand */
                else if (strcmp(cmd, "wt+") == 0)
                {
                    createFilepath(arg1);
                    editAppand(filepath, arg2);
                    clearArr(filepath, 128);
                }
                /* edit a file cover */
                else if (strcmp(cmd, "wt") == 0)
                {
                    createFilepath(arg1);
                    editCover(filepath, arg2);
                    clearArr(filepath, 128);
                }
                /* delete a file */
                else if (strcmp(cmd, "del") == 0)
                {
                    createFilepath(arg1);
                    deleteFile(filepath);
                    clearArr(filepath, 128);
                }
                /* ls */
                else if (strcmp(cmd, "ls") == 0)
                {
                    ls();
                }
                else if (strcmp(cmd, "proc") == 0)
                {
                    showProcess();
                }
                else if (strcmp(cmd, "kill") == 0)
                {
                    killpro(arg1);
                }
                else if (strcmp(cmd, "pause") == 0)
                {
                    pausepro(arg1);
                }
                else if (strcmp(cmd, "resume") == 0)
                {
                    resume(arg1);
                }
                else if (strcmp(cmd, "tictactoe") == 0)
                {
                    main_tic();
                }
		else if(strcmp(cmd, "boom") == 0)
		{
		    mainboom();
		}
                else
                {
                    continue;
                }
            }
        }
        else
        {
            close(fd);
            int pid = fork();
            if (pid != 0)
            { /* parent */
                int s;
                wait(&s);
            }
            else
            { /* child */
                execv(argv[0], argv);
            }
        }
    }

    close(1);
    close(0);
}

void killpro(char *a)
{
    int b = *a - 48;
    if(b >= 0 && b <= NR_TASKS)   printf("operation to proc %d denied \n",b);
    else if(b < NR_TASKS + NR_NATIVE_PROCS)
    {
        proc_table[b].p_flags = 1;
        showProcess();
    }
    else printf("can not find proc with pid:%d \n",b);
}

void pausepro(char *a)
{
    int b = *a - 48;
    if(b >= 0 && b <= NR_TASKS)   printf("operation to proc %d denied \n",b);
    else if(b < NR_TASKS + NR_NATIVE_PROCS)
    {
        proc_table[b].run_state = 0;
        showProcess();
    }
    else printf("can not find proc with pid:%d \n",b);
}

void resume(char *a)
{
    int b = *a - 48;
    if(b >= 0 && b <= NR_TASKS)   printf("operation to proc %d denied \n",b);
    else if(b < NR_TASKS + NR_NATIVE_PROCS)
    {
        proc_table[b].run_state = 1;
        showProcess();
    }
    else printf("can not find proc with pid:%d \n",b);
}

void clearArr(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
        arr[i] = 0;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
File system
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Dinghow Yang, 2018
Attention!Out muti-class file system is basd on string matching actually
, so you can rewrite a real one by yourself =。=
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* Get File Pos */
int getPos()
{
    int i = 0;
    for (i = 0; i < 500; i++)
    {
        if (filequeue[i] == 1)
            return i;
    }
}

int len(char *a)
{
    int ans = 0;
    int i;
    for (i = 0; i < 16; i++)
    {
        if (a[i] == 0)
            break;
        ans++;
    }
    return ans;
}

int vertify()
{
    if (UserState == 0)
    {
        printf("Permission deny!!\n");
        return 0;
    }
    else
        return 1;
}

/* Create Filepath */
void createFilepath(char* filename)
{
	int i = 0, j = 0, k = 0;
		
	for (i; i < len(currentUser); i++)
	{
		filepath[i] = currentUser[i];
	}

	for (j; j < strlen(currentFolder); i++, j++) {
		filepath[i] = currentFolder[j];
	}

	for(k = 0; k < strlen(filename); i++, k++)
	{	
		filepath[i] = filename[k];
	}
	filepath[i] = '\0';
}

/* Update FileLogs */
void updateFileLogs()
{
    int i = 0, count = 0;
    editCover("fileLogs", "");
    while (count <= filecount - 1)
    {
        if (filequeue[i] == 0)
        {
            i++;
            continue;
        }
        char filename[128];
        int len = strlen(files[count]);
        strcpy(filename, files[count]);
        filename[len] = ' ';
        filename[len + 1] = '\0';
        //printf("%s\n", filename);
        editAppand("fileLogs", filename);
        count++;
        i++;
    }
}

/* Update myUsers */
void updateMyUsers()
{
    int i = 0, count = 0;
    editCover("myUsers", "");
    if (strcmp(users[0], "empty") != 0)
    {
        editAppand("myUsers", users[0]);
        editAppand("myUsers", " ");
    }
    else
    {
        editAppand("myUsers", "empty ");
    }
    if (strcmp(users[1], "empty") != 0)
    {
        editAppand("myUsers", users[1]);
        editAppand("myUsers", " ");
    }
    else
    {
        editAppand("myUsers", "empty ");
    }
}

/* Update myUsersPassword */
void updateMyUsersPassword()
{
    int i = 0, count = 0;
    editCover("myUsersPassword", "");
    if (strcmp(passwords[0], "") != 0)
    {
        editAppand("myUsersPassword", passwords[0]);
        editAppand("myUsersPassword", " ");
    }
    else
    {
        editAppand("myUsersPassword", "empty ");
    }
    if (strcmp(passwords[1], "") != 0)
    {
        editAppand("myUsersPassword", passwords[1]);
        editAppand("myUsersPassword", " ");
    }
    else
    {
        editAppand("myUsersPassword", "empty ");
    }
}

/* Add FIle Log */
void addLog(char * filepath)
{
	int pos = -1, i = 0;
	pos = getPos();
	filecount++;
	strcpy(files[pos], filepath);
	updateFileLogs();
	filequeue[pos] = 0;
	if (strcmp("/", currentUser) != 0)
	{
		int fd = -1, k = 0, j = 0;
		char filename[128];
		while (k < strlen(filepath))
		{
			if (filepath[k] != '|')
				k++;
			else
				break;
		}
		k++;
		while (k < strlen(filepath))
		{
			filename[j] = filepath[k];
			k++;
			j++;
		}
		filename[j] = '\0';
		if (strcmp(currentUser, users[0]) == 0)
		{
			editAppand("user1", filename);
			editAppand("user1", " ");
		}
		else if(strcmp(currentUser, users[1]) == 0)
		{
			editAppand("user2", filename);
			editAppand("user2", " ");
		}
	}
}

/* Delete File Log */
void deleteLog(char *filepath)
{
    int i = 0, fd = -1;
    for (i = 0; i < filecount; i++)
    {
        if (strcmp(filepath, files[i]) == 0)
        {
            strcpy(files[i], "empty");
            int len = strlen(files[i]);
            files[i][len] = '0' + i;
            files[i][len + 1] = '\0';
            fd = open(files[i], O_CREAT | O_RDWR);
            close(fd);
            filequeue[i] = 1;
            break;
        }
    }
    filecount--;
    updateFileLogs();
}

void showhelp()
{
    printf(" _________________________________________________________________ \n");
    printf("|          instruction           |             function           |\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("| help                           | show help table                |\n");
    printf("| sudo                           | obtain administrator privileges|\n");
    printf("| adduser  [username] [password] | add user                       |\n");
    printf("| deluser  [username] [password] | remove user                    |\n");
    printf("| su       [username] [password] | shift to user                  |\n");
    printf("| ls                             | show file list                 |\n");
    printf("| mkdir    [foldername]          | create folder                  |\n");
    printf("| cd       [foldername]          | open folder                    |\n");
    printf("| rd       [filename]            | read file                      |\n");
    printf("| mkfile   [filename] [content]  | create file                    |\n");
    printf("| wt  +    [filename] [content]  | edit file, append content      |\n");
    printf("| wt       [filename] [content]  | edit file, cover content       |\n");
    printf("| del      [filename]            | delete file                    |\n");
    printf("| proc                           | show running process table     |\n");
    printf("| kill     [proc.pid]            | kill process                   |\n");
    printf("| pause    [proc.pid]            | pause process                  |\n");
    printf("| resume   [proc.pid]            | resume process                 |\n");
    printf("\n");
    printf(" Applications: tictactoe,boom\n");
}

/*Init the currentFold array*/
void initFolder() {
	for (int i = 1; i < 128; i++) {
		currentFolder[i] = '\0';
	}
}

/* Init FS */
void initFs()
{
    int fd = -1, n = 0, i = 0, count = 0, k = 0;
    char bufr[1024] = "";
    char bufp[1024] = "";
    char buff[1024] = "";

    for (i = 0; i < 500; i++)
        filequeue[i] = 1;

    initFolder();
    fd = open("myUsers", O_RDWR);
    close(fd);
    fd = open("myUsersPassword", O_RDWR);
    close(fd);
    fd = open("fileLogs", O_RDWR);
    close(fd);
    fd = open("user1", O_RDWR);
    close(fd);
    fd = open("user2", O_RDWR);
    close(fd);
    /* init users */
    fd = open("myUsers", O_RDWR);
    n = read(fd, bufr, 1024);
    bufr[strlen(bufr)] = '\0';
    for (i = 0; i < strlen(bufr); i++)
    {
        if (bufr[i] != ' ')
        {
            users[count][k] = bufr[i];
            k++;
        }
        else
        {
            while (bufr[i] == ' ')
            {
                i++;
                if (bufr[i] == '\0')
                {
                    users[count][k] = '\0';
                    if (strcmp(users[count], "empty") != 0)
                        usercount++;
                    count++;
                    break;
                }
            }
            if (bufr[i] == '\0')
            {
                break;
            }
            i--;
            users[count][k] = '\0';
            if (strcmp(users[count], "empty") != 0)
                usercount++;
            k = 0;
            count++;
        }
    }
    close(fd);
    count = 0;
    k = 0;

    /* init password */
    fd = open("myUsersPassword", O_RDWR);
    n = read(fd, bufp, 1024);
    for (i = 0; i < strlen(bufp); i++)
    {
        if (bufp[i] != ' ')
        {
            passwords[count][k] = bufp[i];
            k++;
        }
        else
        {
            while (bufp[i] == ' ')
            {
                i++;
                if (bufp[i] == '\0')
                {
                    count++;
                    break;
                }
            }
            if (bufp[i] == '\0')
                break;
            i--;
            passwords[count][k] = '\0';
            k = 0;
            count++;
        }
    }
    close(fd);
    count = 0;
    k = 0;

    /* init files */
    fd = open("fileLogs", O_RDWR);
    n = read(fd, buff, 1024);
    for (i = 0; i <= strlen(buff); i++)
    {
        if (buff[i] != ' ')
        {
            files[count][k] = buff[i];
            k++;
        }
        else
        {
            while (buff[i] == ' ')
            {
                i++;
                if (buff[i] == '\0')
                {
                    break;
                }
            }
            if (buff[i] == '\0')
            {
                files[count][k] = '\0';
                count++;
                break;
            }
            i--;
            files[count][k] = '\0';
            k = 0;
            count++;
        }
    }
    close(fd);

    int empty = 0;
    for (i = 0; i < count; i++)
    {
        char flag[7];
        strcpy(flag, "empty");
        flag[5] = '0' + i;
        flag[6] = '\0';
        fd = open(files[i], O_RDWR);
        close(fd);

        if (strcmp(files[i], flag) != 0)
            filequeue[i] = 0;
        else
            empty++;
    }
    filecount = count - empty;
}

/*Create folder*/
void createFolder(char* filepath,int flag) 
{
	int fd = -1, i = 0, pos;
	pos = getPos();
	char f[7];
	strcpy(f, "empty");
	f[5] = '0' + pos;
	f[6] = '\0';
	if (strcmp(files[pos], f) == 0 && flag == 1)
	{
		unlink(files[pos]);
	}

	fd = open(filepath, O_CREAT | O_RDWR);
	printf("folder name: %s \n", filepath);
	if (fd == -1)
	{
		printf("Operation failed, please check the path and try again!\n");
		return;
	}
	if (fd == -2)
	{
		printf("Operation failed, file already exists!\n");
		return;
	}
	close(fd);

	/* add log */
	if (flag == 1)
		addLog(filepath);
}

/* Create File */
void createFile(char *filepath, char *buf, int flag)
{
    int fd = -1, i = 0, pos;
    pos = getPos();
    char f[7];
    strcpy(f, "empty");
    f[5] = '0' + pos;
    f[6] = '\0';
    if (strcmp(files[pos], f) == 0 && flag == 1)
    {
        unlink(files[pos]);
    }

    fd = open(filepath, O_CREAT | O_RDWR);
    printf("file name: %s content: %s\n", filepath, buf);
    if (fd == -1)
    {
        printf("Fail, please check and try again!!\n");
        return;
    }
    if (fd == -2)
    {
        printf("Fail, file exsists!!\n");
        return;
    }
    //printf("%s\n", buf);

    write(fd, buf, strlen(buf));
    close(fd);

    /* add log */
    if (flag == 1)
        addLog(filepath);
}

/*Get into the fold*/
void openFolder(char* filepath,char* filename)
{
	//Check if it is a back father path command
	if (strcmp(filename, "..") == 0) 
	{	
		int i = strlen(currentFolder) - 1,j;

		currentFolder[i] = '\0';
		for (j = i-1; j >= 0; j--) 
		{
			if (currentFolder[j] == '|')
				break;
		}
		j++;
		for (j; j < i; j++) 
		{
			currentFolder[j] = '\0';
		}
		printf("%s\n",currentFolder);
		return;
	}

	int name_length = strlen(filename);
	//Check if it is a folder
	if (filename[name_length-1] != '*')
		return;
	
	int i = 0,j = 0;

	for (i; i < 128; i++)
	{
		if (currentFolder[i] == '\0')
			break;
	}

	for (j = 0; j < strlen(filename); j++,i++)
	{
		currentFolder[i] = filename[j];
	}
	currentFolder[i++] = '|';
	currentFolder[i] = '\0';
}

/* Read File */
void readFile(char *filepath)
{
    if (vertify() == 0)
        return;

    int fd = -1;
    int n;
    char bufr[1024] = "";
    fd = open(filepath, O_RDWR);
    if (fd == -1)
    {
        printf("Fail, please check and try again!!\n");
        return;
    }
    n = read(fd, bufr, 1024);
    bufr[n] = '\0';
    printf("%s(fd=%d) : %s\n", filepath, fd, bufr);
    close(fd);
}

/* Edit File Appand */
void editAppand(char *filepath, char *buf)
{
    if (vertify() == 0)
        return;

    int fd = -1;
    int n, i = 0;
    char bufr[1024] = "";
    char empty[1024];

    for (i = 0; i < 1024; i++)
        empty[i] = '\0';
    fd = open(filepath, O_RDWR);
    if (fd == -1)
    {
        printf("Fail, please check and try again!!\n");
        return;
    }

    n = read(fd, bufr, 1024);
    n = strlen(bufr);

    for (i = 0; i < strlen(buf); i++, n++)
    {
        bufr[n] = buf[i];
        bufr[n + 1] = '\0';
    }
    write(fd, empty, 1024);
    fd = open(filepath, O_RDWR);
    write(fd, bufr, strlen(bufr));
    close(fd);
}

/* Edit File Cover */
void editCover(char *filepath, char *buf)
{

    if (vertify() == 0)
        return;

    int fd = -1;
    int n, i = 0;
    char bufr[1024] = "";
    char empty[1024];

    for (i = 0; i < 1024; i++)
        empty[i] = '\0';

    fd = open(filepath, O_RDWR);
    //printf("%d",fd);
    if (fd == -1)
        return;
    write(fd, empty, 1024);
    close(fd);
    fd = open(filepath, O_RDWR);
    write(fd, buf, strlen(buf));
    close(fd);
}

/* Delete File */
void deleteFile(char *filepath)
{
    if (vertify() == 0)
        return;
    if (usercount == 0)
    {
        printf("Fail!\n");
        return;
    }
    editCover(filepath, "");
    //printf("%s",filepath);
    int a = unlink(filepath);
    if (a != 0)
    {
        printf("Edit fail, please try again!\n");
        return;
    }
    deleteLog(filepath);

    char username[128];
    if (strcmp(currentUser, users[0]) == 0)
    {
        strcpy(username, "user1");
    }
    if (strcmp(currentUser, users[1]) == 0)
    {
        strcpy(username, "user2");
    }

    char userfiles[20][128];
    char bufr[1024];
    char filename[128];
    char realname[128];
    int fd = -1, n = 0, i = 0, count = 0, k = 0;
    fd = open(username, O_RDWR);
    n = read(fd, bufr, 1024);
    close(fd);

    for (i = strlen(currentUser) + 1; i < strlen(filepath); i++, k++)
    {
        realname[k] = filepath[i];
    }
    realname[k] = '\0';
    k = 0;
    for (i = 0; i < strlen(bufr); i++)
    {
        if (bufr[i] != ' ')
        {
            filename[k] = bufr[i];
            k++;
        }
        else
        {
            filename[k] = '\0';
            if (strcmp(filename, realname) == 0)
            {
                k = 0;
                continue;
            }
            strcpy(userfiles[count], filename);
            count++;
            k = 0;
        }
    }

    i = 0, k = 0;
    for (k = 0; k < 2; k++)
    {
        printf("%s\n", userfiles[k]);
    }
    editCover(username, "");
    while (i < count)
    {
        if (strlen(userfiles[i]) < 1)
        {
            i++;
            continue;
        }
        char user[128];
        int len = strlen(userfiles[i]);
        strcpy(user, userfiles[i]);
        user[len] = ' ';
        user[len + 1] = '\0';
        editAppand(username, user);
        i++;
    }
}

void shift(char *username, char *password)
{
    int i = 0;
    for (i = 0; i < usercount; i++)
    {
        if (strcmp(username, users[i]) == 0 && strcmp(password, passwords[i]) == 0 && strcmp(username, "empty") != 0)
        {
            strcpy(currentUser, users[i]);
            UserState = i + 1;
            printf("Welcome! %s!\n", users[i]);
            return;
        }
        //printf("%s %s %s %s",username,password,users[i],passwords[i]);
    }
    printf("Sorry! No such user!\n");
}

/* Add User */
void addUser(char *username, char *password)
{
    if (UserState == 3)
    {
        int i;
        for (i = 0; i < 2; i++)
        {
            if (strcmp(users[i], username) == 0)
            {
                printf("User exists!\n");
                return;
            }
        }
        if (usercount == 2)
        {
            printf("No more users\n");
            return;
        }
        if (strcmp(users[0], "empty") == 0)
        {
            strcpy(users[0], username);
            strcpy(passwords[0], password);
            usercount++;
            updateMyUsers();
            updateMyUsersPassword();
            return;
        }
        if (strcmp(users[1], "empty") == 0)
        {
            strcpy(users[1], username);
            strcpy(passwords[1], password);
            usercount++;
            updateMyUsers();
            updateMyUsersPassword();
            return;
        }
    }
    else
        printf("Permission Deny!");
}

/* Move User */
void moveUser(char *username, char *password)
{
    if (UserState == 3)
    {
        int i = 0;
        for (i = 0; i < 2; i++)
        {
            if (strcmp(username, users[i]) == 0 && strcmp(password, passwords[i]) == 0)
            {
                //strcpy(currentUser, username);
                int fd = -1, n = 0, k = 0, count = 0;
                char bufr[1024], deletefile[128];
                if (i == 0)
                {
                    fd = open("user1", O_RDWR);
                }
                if (i == 1)
                {
                    fd = open("user2", O_RDWR);
                }
                n = read(fd, bufr, 1024);
                close(fd);
                for (k = 0; k < strlen(bufr); k++)
                {
                    if (bufr[k] != ' ')
                    {
                        deletefile[count] = bufr[k];
                        count++;
                    }
                    else
                    {
                        deletefile[count] = '\0';
                        createFilepath(deletefile);
                        deleteFile(filepath);
                        count = 0;
                    }
                }
                printf("Delete %s!\n", users[i]);
                strcpy(users[i], "empty");
                strcpy(passwords[i], "");
                updateMyUsers();
                updateMyUsersPassword();
                usercount--;
                strcpy(currentUser, "/");
                return;
            }
        }
        printf("Sorry! No such user!\n");
    }
    else
        printf("Permission Deny!");
}
/* Compare the file path to current directory */
void pathCompare(char* temp)
{
	char current_temp[256];
	char filename_only[64];
	int i = 0,j = 0,k = 0;
	int flag =1;

	for(i;i < strlen(currentFolder);i++)
	{
		current_temp[i] = currentFolder[i + 1];	
	}
	
	for(j;j < strlen(current_temp);j++)
	{
		if(current_temp[j] != temp[j])
		{
			flag = 0;
			break;
		}	
	}
	if(flag == 1)
	{
		for(j; j < strlen(temp);j++,k++)
		{
			filename_only[k] = temp[j];
		}
		filename_only[k] = '\0';

		printf("%s\n",filename_only);	
	}
}

/* split path */
void pathFilter(char* bufr)
{
	char *p,*q;
	char temp[128];
	int length = 0;
	p = bufr;
	q = p;
	while(1)
	{
		while(*q != ' ' && *q != '\0')
		{
			q++;												
		}
		if(*q == '\0')
			return;
		else
		{
			*q = '\0';
			length = q - p;
			int i = 0;
			for(i;i < length; i++,p++)
			{
				temp[i] = *p;
			}
			temp[i] = '\0';
			//printf("temp=%s\n",temp);
			pathCompare(temp);
			q++;
			p = q;
		}
	}
}

/* Ls */
void ls()
{
	int fd = -1, n;
	char bufr[1024];
	if (strcmp(currentUser, users[0]) == 0)
	{
		fd = open("user1", O_RDWR);
		if (fd == -1)
		{
			printf("empty\n");
		}
		n = read(fd, bufr, 1024);
		pathFilter(bufr);												
		//printf("%s\n", bufr);
		close(fd);
	}
	else if(strcmp(currentUser, users[1]) == 0)
	{
		fd = open("user2", O_RDWR);
		if (fd == -1)
		{
			printf("empty\n");
		}
		n = read(fd, bufr, 1024);
		pathFilter(bufr);
		//printf("%s\n", bufr);
		close(fd);
	}
	else
		printf("Permission deny!\n");
}

/* Show Process */
void showProcess()
{	int i = 0;
	printf("---------------------------------\n");
    printf("| pid |    name     |   state   |\n");
    printf("---------------------------------\n");
	for (i = 0; i < NR_TASKS + NR_NATIVE_PROCS; i++)
	{
		if(proc_table[i].p_flags != 1){
			printf("|  %d",proc_table[i].pid);
			if(proc_table[i].pid<10) printf("  ");
			else if(proc_table[i].pid<100) printf(" ");

        		printf("| %s",proc_table[i].name);
			int j;
        		for(j=0;j<12-len(proc_table[i].name);j++)
            			printf(" ");

			if(proc_table[i].run_state) printf("|  running ");
			else printf("|  paused  ");

			if(i <= NR_TASKS) printf("-");
			else printf(" ");
        		printf("|\n");	
		}
	}
	printf("---------------------------------\n");
}

/*****************************************************************************
 *                                Init
 *****************************************************************************/
/**
 * The hen.
 * 
 *****************************************************************************/
void Init()
{
    int fd_stdin = open("/dev_tty0", O_RDWR);
    assert(fd_stdin == 0);
    int fd_stdout = open("/dev_tty0", O_RDWR);
    assert(fd_stdout == 1);

    printf("Init() is running ...\n");

    /* extract `cmd.tar' */
    untar("/cmd.tar");
    welcomeAnimation();
    welcome();
    //shabby_shell("/dev_tty0");

    //char * tty_list[] = {"/dev_tty1", "/dev_tty2"};
    char *tty_list[] = {"/dev_tty0", "/dev_tty1", "/dev_tty2"};

    int i;
    for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++)
    {
        int pid = fork();
        if (pid != 0)
        {   /* parent process */
            //printf("[parent is running, child pid:%d]\n", pid);
        }
        else
        { /* child process */
            //printf("[child is running, pid:%d]\n", getpid());
            close(fd_stdin);
            close(fd_stdout);
            shabby_shell(tty_list[i]);
            assert(0);
        }
    }

    while (1)
    {
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
    while (1)
    {
        if (proc_table[6].run_state == 1)
        {
        }
    }
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
    while (1)
    {
        if (proc_table[7].run_state == 1)
        {
        }
    }
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
    while (1)
    {
        if (proc_table[8].run_state == 1)
        {
        }
    }
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
    int i;
    char buf[256];

    /* 4 is the size of fmt in the stack */
    va_list arg = (va_list)((char *)&fmt + 4);

    i = vsprintf(buf, fmt, arg);

    printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

    /* should never arrive here */
    __asm__ __volatile__("ud2");
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
File system
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Yakang Li, 2018
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include<stdio.h> 

#define SIZE 3

char scane[SIZE][SIZE]={'*','*','*','*','*','*','*','*','*'};
int grade[SIZE][SIZE]={50,10,50,10,100,10,50,10,50};

void AI_input(int x,int y);
void display();
int check();
int main_tic()
{
	printf("Welcome Tictactoe, input q to quit game\n");
	int result=0;
	display();
	for(int i=0;i<5;i++)
	{
		printf("Please input(x y):");
		char input[4];
		for(int i=0;i<4;i++)
		{
		    input[i]=0;	

		}
		char x,y;
		//scanf("%d,%d",&x,&y);
		int r=read(0,input,4);
		input[r]=0;
		x=input[0]-48;
		y=input[2]-48;
/*
		printf("%c\n",input[0]);
		printf("%c\n",input[2]);
		printf("%d\n",r);
		printf("%c\n",x);
		printf("%c\n",y);
		printf("%s\n",input);
		read(0,input,128);
*/		
		if(x+48=='q'){
		   printf("Game Over\n"); 
		   return 0;
		    
		}
		while(1){
			if(x>0&&x<4&&y>0&&y<4&&(grade[x][y]%10==0)) break;
			else {
				printf("Error,input again:");
				int r=read(0,input,4);
				input[r]=0;
				x=input[0]-48;
				y=input[2]-48;
				if(x+48=='q'){
		   		    printf("Game Over\n"); 
		   		    return 0;
		    
				}
			}
		}
		scane[x-1][y-1]='o';
		grade[x-1][y-1]=4;
		
		if(check()==1){
			printf("You win\n");
			result=1;
			break;
		}
	
		if(i!=4) AI_input(x-1,y-1);
		
		if(check()==2){
			printf("You lose\n");
			result=1;
			break;
		}
		display();
	}
	if(result==0) printf("Equal\n");
	return 0;
}

void display()
{
	for(int i=0;i<SIZE;i++)
	{
		for(int j=0;j<SIZE;j++)
		{
			printf("%c ",scane[i][j]);
		}
		printf("\n");
	}
}

void AI_input(int x,int y)
{	
	int grade_sum[8];
	grade_sum[0]=grade[0][0]+grade[0][1]+grade[0][2];
	grade_sum[1]=grade[1][0]+grade[1][1]+grade[1][2];
	grade_sum[2]=grade[2][0]+grade[2][1]+grade[2][2];
	grade_sum[3]=grade[0][0]+grade[1][0]+grade[2][0];
	grade_sum[4]=grade[0][1]+grade[1][1]+grade[2][1];
	grade_sum[5]=grade[0][2]+grade[1][2]+grade[2][2];
	grade_sum[6]=grade[0][0]+grade[1][1]+grade[2][2];
	grade_sum[7]=grade[0][2]+grade[1][1]+grade[2][0];
	for(int i=0;i<8;i++){
		if(grade_sum[i]%10==8){
			if(i==0){
				if(grade[0][0]%10==0){
					grade[0][0]=2;
					scane[0][0]='x';
				}
				else if(grade[0][1]%10==0){
					grade[0][1]=2;
					scane[0][1]='x';
				}
				else if(grade[0][2]%10==0){
					grade[0][2]=2;
					scane[0][2]='x';
				}
			}
			else if(i==1){
				if(grade[1][0]%10==0){
					grade[1][0]=1;
					scane[1][0]='x';
				}
				else if(grade[1][1]%10==0){
					grade[1][1]=1;
					scane[1][1]='x';
				}
				else if(grade[1][2]%10==0){
					grade[1][2]=1;
					scane[1][2]='x';
				}
			}
			else if(i==2){
				if(grade[2][0]%10==0){
					grade[2][0]=1;
					scane[2][0]='x';
				}
				else if(grade[2][1]%10==0){
					grade[2][1]=1;
					scane[2][1]='x';
				}
				else if(grade[2][2]%10==0){
					grade[2][2]=1;
					scane[2][2]='x';
				}
			}
			else if(i==3){
				if(grade[0][0]%10==0){
					grade[0][0]=1;
					scane[0][0]='x';
				}
				else if(grade[1][0]%10==0){
					grade[1][0]=1;
					scane[1][0]='x';
				}
				else if(grade[2][0]%10==0){
					grade[2][0]=1;
					scane[2][0]='x';
				}
			}
			else if(i==4){
				if(grade[0][1]%10==0){
					grade[0][1]=1;
					scane[0][1]='x';
				}
				else if(grade[1][1]%10==0){
					grade[1][1]=1;
					scane[1][1]='x';
				}
				else if(grade[2][1]%10==0){
					grade[2][1]=1;
					scane[2][1]='x';
				}
			}
			else if(i==5){
				if(grade[0][2]%10==0){
					grade[0][2]=1;
					scane[0][2]='x';
				}
				else if(grade[1][2]%10==0){
					grade[1][2]=1;
					scane[1][2]='x';
				}
				else if(grade[2][2]%10==0){
					grade[2][2]=1;
					scane[2][2]='x';
				}
			}
			else if(i==6){
				if(grade[0][0]%10==0){
					grade[0][0]=1;
					scane[0][0]='x';
				}
				else if(grade[1][1]%10==0){
					grade[1][1]=1;
					scane[1][1]='x';
				}
				else if(grade[2][2]%10==0){
					grade[2][2]=1;
					scane[2][2]='x';
				}
			}
			else if(i==7){
				if(grade[0][2]%10==0){
					grade[0][2]=1;
					scane[0][2]='x';
				}
				else if(grade[1][1]%10==0){
					grade[1][1]=1;
					scane[1][1]='x';
				}
				else if(grade[2][0]%10==0){
					grade[2][0]=1;
					scane[2][0]='x';
				}
			}
			return;
		}
	}
	
	int max=0;
	int max_x;
	int max_y;
	for(int i=0;i<SIZE;i++)
	{
		for(int j=0;j<SIZE;j++)
		{
			if(grade[i][j]>max){
				max=grade[i][j];
				max_x=i;
				max_y=j;
			} 
		}
		
	}
	grade[max_x][max_y]=1;
	scane[max_x][max_y]='x';
}

int check()
{
	int grade_sum[8];
	grade_sum[0]=grade[0][0]+grade[0][1]+grade[0][2];
	grade_sum[1]=grade[1][0]+grade[1][1]+grade[1][2];
	grade_sum[2]=grade[2][0]+grade[2][1]+grade[2][2];
	grade_sum[3]=grade[0][0]+grade[1][0]+grade[2][0];
	grade_sum[4]=grade[0][1]+grade[1][1]+grade[2][1];
	grade_sum[5]=grade[0][2]+grade[1][2]+grade[2][2];
	grade_sum[6]=grade[0][0]+grade[1][1]+grade[2][2];
	grade_sum[7]=grade[0][2]+grade[1][1]+grade[2][0];
	for(int i=0;i<8;i++)
	{
		if(grade_sum[i]==12) return 1;
		else if(grade_sum[i]==3) return 2;
		else return 0;
	}
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Boom
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Yakang Li, 2018
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

char map_hidden[9][9]={
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
'*','*','*','*','*','*','*','*','*',
};



void show_map();
int win();

int mainboom()
{
	int i,j;
	for(i=0;i<9;i++){
		for(j=0;j<9;j++){
			map_hidden[i][j]='*';
		}
	}
	char map[9][9]={
	'X','2','1','1','1','1','1','X','1',
	'2','X','2','3','X','2','2','2','2',
	'1','2','X','3','X','3','2','X','1',
	'1','2','3','3','3','3','X','3','2',
	'1','X','2','X','3','X','3','3','X',
	'1','1','3','3','X','3','X','2','1',
	'1','2','2','X','3','3','3','3','2',
	'X','3','X','3','3','X','3','X','X',
	'2','X','3','X','2','1','3','X','3',
	};
	printf("Welcome to minesweep!\n");
	show_map();
	
	while(1){
		printf("Please input(x y):");
		char input[4];
		for(int i=0;i<4;i++)
		{
		    input[i]=0;	

		}
		char x,y;
		//scanf("%d,%d",&x,&y);
		int r=read(0,input,4);
		input[r]=0;
		x=input[0]-48;
		y=input[2]-48;
		
		if(x+48=='q'){
		   printf("Game Over\n"); 
		   return 0;
		    
		}
		while(1){
			if(x>0&&x<=9&&y>0&&y<=9&&map_hidden[x-1][y-1]=='*') break;
			else{
				printf("Error,please input again:");
				int r=read(0,input,4);
				input[r]=0;
				x=input[0]-48;
				y=input[2]-48;
		
				if(x+48=='q'){
				   printf("Game Over\n"); 
				   return 0;
				    
				}
			}
		}
		
		int m=x-1;
		int n=y-1;
		
		if(map[m][n]=='X'){
			printf("Game Over!\n");
			return 0;
		}
		int i,j; 
		for(i=m-1;i<x+1;i++){
			if(i>=0&&i<=8){
				for(j=n-1;j<y+1;j++){
					if(j>=0&&j<=8){
						if(map[i][j]!='X')map_hidden[i][j]=map[i][j];
					}
				}
			}
			
		}
		show_map();
		if(win()){
			printf("You Win!\n");
			break;
		}
	}
	return 0;
}

void show_map()
{
	int i,j;
	for(i=0;i<9;i++){
		for(j=0;j<9;j++){
			printf("%c ",map_hidden[i][j]);
		}
		printf("\n");
	}
}

int win(){
	int sum=0;
	int i,j;
	for(i=0;i<9;i++){
		for(j=0;j<9;j++){
			if(map_hidden[i][j]=='*') sum++;
		}
	}
	if(sum==23) return 1;
	else return 0;
}

