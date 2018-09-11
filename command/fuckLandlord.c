#include<stdio.h>

unsigned int _seed2 = 0xDEADBEEF;

void srand(unsigned int seed){
    _seed2 = seed;
}

int rand() {
    unsigned int next = _seed2;
    unsigned int result;
    
    next *= 1103515245;
    next += 12345;
    result = ( unsigned int  ) ( next / 65536 ) % 2048;
    
    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= ( unsigned int ) ( next / 65536 ) % 1024;
    
    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= ( unsigned int ) ( next / 65536 ) % 1024;
    
    _seed2 = next;
    
    return result;
}

char cardsNum[20]={0,0,0,'3','4','5','6','7','8','9','10','J','Q','K','A','2','B','R'};

int cards[54];
void cardsInit();
void printCards();
void randomSort();
void distributeCards();
void printArray(int *a,int len);
void cardsSort(int* a, int n);
void sortPlayerCards();
void statisticCardsCnt();
int search1(int p,int judge,int v);
int search2(int p,int judge,int v);
int search3(int p,int v);
int search4(int p,int v);
int search5(int p,int min,int len,int cnt);
void deleteCards(int p,int n,int c);
int ai(int p);
int bang(int p,int v);
void initNowCard();
int humanPlayer();
int check();
void game();

int landlordCards[3];
int computerPlayer[2][20];
int human[20];
int landlord;
int nowPlayer;//现在出牌者

int now;//0:任意 1:单牌 2:对子 3:三个不带 4:三带单 5:三带对 6:顺子 7:连对 8:飞机不带 9:飞机带单 10:飞机带对 11:炸弹
int length;//顺子长度 连对长度 飞机长度
int value;//单排、对子、三、炸弹当前，顺子当前最小，连对当前最小，飞机当前最小

int cntOfCards[2][18];//3~13 14=1 15=2 16 17
int nowCards[20];
int maxCards[20];
int maxPlayer;

char num2card(int n){
    if(n==-1)
        return 'B';
    else if(n==-2)
        return 'R';
    else if(n==1)
        return 'A';
    else if(n==11)
        return 'J';
    else if(n==12)
        return 'Q';
    else if(n==13)
        return 'K';
    else return n+'0';
}

int main()
{
    /*
     cardsInit();
     //printCards();
     randomSort();
     //printCards();
     distributeCards();
     printArray(human, 20);
     printArray(computerPlayer[0], 20);
     printArray(computerPlayer[1], 20);
     sortPlayerCards();
     printArray(human, 20);
     printArray(computerPlayer[0], 20);
     printArray(computerPlayer[1], 20);
     statisticCardsCnt();
     now=6;
     value=3;
     length=5;
     ai(0);*/
    for(int i=0;i<20;i++){
        nowCards[i]=0;
        maxCards[i]=0;
    }
    cardsInit();
    randomSort();
    distributeCards();
    sortPlayerCards();
    //printf("--1--\n");
    //printArray(computerPlayer[0], 20);
    //printArray(computerPlayer[1], 20);
    statisticCardsCnt();
    //printf("--2--\n");
    //printArray(computerPlayer[0], 20);
    //printArray(computerPlayer[1], 20);
    //printArray(human, 20);
    game();
    //0:任意 1:单牌 2:对子 3:三个不带 4:三带单 5:三带对 6:顺子 7:连对 8:飞机不带 9:飞机带单 10:飞机带对 11:炸弹
    //now_now,now_length,now_value
    //check();
    
    return 0;
}



void printArray(int *a,int len){
    for(int i=0;i<len;i++){
        if(a[i]==10)
            printf("10 ");
        else
            printf("%c ",num2card(a[i]));
    }
    printf("\n");
}

void cardsInit(){
    for(int i=1;i<=13;i++){
        for(int j=1;j<=4;j++){
            cards[i+13*(j-1)]=i;
        }
    }
    cards[0]=-1;//黑鬼
    cards[53]=-2;//红鬼
}

void printCards(){
    for(int i=0;i<54;i++){
        printf("%d ",cards[i]);
    }
    printf("\n");
}

void randomSort(){
    int res[54];
    for(int i=54;i>=1;i--){
        int cnt=rand()%i;
        res[54-i]=cards[cnt];
        cards[cnt]=0;
        for(int j=cnt;j<i-1;j++){
            cards[j]=cards[j+1];
        }
    }
    for(int i=0;i<54;i++){
        cards[i]=res[i];
    }
    printf("landlord's cards: ");
    for(int i=0;i<3;i++){
        landlordCards[i]=cards[i];
        printf("%c ",num2card(landlordCards[i]));
    }
    printf("\n");
    for(int i=0;i<51;i++){
        cards[i]=cards[i+3];
    }
    cards[51]=0;
    cards[52]=0;
    cards[53]=0;
}

void distributeCards(){
    for(int i=0;i<2;i++){
        for(int j=0;j<20;j++){
            computerPlayer[i][j]=0;
        }
    }
    for(int i=0;i<20;i++)
        human[i]=0;
    for(int i=0;i<17;i++){
        for(int j=0;j<2;j++){
            computerPlayer[j][i]=cards[3*i+j];
        }
        human[i]=cards[3*i+2];
    }
    landlord=rand()%3;
    if(landlord<2){
        for(int i=0;i<3;i++){
            computerPlayer[landlord][17+i]=landlordCards[i];
        }
    }else{
        for(int i=0;i<3;i++){
            human[17+i]=landlordCards[i];
        }
    }
}

void statisticCardsCnt(){
    for(int i=0;i<18;i++){
        cntOfCards[0][i]=0;
        cntOfCards[1][i]=0;
    }
//printf("---1.5---");
    for(int i=0;i<2;i++){
        for(int j=0;j<20;j++){
	    //printf("i: %d , j: %d \n",i,j);
            if(computerPlayer[i][j]<0)
                cntOfCards[i][15-computerPlayer[i][j]]++;
            else if(computerPlayer[i][j]==1||computerPlayer[i][j]==2)
                cntOfCards[i][13+computerPlayer[i][j]]++;
            else cntOfCards[i][computerPlayer[i][j]]++;
            
        }
    }
}

int compareCard(int a,int b){
    if(a==0) return 1;
    if(b==0) return 0;
    if(a==-2)
        return 1;
    if(b==-2)
        return 0;
    if(a==-1)
        return 1;
    if(b==-1)
        return 0;
    if(a<=2&&a>0) a+=13;
    if(b<=2&&b>0) b+=13;
    if(a>b)
        return 1;
    else return 0;
}

void cardsSort(int* a, int n){
    int i, j, temp;
    for (j = 0; j < n - 1; j++)
        for (i = 0; i < n - 1 - j; i++){
            if(compareCard(a[i], a[i+1])==1){
                temp = a[i];
                a[i] = a[i + 1];
                a[i + 1] = temp;
            }
        }
}

void sortPlayerCards(){
    cardsSort(human, 20);
    cardsSort(computerPlayer[0], 20);
    cardsSort(computerPlayer[1], 20);
}

int grel(int res){
    if(res==14||res==15)
        return res-13;
    if(res==16||res==17)
        return 15-res;
    return res;
}

int ai(int p){
    switch(now){
        case 0:{
            int res=search1(p,0,2);
            int rel=grel(res);
            deleteCards(p,rel,1);
            initNowCard();
            nowCards[0]=res;
            return 1;//出单
            break;
        }
        case 1:{
            int res=search1(p,0,value);
            int rel=grel(res);
            if(res!=0){//出牌
                deleteCards(p,rel,1);
                initNowCard();
                nowCards[0]=res;
                return 1;//出单
            }
            else{
                return bang(p,2);//出炸弹
            }
            return 0;//不出
            break;
        }
        case 2:{
            int res=search2(p,0,value);
            int rel=rel=grel(res);
            if(res!=0){
                initNowCard();
                nowCards[0]=res;
                nowCards[1]=res;
                deleteCards(p,rel,2);
                return 1;
            }
            else{
                return bang(p,2);//出炸弹
            }
            return 0;//不出
            break;
        }
        case 3:{
            int res=search3(p,value);
            int rel=grel(res);
            if(res!=0){
                deleteCards(p,rel,3);
                initNowCard();
                nowCards[0]=res;
                nowCards[1]=res;
                nowCards[2]=res;
                return 1;
            }
            else{
                return bang(p,2);//出炸弹
            }
            return 0;//不出
            break;
        }
        case 4:{
            int res=search3(p,value);
            int rel=grel(res);
            if(res==0){
                return bang(p,2);//出炸弹
            }
            int dres=search1(p,res,2);
            int drel=grel(dres);
            if(dres!=0){
                deleteCards(p,drel,1);
                deleteCards(p,rel,3);
                initNowCard();
                nowCards[0]=res;
                nowCards[1]=res;
                nowCards[2]=res;
                nowCards[3]=dres;
                return 1;//三代一
            }
            return 0;//不出
            break;
        }
        case 5:{
            int res=search3(p,value);
            int rel=grel(res);
            if(res==0){
                return bang(p,2);//出炸弹
            }
            int dres=search2(p,res,2);
            int drel=grel(dres);
            if(dres!=0){
                deleteCards(p,drel,2);
                deleteCards(p,rel,3);
                initNowCard();
                nowCards[0]=res;
                nowCards[1]=res;
                nowCards[2]=res;
                nowCards[3]=dres;
                nowCards[4]=dres;
                return 1;//三代二
            }
            return 0;//不出
            break;
        }
        case 6:{
            int res=search5(p,value+1,length,1);
            int rel=grel(res);
            if(res!=0){
                initNowCard();
                for(int i=0;i<length;i++){
                    nowCards[i]=rel+i;
                    if(rel+i>13){
                        deleteCards(p,1,1);
                        continue;
                    }
                    deleteCards(p,rel+i,1);
                }
                return 1;
            }
            else{
                return bang(p,2);//出炸弹
            }
            return 0;
            break;
        }
        case 7:{
            int res=search5(p,value+1,length,2);
            int rel=grel(res);
            if(res!=0){
                initNowCard();
                for(int i=0;i<length;i++){
                    nowCards[2*i]=rel+i;
                    nowCards[2*i+1]=rel+i;
                    if(rel>13){
                        deleteCards(p,1,2);
                        continue;
                    }
                    deleteCards(p,rel+i,2);
                }
                return 1;
            }
            else{
                return bang(p,2);//出炸弹
            }
            return 0;
            break;
        }
        case 8:{
            return bang(p,2);//出炸弹
            break;
        }
        case 9:{
            return bang(p,2);//出炸弹
            break;
        }
        case 10:{
            return bang(p,2);//出炸弹
            break;
        }
        case 11:{
            return bang(p,value);//出炸弹
            break;
        }
    }
    return 0;
}

int search1(int p,int judge,int v){//单
    int cc=0;
    for(int i=value+1;i<=17;i++){
        if(i==judge)
            continue;
        if(cntOfCards[p][i]==1){
            cc=i;
            break;
        }
        if(cntOfCards[p][i]>1&&cc==0)
            cc=i;
    }
    return cc;
}

int search2(int p,int judge,int v){//对子
    int cc=0;
    for(int i=v+1;i<=17;i++){
        if(i==judge)
            continue;
        if(cntOfCards[p][i]==2){
            cc=i;
            break;
        }
        if(cntOfCards[p][i]>2&&cc==0)
            cc=i;
    }
    return cc;
}

int search3(int p,int v){//三
    int cc=0;
    for(int i=v+1;i<=17;i++){
        if(cntOfCards[p][i]==3){
            cc=i;
            break;
        }
        if(cntOfCards[p][i]>3&&cc==0)
            cc=i;
    }
    return cc;
}

int search4(int p,int v){//非炸情况 v=2
    int cc=0;
    for(int i=v+1;i<=17;i++){
        if(cntOfCards[p][i]==4){
            cc=i;
            break;
        }
    }
    if(cc==0){
        if(cntOfCards[p][16]==1&&cntOfCards[p][17]==1)
            return -1;//王炸
    }
    return cc;
}

int search5(int p,int min,int len,int cnt){//顺子、连对、飞机
    int cc=0;
    for(int i=min;i<=13;i++){
        if(min+len-1>14)
            break;
        int d=1;
        for(int j=i;j<i+len;j++){
            if(cntOfCards[p][j]<cnt){
                d=0;
                break;
            }
        }
        if(d==1){
            cc=i;
            break;
        }
    }
    return cc;
}

void deleteCards(int p,int n,int c){//player,number,count
    /*int* player=p==3?human:computerPlayer[p];
    for(int i=0;i<20;i++){
        if(c==0)
            break;
        if(player[i]==c){
            player[i]=0;
            c--;
        }
    }*/
}

int bang(int p,int v){
    int res=search4(p, v);
    if(res==-1){
        deleteCards(p,-1,1);
        deleteCards(p,-2,1);
        initNowCard();
        nowCards[0]=15;
        nowCards[1]=16;
        return -1;
    }else if(res!=0){
        int rel=grel(res);
        deleteCards(p,rel,4);
        initNowCard();
        for(int i=0;i<4;i++)
            nowCards[i]=res;
        return -1;
    }
    return 0;
}

void initNowCard(){
    for(int i=0;i<20;i++){
        nowCards[i]=0;
    }
}

void displayCards(int *c,int len){
    for(int i=0;i<len;i++){
        //printf()
    }
}

void game(){
    int p=landlord;
    value=0;
    length=0;
    now=0;
    //int c=0;
    maxPlayer=landlord;
    
    printf("player %d is landlord\n",landlord+1);
    printf("------\n");
    while(1){
        if(p<2){
            if(computerPlayer[p][0]==0){
                printf("computer player %d win\n",p);
                return;
            }
        }else{
            if(human[0]==0){
                printf("you win\n");
                return;
            }
        }
        int ans;
        initNowCard();
        if(maxPlayer==p){//刷新
            for(int i=0;i<20;i++){
                maxCards[i]=0;
            }
            value=0;
            length=0;
            now=0;
        }
        if(p<2){
            ans=ai(p);
            printf("palyer %d: ",p+1);
            if(!ans){
                printf("0 ");
            }
            else if(check()){
                
                for(int i=0;i<20;i++){
                    if(nowCards[i]==0)
                        break;
                    if(nowCards[i]==10)
                        printf("10 ");
                    else printf("%c ",cardsNum[nowCards[i]]);
                    int rel=grel(nowCards[i]);
                    maxCards[i]=nowCards[i];
                    for(int j=0;j<20;j++){
                        if(computerPlayer[p][j]==rel){
                            computerPlayer[p][j]=0;
                            break;
                        }
                    }
                }
                maxPlayer=p;
            }else{
                //不出
                sortPlayerCards();
                for(int i=19;i>=0;i--){
                    if(nowCards[19-i]==0)
                        break;
                    computerPlayer[p][i]=nowCards[19-i];
                }//rollback
                sortPlayerCards();
                printf("0 ");
            }
            sortPlayerCards();
            statisticCardsCnt();
            for(int i=0;i<20;i++){
                if(computerPlayer[p][i]==0){
                    printf("(%d left)\n",i);
                    break;
                }
            }
        }
        else{//human
            printArray(human, 20);
            ans=humanPlayer();
            if(ans==-1)
                return;
            printf("player 3: ");
            if(ans==0){
                /*for(int i=0;i<20;i++){
                    if(nowCards[i]==0)
                        break;
                    printf("%c ",cardsNum[nowCards[i]]);
                }*/
                printf("0 ");
            }
            else{
                for(int i=0;i<20;i++){
                    if(nowCards[i]==0)
                        break;
                    if(nowCards[i]==10)
                        printf("10 ");
                    else printf("%c ",cardsNum[nowCards[i]]);
                }
                for(int i=0;i<20;i++)
                    maxCards[i]=nowCards[i];
                maxPlayer=p;
            }
            sortPlayerCards();
            for(int i=0;i<20;i++){
                if(human[i]==0){
                    printf("(%d left)\n",i);
                    break;
                }
            }
        }
        p=(p+1)%3;
        sortPlayerCards();
        statisticCardsCnt();
        //printArray(computerPlayer[0], 20);
        //printArray(computerPlayer[1], 20);
    }
}

int humanPlayer(){
    while(1){
        printf("it's your turn:");
        char h[20];
	for(int i=0;i<20;i++)
	    h[i]=0;
        int r=read(0,h,20);
        int cnt=0;
        int flag=1;
        if(h[0]=='q'){
            printf("see you\n");
            return -1;
        }
        if(maxPlayer==2&&h[0]=='0'){
            printf("invalid\n");
            continue;
        }
        if(h[0]=='0')
            return 0;
	printf("%s\n",h);
        for(int i=0;i<20;i++){
            if(h[i]==0)
                break;
            if(h[i]=='1'&&i<19&&h[i+1]=='0'){//10
                i++;
                nowCards[cnt]=10;
                cnt++;
            }else if(h[i]=='A'){
                nowCards[cnt]=14;
                cnt++;
            }else if(h[i]=='2'){
                nowCards[cnt]=15;
                cnt++;
            }else if(h[i]=='J'){
                nowCards[cnt]=11;
                cnt++;
            }else if(h[i]=='Q'){
                nowCards[cnt]=12;
                cnt++;
            }else if(h[i]=='K'){
                nowCards[cnt]=13;
                cnt++;
            }else if(h[i]=='R'){
                nowCards[cnt]=17;
                cnt++;
            }else if(h[i]=='B'){
                nowCards[cnt]=16;
                cnt++;
            }else if(h[i]>='3'&&h[i]<='9'){
                nowCards[cnt]=h[i]-'0';
                cnt++;
            }else{
                flag=0;
                break;
            }
            int e=0;
            for(int j=0;j<20;j++){
                if(human[j]==grel(nowCards[cnt-1])){
                    human[j]=0;
                    e=1;
                    break;
                }
            }
            //printArray(human, 20);
            if(e==0){
                cnt--;
                nowCards[cnt]=0;
                flag=0;
                break;
            }
        }
        if(flag==0){//rollback
            sortPlayerCards();
            for(int i=0;i<20;i++){
                if(nowCards[i]==0)
                    break;
                human[19-i]=grel(nowCards[i]);
            }
            sortPlayerCards();
            continue;
        }
        else{
            //for(int i=0;i<20;i++)
            //    printf("%d ",nowCards[i]);
            //printf("\n");
            if(check())
                break;
            else{
                sortPlayerCards();
                for(int i=0;i<20;i++){
                    if(nowCards[i]==0)
                        break;
                    human[19-i]=grel(nowCards[i]);
                }
                sortPlayerCards();
                printf("invalid\n");
                continue;
            }
        }
    }
    sortPlayerCards();
    
    return 1;
}

int check(){
    int max_now=0;
    int max_value=0;
    int max_length=0;
    int len=0;
    for(int i=0;i<20;i++)
        if(maxCards[i]!=0)
            len++;
        else break;
    //0:任意 1:单牌 2:对子 3:三个不带 4:三带单 5:三带对 6:顺子 7:连对 8:飞机不带 9:飞机带单 10:飞机带对 11:炸弹
    if(len==2&&maxCards[0]==15){//王炸
        max_now=11;
        max_value=100;
        max_length=0;
    }else if(len==0){//任意
        max_now=0;
        max_value=0;
        max_length=0;
    }else if(len<=3){//单、对、三
        max_now=len;
        max_length=0;
        max_value=maxCards[0];
    }else if(len==4){
        if(maxCards[0]==maxCards[3]){//普通炸
            max_now=11;
            max_length=0;
            max_value=maxCards[0];
        }else{//三代一
            max_now=4;
            max_length=0;
            max_value=maxCards[0];
        }
    }else if(len==5){
        if(maxCards[0]==maxCards[1]){//三代二
            max_now=5;
            max_length=0;
            max_value=maxCards[0];
        }else{//顺子
            max_now=6;
            length=5;
            max_value=maxCards[0];
        }
    }else if(len>=6&&maxCards[0]!=maxCards[1]){//顺子
        max_now=6;
        max_length=len;
        max_value=maxCards[0];
    }else if(len>=6&&maxCards[0]!=maxCards[2]){//连对
        max_now=7;
        max_length=len/2;
        max_value=maxCards[0];
    }else{//飞机
        int pos=0;
        int cnt=0;
        while(pos<len){
            if(maxCards[pos]==maxCards[pos+1]&&maxCards[pos]==maxCards[pos+2]){
                cnt++;
                pos+=3;
            }else break;
        }
        max_now=len/cnt+5;//1，2，3
        max_length=cnt;
        max_value=maxCards[0];
    }
    
    int now_now=0;
    int now_value=0;
    int now_length=0;
    len=0;
    for(int i=0;i<20;i++)
        if(nowCards[i]!=0)
            len++;
        else break;
    if(len==1){//单
        now_now=1;
        now_value=nowCards[0];
        now_length=0;
    }else if(len==2&&nowCards[0]==15&&nowCards[0]==16){//王炸
        now_now=11;
        now_value=100;
        now_length=0;
    }else if(len==2&&nowCards[0]==nowCards[1]){//对子
        now_now=2;
        now_value=nowCards[0];
        now_length=0;
    }else if(len==3&&nowCards[0]==nowCards[1]&&nowCards[0]==nowCards[2]){//三
        now_now=3;
        now_value=nowCards[0];
        now_length=0;
    }else if(len==4&&nowCards[0]==nowCards[1]&&nowCards[0]==nowCards[2]&&nowCards[0]==nowCards[3]){//普通炸
        now_now=11;
        now_value=nowCards[0];
        now_length=0;
    }else if(len==4&&nowCards[0]==nowCards[1]&&nowCards[0]==nowCards[2]&&nowCards[0]!=nowCards[3]){//3 1
        now_now=4;
        now_value=nowCards[0];
        now_length=0;
    }else if(len==5&&nowCards[0]==nowCards[1]&&nowCards[0]==nowCards[2]&&nowCards[0]!=nowCards[3]&&nowCards[3]==nowCards[4]){//3 2
        now_now=5;
        now_value=nowCards[0];
        now_length=0;
    }else if(len>=5){
        int d=1;
        for(int i=0;i<len-1;i++){
            if((nowCards[i+1]-nowCards[i])!=1){
                d=0;
                break;
            }
        }
        if(d==1){//顺子
            now_now=6;
            now_value=nowCards[0];
            now_length=len;
        }else if(len%2==0){
            d=1;
            for(int i=0;i<len/2;i++){
                if(!(nowCards[2*i]==nowCards[2*i+1]&&((i==len/2-1)||((nowCards[2*(i+1)]-nowCards[2*i])==1)))){
                    d=0;
                    break;
                }
            }
            if(d==1){//连对
                now_now=7;
                now_value=nowCards[0];
                now_length=len/2;
            }else{//飞机
                int cnt=1;
                int l=0;
                for(int i=1;i<len;i++){
                    if(cnt>3){
                        l++;
                        break;
                    }
                    if(nowCards[i]==nowCards[i-1])
                        cnt++;
                    else if(cnt==3&&((nowCards[i]-nowCards[i-1])==1)){
                        l++;
                        cnt=1;
                    }else break;
                }
                if(cnt==3)
                    l++;
                if(len-l*3==0){
                    now_now=8;//1，2，3
                    now_length=l;
                    now_value=nowCards[0];
                }else if(len-l*5==0){
                    d=1;
                    for(int i=0;i<l;i++){
                        if(nowCards[3*l+2*i]!=nowCards[3*l+2*i+1]){
                            d=0;
                            break;
                        }
                    }
                    if(d==1){
                        now_now=10;//1，2，3
                        now_length=l;
                        now_value=nowCards[0];
                    }else return 0;
                }else if(len-l*4==0){
                    now_now=9;//1，2，3
                    now_length=l;
                    now_value=nowCards[0];
                }else return 0;
            }
        }else{
            int cnt=1;
            int l=0;
            for(int i=1;i<len;i++){
                if(cnt>3){
                    l++;
                    break;
                }
                if(nowCards[i]==nowCards[i-1])
                    cnt++;
                else if(cnt==3&&((nowCards[i]-nowCards[i-1])==1)){
                    l++;
                    cnt=1;
                }else break;
            }
            if(cnt==3)
                l++;
            if(len-l*3==0){
                now_now=8;//1，2，3
                now_length=l;
                now_value=nowCards[0];
            }else if(len-l*5==0){
                d=1;
                for(int i=0;i<l;i++){
                    if(nowCards[3*l+2*i]!=nowCards[3*l+2*i+1]){
                        d=0;
                        break;
                    }
                }
                if(d==1){
                    now_now=10;//1，2，3
                    now_length=l;
                    now_value=nowCards[0];
                }else return 0;
            }else if(len-l*4==0){
                now_now=9;//1，2，3
                now_length=l;
                now_value=nowCards[0];
            }else return 0;
        }
    }else{
        return 0;
    }
    //printf("%d %d %d\n",max_now,max_length,max_value);
    //printf("%d %d %d\n",now_now,now_length,now_value);
    if(max_now==0){
        now=now_now;
        length=now_length;
        value=now_value;
        return 1;
    }
    if(max_now!=now_now&&now_now!=11)
        return 0;
    if(max_now>=6&&max_now<=10){
        if(max_length!=now_length)
            return 0;
        else{
            if(now_value>max_value){
                now=now_now;
                length=now_length;
                value=now_value;
                return 1;
            }
            else return 0;
        }
    }
    if(max_now==11){
        if(now_now!=11)
            return 0;
        else{
            if(now_value>max_value){
                now=now_now;
                length=now_length;
                value=now_value;
                return 1;
            }
            else return 0;
        }
    }else{
        if(now_now==11){
            now=now_now;
            length=now_length;
            value=now_value;
            return 1;
        }else{
            if(now_value>max_value){
                now=now_now;
                length=now_length;
                value=now_value;
                return 1;
            }
            else return 0;
        }
    }
    return 0;
}
