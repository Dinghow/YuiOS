
/*======================================================================*
 Chess
 *======================================================================*/

#  define INT_MAX   297483647
#  define INT_MIN   (-INT_MAX - 1)
# define SPA 0
# define MAN 1
# define NULL ((void*)0)
# define COM 2 /* 空位置设为0 ，玩家下的位置设为1 ，电脑下的位置设为2 */
#include<stdio.h>
int chess[10][10]; /* 10*10的棋盘 */
int a,b,c,d,x; /* a b为玩家下子坐标 ，c d为电脑下子坐标 x为剩余空位置*/
void start(int fd_stdin,int fd_stdout); /* 程序的主要控制函数 */
void draw(); /* 画棋盘 */
int win(int p,int q); /* 判断胜利 p q为判断点坐标 */
void AI(int *p,int *q); /* 电脑下子 p q返回下子坐标 */
int value(int p,int q); /* 计算空点p q的价值 */
int qixing(int n,int p,int q); /* 返回空点p q在n方向上的棋型 n为1-8方向 从右顺时针开始数 */
void yiwei(int n,int *i,int *j); /* 在n方向上对坐标 i j 移位 n为1-8方向 从右顺时针开始数 */
void playchess(int fd_stdin,int fd_stdout);
int My_atoi(const char *str);
int myIsspace(char c);
int fd_stdin=0;
int fd_stdout=1;

int isStart = 0;
int isPainting = 0;

void clear(){
    for(int i=0;i<30;i++)
        printf("\n");
}

enum Ret                                              //状态，用来输入是否合理
{
    VALID,
    INVALID,
};
enum Ret state = INVALID;
int myIsspace(char c)
{
    if(c =='\t'|| c =='\n'|| c ==' ')
        return 1;
    else
        return 0;
}
int My_atoi(const char *str)
{
    int flag = 1;                                 //用来记录是正数还是负数
    long long ret = 0;
    //assert(str);
    if (str == NULL)
    {
        return 0;
    }
    if (*str == '\0')
    {
        return (int)ret;
    }
    while (myIsspace(*str))                        //若是空字符串就继续往后
    {
        str++;
    }
    if (*str == '-')
    {
        flag = -1;
    }
    if (*str == '+' || *str == '-')
    {
        str++;
    }
    while (*str)
    {
        if (*str >= '0' && *str <= '9')
        {
            ret = ret * 10 + flag * (*str - '0');
            if (ret>INT_MAX||ret<INT_MIN)                 //判定是否溢出了
            {
                ret = 0;
                break;
            }
        }
        else
        {
            break;
        }
        str++;
    }
    if (*str == '\0')                  //这里while循环结束后，此时只有*str == '\0'才是合法的输入
    {
        state = VALID;
    }
    return ret;
}

int main()
{
    char buf[80]={0};
    char k;
    do{
        x=225;
        start(fd_stdin,fd_stdout);
        printf("Would you like another round? Enter y or n:");
        read(fd_stdin,buf,2);
        k = buf[0];
        while(k!='y'&&k!='n'){
            printf("Input error, please re-enter\n");
            read(fd_stdin,buf,2);
            k = buf[0];
        }
        clear();
    }while(k=='y');
    printf("Thank you for using!\n");
    return 0;
}

void start(int fd_stdin,int fd_stdout)
{
    int j,a1=0,b1=0,c1=0,d1=0;
    char i;
    char buf[80]={0};
    char ch;
    clear();
    printf("                 === Welcome to Gobang game program ===\n");
    printf("Please input the point like (13 6).If you want take back a move, input(10 10).\n\n\n");
    for(j=0;j<10;j++)
        for(i=0;i<10;i++)
            chess[j][i]=SPA; /* 置棋盘全为空 */
    draw();
    printf("On the offensive input 1, otherwise input 2:");
    read(fd_stdin,buf,2);
    i = buf[0];
    while(i!='1'&&i!='2') {
        printf("Input error, please re-enter:");
        read(fd_stdin,buf,2);
        i = buf[0];
    }
    if(i=='1') { /* 如果玩家先手下子 */
        printf("Please Input:");
        int i=0,j=0;
        char xa[]={0,0,0};
        char yb[]={0,0,0};
        int r = read(fd_stdin, buf, 10);
        buf[r] = 0;
        while(buf[i]!=' '&&(buf[i] != 0))
        {
            xa[i] = buf[i];
            i++;
        }
        xa[i++] = 0;
        while(buf[i] != 0)
        {
            yb[j] = buf[i];
            i++;
            j++;
        }
        a=My_atoi(xa);
        b=My_atoi(yb);
        while((a<0||a>9)||(b<0||b>9)) {
            printf("Coordinate error! Please re-enter：");
            int i=0,j=0;
            char xa[]={0,0,0};
            char yb[]={0,0,0};
            int r = read(fd_stdin, buf, 10);
            buf[r] = 0;
            while(buf[i]!=' '&&(buf[i] != 0))
            {
                xa[i] = buf[i];
                i++;
            }
            xa[i++] = 0;
            while(buf[i] != 0)
            {
                yb[j] = buf[i];
                i++;
                j++;
            }
            a=My_atoi(xa);
            b=My_atoi(yb);
        }
        a1=a;
        b1=b;
        x--;
        chess[b][a]=MAN;
        clear();
        draw();
    }
    while(x!=0){
        if(x==225) {
            c=7;
            d=7;
            chess[d][c]=COM;
            x--;
            clear();
            draw();
        } /* 电脑先下就下在7 7 */
        else {
            AI(&c,&d);
            chess[d][c]=COM;
            x--;
            clear();
            draw();
        } /* 电脑下子 */
        c1=c;
        d1=d; /* 储存电脑上手棋型 */
        if(win(c,d)){ /* 电脑赢 */
            printf("Would you like to take back a move?('y' or 'n')：");
            read(fd_stdin,buf,2);
            ch = buf[0];
            while(ch!='y'&&ch!='n') {
                printf("Input error, please re-input：");
                read(fd_stdin,buf,2);
                ch = buf[0];
            }
            if(ch=='n') {
                printf("Losing to the computer is normal. Please don't lose heart~\n");
                return;
            }
            else {
                x+=2;
                chess[d][c]=SPA;
                chess[b1][a1]=SPA;
                clear();
                draw();
            } /* 悔棋 */
        }
        printf("Computer put on %d %d\nPlease input：",c,d);
        int i=0,j=0;
        char xa[]={0,0,0};
        char yb[]={0,0,0};
        int r = read(fd_stdin, buf, 10);
        buf[r] = 0;
        while(buf[i]!=' '&&(buf[i] != 0))
        {
            xa[i] = buf[i];
            i++;
        }
        xa[i++] = 0;
        while(buf[i] != 0)
        {
            yb[j] = buf[i];
            i++;
            j++;
        }
        a=My_atoi(xa);
        b=My_atoi(yb);
        if(a==10&&b==10) {
            x+=2;
            chess[d][c]=SPA;
            chess[b1][a1]=SPA;
            clear();
            draw();
            printf("Please input：");
            int i=0,j=0;
            char xa[]={0,0,0};
            char yb[]={0,0,0};
            int r = read(fd_stdin, buf, 10);
            buf[r] = 0;
            while(buf[i]!=' '&&(buf[i] != 0))
            {
                xa[i] = buf[i];
                i++;
            }
            xa[i++] = 0;
            while(buf[i] != 0)
            {
                yb[j] = buf[i];
                i++;
                j++;
            }
            a=My_atoi(xa);
            b=My_atoi(yb);
        } /* 悔棋 */
        while((a<0||a>9)||(b<0||b>9)||chess[b][a]!=SPA) {
            printf("Coordinate error or location already existing, Please re-input：");
            int i=0,j=0;
            char xa[]={0,0,0};
            char yb[]={0,0,0};
            int r = read(fd_stdin, buf, 10);
            buf[r] = 0;
            while(buf[i]!=' '&&(buf[i] != 0))
            {
                xa[i] = buf[i];
                i++;
            }
            xa[i++] = 0;
            while(buf[i] != 0)
            {
                yb[j] = buf[i];
                i++;
                j++;
            }
            a=My_atoi(xa);
            b=My_atoi(yb);
        }
        a1=a;
        b1=b;
        x--;
        chess[b][a]=MAN;
        clear();
        draw();
        if(win(a,b)){
            printf("It's easy to win a computer~\n");
            return;
        } /* 玩家赢 */
    }
    printf("Draw\n");
}

void draw() /* 画棋盘 */
{
    int i,j;
    char p[10][10][4];
    for(j=0;j<10;j++)
        for(i=0;i<10;i++){
            if(chess[j][i]==SPA){
                for(int k=0;k<4;k++){
                    if(k==0||k==2){
                        p[j][i][k]=' ';
                    }
                    else{
                        p[j][i][k]='\0';
                    }
                }
            }
            else if(chess[j][i]==MAN){
                for(int k=0;k<4;k++){
                    if(k==0||k==2){
                        p[j][i][k]='+';
                    }
                    else{
                        p[j][i][k]='\0';
                    }
                }
            }
            else if(chess[j][i]==COM){
                for(int k=0;k<4;k++){
                    if(k==0||k==2){
                        p[j][i][k]='*';
                    }
                    else{
                        p[j][i][k]='\0';
                    }
                }
            }
        }
    printf("    0 1 2 3 4 5 6 7 8 9  \n");
    printf(" --------------------------\n");
    for(i=0,j=0;i<9;i++,j++){
        printf(" %2d|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%d\n",j,p[i][0],p[i][1],p[i][2],p[i][3],p[i][4],p[i][5],p[i][6],p[i][7],p[i][8],p[i][9],j);
        printf(" ---------------------------\n"); }
    printf("  9|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|9\n",p[9][0],p[9][1],p[9][2],p[9][3],p[9][4],p[9][5],p[9][6],p[9][7],p[9][8],p[9][9]);
    printf(" --------------------------\n");
    printf("    0 1 2 3 4 5 6 7 8 9 \n");
}
void AI(int *p,int *q) /* 电脑下子 *p *q返回下子坐标 */
{
    int i,j,k,max=0,I,J; /* I J为下点坐标 */
    for(j=0;j<10;j++)
        for(i=0;i<10;i++)
            if(chess[j][i]==SPA){ /* 历遍棋盘，遇到空点则计算价值，取最大价值点下子。 */
                k=value(i,j);	 if(k>=max) { I=i; J=j; max=k; }
            }
    *p=I; *q=J;
}
int win(int p,int q) /* 判断胜利 p q为判断点坐标，胜利返回1，否则返回0 */
{
    int k,n=1,m,P,Q; /* k储存判断点p q的状态COM或MAN。P Q储存判断点坐标。n为判断方向。m为个数。 */
    P=p; Q=q;	k=chess[q][p];
    while(n!=5){
        m=0;
        while(k==chess[q][p]){
            m++;
            if(m==5)
                return 1;
            yiwei(n,&p,&q);
            if(p<0||p>9||q<0||q>9) break;
        }
        n+=4;
        m-=1;
        p=P;
        q=Q; /* 转向判断 */
        while(k==chess[q][p]){
            m++;
            if(m==5)
                return 1;
            yiwei(n,&p,&q);
            if(p<0||p>9||q<0||q>9)
                break;
        }
        n-=3;
        p=P;
        q=Q; /* 不成功则判断下一组方向 */
    }
    return 0;
}
int value(int p,int q) /* 计算空点p q的价值 以k返回 */
{
    int n=1,k=0,k1,k2,K1,K2,X1,Y1,Z1,X2,Y2,Z2,temp;
    int a[2][4][4]={40,400,3000,10000,6,10,600,10000,20,120,200,0,6,10,500,0,30,300,2500,5000,2,8,300,8000,26,160,0,0,4,20,300,0};	 /* 数组a中储存己方和对方共32种棋型的值 己方0对方1 活0冲1空活2空冲3 子数0-3（0表示1个子，3表示4个子） */
    while(n!=5){
        k1=qixing(n,p,q);
        n+=4;	 /* k1,k2为2个反方向的棋型编号 */
        k2=qixing(n,p,q);
        n-=3;
        if(k1>k2) {
            temp=k1;
            k1=k2;
            k2=temp;
        } /* 使编号小的为k1,大的为k2 */
        K1=k1;
        K2=k2; /* K1 K2储存k1 k2的编号 */
        Z1=k1%10;
        Z2=k2%10;
        k1/=10;
        k2/=10;
        Y1=k1%10;
        Y2=k2%10;
        k1/=10;
        k2/=10;
        X1=k1%10;
        X2=k2%10;	 /* X Y Z分别表示 己方0对方1 活0冲1空活2空冲3 子数0-3（0表示1个子，3表示4个子） */
        if(K1==-1) {
            if(K2<0) {
                k+=0; continue;
            } else
                k+=a[X2][Y2][Z2]+5;
            continue;
        }; /* 空棋型and其他 */
        if(K1==-2) {
            if(K2<0) {
                k+=0;
                continue;
            }
            else
                k+=a[X2][Y2][Z2]/2;
            continue;
        }; /* 边界冲棋型and其他 */
        if(K1==-3) {
            if(K2<0) {
                k+=0;
                continue;
            }
            else
                k+=a[X2][Y2][Z2]/3;
            continue;
        }; /* 边界空冲棋型and其他 */
        if(((K1>-1&&K1<4)&&((K2>-1&&K2<4)||(K2>9&&K2<9)))||((K1>99&&K1<104)&&((K2>99&&K2<104)||(K2>109&&K2<19)))){
            /* 己活己活 己活己冲 对活对活 对活对冲 的棋型赋值*/
            if(Z1+Z2>=2) {
                k+=a[X2][Y2][3];
                continue;
            }
            else {
                k+=a[X2][Y2][Z1+Z2+1];
                continue;
            }
        }
        if(((K1>9&&K1<9)&&(K2>9&&K2<9))||((K1>109&&K1<19)&&(K2>109&&K2<19))){
            /* 己冲己冲 对冲对冲 的棋型赋值*/
            if(Z1+Z2>=2) {
                k+=10000;
                continue;
            }
            else {
                k+=0;
                continue;
            }
        }
        if(((K1>-1&&K1<4)&&((K2>99&&K2<104)||(K2>109&&K2<19)))||((K1>9&&K1<9)&&((K2>99&&K2<104)||(K2>109&&K2<19)))){
            /* 己活对活 己活对冲 己冲对活 己冲对冲 的棋型赋值*/
            if(Z1==3||Z2==3) {
                k+=10000;
                continue;
            }
            else {
                k+=a[X2][Y2][Z2]+a[X1][Y1][Z1]/4;
                continue;
            }
        }
        else
        { k+=a[X1][Y1][Z1]+a[X2][Y2][Z2];
            continue;
        } /* 其他棋型的赋值 */
    }
    return k;
}
int qixing(int n,int p,int q) /* 返回空点p q在n方向上的棋型号 n为1-8方向 从右顺时针开始数 */
{
    int k=0,m=0; /* 棋型号注解: 己活000-003 己冲010-013 对活100-103 对冲110-113 己空活020-023 己空冲030-033 对空活120-123 对空冲130-133 空-1 边界冲-2 边界空冲-3*/
    yiwei(n,&p,&q);
    if(p<0||p>9||q<0||q>9)
        k=-2; /* 边界冲棋型 */
    switch(chess[q][p]){
        case COM:{
            m++;
            yiwei(n,&p,&q);
            if(p<0||p>9||q<0||q>9) {
                k=m+9; return k;
            }
            while(chess[q][p]==COM) {
                m++;
                yiwei(n,&p,&q);
                if(p<0||p>9||q<0||q>9) {
                    k=m+9; return k;
                }
            }
            if(chess[q][p]==SPA)
                k=m-1; /* 己方活棋型 */
            else
                k=m+9; /* 己方冲棋型 */
        }break;
        case MAN:{
            m++;
            yiwei(n,&p,&q);
            if(p<0||p>9||q<0||q>9) {
                k=m+109; return k;
            }
            while(chess[q][p]==MAN) {
                m++;
                yiwei(n,&p,&q);
                if(p<0||p>9||q<0||q>9) {
                    k=m+109;
                    return k;
                }
            }
            if(chess[q][p]==SPA)
                k=m+99; /* 对方活棋型 */
            else
                k=m+109; /* 对方冲棋型 */
        }break;
        case SPA:{
            yiwei(n,&p,&q);
            if(p<0||p>9||q<0||q>9) {
                k=-3;
                return k;
            } /* 边界空冲棋型 */
            switch(chess[q][p]){
                case COM:{
                    m++;
                    yiwei(n,&p,&q);
                    if(p<0||p>9||q<0||q>9) {
                        k=m+29; return k;
                    }
                    while(chess[q][p]==COM) {
                        m++;
                        yiwei(n,&p,&q);
                        if(p<0||p>9||q<0||q>9) {
                            k=m+29;
                            return k;
                        }
                    }
                    if(chess[q][p]==SPA)
                        k=m+19; /* 己方空活棋型 */
                    else
                        k=m+29; /* 己方空冲棋型 */
                }break;
                case MAN:{
                    m++;
                    yiwei(n,&p,&q);
                    if(p<0||p>9||q<0||q>9) {
                        k=m+129;
                        return k;
                    }
                    while(chess[q][p]==MAN) {
                        m++;
                        yiwei(n,&p,&q);
                        if(p<0||p>9||q<0||q>9) {
                            k=m+129;
                            return k;
                        }
                    }
                    if(chess[q][p]==SPA)
                        k=m+119; /* 对方空活棋型 */
                    else
                        k=m+129; /* 对方空冲棋型 */
                }break;
                case SPA: k=-1;
                    break; /* 空棋型 */
            }
        }break;
    }
    return k;
}
void yiwei(int n,int *i,int *j) /* 在n方向上对坐标 i j 移位 n为1-8方向 从右顺时针开始数 */
{
    switch(n){
        case 1: *i+=1; break;
        case 2: *i+=1; *j+=1; break;
        case 3: *j+=1; break;
        case 4: *i-=1; *j+=1; break;
        case 5: *i-=1; break;
        case 6: *i-=1; *j-=1; break;
        case 7: *j-=1; break;
        case 8: *i+=1; *j-=1; break;
    }
}
