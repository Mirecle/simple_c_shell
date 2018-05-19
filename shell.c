/*简单的模拟shell，可以执行带一个输入重定向（或输出重定向或管道）命令，程序存在不完善之处，基本能用*/
#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<dirent.h>

char commd[100][100];
int number=0,b=0;
int test;
char command[100]; /*存放命令*/
char argv[50][50]; /*存放分解后的命令*/
int count; /*命令分解后的个数*/
/*枚举类型，依次表示一般命令，带输出重定向命令，带输入重定向命令，带管道命令*/
enum specify{NORMAL,OUT_REDIRECT,IN_REDIRECT,PIPE};

/*函数声明*/
int ttt=0;
int analysis_command();
int do_command();
int find_command(char *command);
int history();
/*主函数*/
int main()
{
    while(1){
        printf("$my_shell:");
        command[0]=0;
        gets(command);
        if(!analysis_command())
            continue;
        test=strcmp(command,"history");
        if((strcmp(command,"history")!=32)&&(command[0]!='!')){
            do_command();
            strcpy(commd[number],command);
            number++;}
    }
    return 0;
}

/*解析命令*/
int analysis_command()
{
    char *s=command;
    int i=0,j=0,state=0;
    strcat(command," ");
    /*按空格为分隔符将命令分隔开来，存放在argv中*/
    while(*s){
        switch(state){
            case 0:
                if(!isspace(*s))
                    state=1;
                else
                    s++;
                break;
            case 1:
                if(isspace(*s)){
                    argv[i][j]='\0';
                    i++;
                    j=0;
                    state=0;
                }else{
                    argv[i][j]=*s;
                    j++;
                }
                s++;
                break;
        }
    }
    count=i;
    if(count==0)
        return 0;

    if(strcmp(argv[0],"logout")==0 || strcmp(argv[0],"exit")==0)
        exit(0);
    
    if(strcmp(argv[0], "history")==0)
    {       history();
        return 1;
    }
    if (argv[0][0]=='!') {
        if (argv[0][1]=='!') {
            printf("%s",commd[number-1]);
        }
        else
        {
            for(b=1;argv[0][b]!='\0';b++)
            {
                if ((argv[0][b]-48>=0 )&& (argv[0][b]-48<=9)) {
                    ttt=ttt*10+argv[0][b]-48;
                }
                else
                    printf("Incorrect input format\n");
            
            }
            if (ttt>number) {
                printf("No such command in history!\n");
            }
            else
                printf("%s\n",commd[ttt-1]);
        
        }
                
    }
    /*判断命令是否存在*/
    else if(!find_command(argv[0])){
        puts("error:can't find command");
        return 0;
    }
    return 1;
}

/*执行命令*/
int do_command()
{
    int i,j;
    char* file;
    char* arg[50];
    char* arg2[50];
    int f=0,back_run=0;
    int fd,pid,fd2,pid2;
    enum specify type=NORMAL;
    
    for(i=0;i<count;i++){
        arg[i]=argv[i];
    }
    arg[i]=NULL;
    
    if(strcmp(arg[count-1],"<")==0 || strcmp(arg[count-1],">")==0 || strcmp(arg[count-1],"|")==0){
        printf("error:command error\n");
        return 0;
    }
    
    for(i=0;i<count;i++){//重定向啊重定向
        if(strcmp(arg[i],"<")==0){
            f++;
            file=arg[i+1];
            arg[i]=NULL;
            type=IN_REDIRECT;
        }else if(strcmp(arg[i],">")==0){
            f++;
            file=arg[i+1];
            arg[i]=NULL;
            type=OUT_REDIRECT;
        }else if(strcmp(arg[i],"|")==0){
            f++;
            type=PIPE;
            arg[i]=NULL;
            for(j=i+1;j<count;j++){//管道啊管道
                arg2[j-i-1]=arg[j];
            }
            arg2[j-i-1]=NULL;
            if(!find_command(arg2[0])){//后一条命令是非法命令
                printf("error:can't find command\n");
                return 0;
            }
        }
    }
    
    if(strcmp(arg[count-1],"&")==0){//如果有&号，说明父进程等待子进程
        back_run=1;
        arg[count-1]=NULL;
    }
    
    if(f>1){
        printf("error:don't identify the command");
        return 0;
    }
    
    pid=fork();//创建进程
    if(pid<0){
        perror("fork error");
        exit(0);
    }else if(pid==0){/*在子进程里*/
        switch(type){
            case NORMAL:
                execvp(arg[0],arg);
                break;
            case IN_REDIRECT:
                fd=open(file,O_RDONLY);
                dup2(fd,STDIN_FILENO);
                execvp(arg[0],arg);
                break;
            case OUT_REDIRECT:
                fd=open(file,O_WRONLY|O_CREAT|O_TRUNC,0666);
                dup2(fd,STDOUT_FILENO);
                execvp(arg[0],arg);
                break;
            case PIPE:
                pid2=fork();
                if(pid2==0){
                    fd2=open("tempfile",O_WRONLY|O_CREAT|O_TRUNC,0600);
                    dup2(fd2,STDOUT_FILENO);
                    execvp(arg[0],arg);
                }else{
                    waitpid(pid2,NULL,0);
                    fd=open("tempfile",O_RDONLY);//标准输入
                    dup2(fd,STDIN_FILENO);
                    execvp(arg2[0],arg2);
                }
                break;
        }
    }else{
        if(!back_run)
            waitpid(pid,NULL,0);
    }
    return 1;
}
/*返回历史命令*/
int history()
{
    if (number==0) {
        puts("no command has been used\n");
    }
    else
    {
        for(b=number;b>number-10;b--)
            if (b==0)
            break;
        
            else
                printf("%d %s\n",b,commd[b-1]);
    }
    return 0;

}

/*查找命令*/
int find_command(char *command)
{
    DIR *d;
    struct dirent *ptr;
    char temp[100];
    char *dir;
    /*获得环境变量*/
    char *path=getenv("PATH");  // /获得环境变量/
    /*分隔环境变量，且依次查找各个目录看命令是否存在*/
    strcpy(temp,path);
    dir=strtok(temp,":");
    while(dir){
        d=opendir(dir);
        while((ptr=readdir(d)) != NULL)
            if(strcmp(ptr->d_name,command) == 0){
                closedir(d);
                return 1;
            }
        closedir(d);
        dir=strtok(NULL,":");
    }
    return 0;
}