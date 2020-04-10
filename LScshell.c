#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>



int main()
{
    const char *exitstr="exit";
    const char *cdstr="cd";

    const char *pipestr="|";
    const char *oredirstr=">";
    const char *oaredirstr=">>";
    const char *iredirstr="<";
    const char *backgroundstr="&";


    char cwd[512]="";//current working dir
    char hname[512]="hostname";//default hostname
    char user[512]="user";//default user
    char cmd[512];//raw input

    pid_t bpid[20]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //background pid
    pid_t rrpid[20]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //round robin
//    int at[20];//arrival time

    char **cmdargv=NULL;//mykos kathe entolis i orismatos 63 megisto(paradoxi)
    char *cmdbuf;//voithitiko

    int n=0,i=0,loc=0,p=0,j=0;
    int ctime[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},bstatus[20]= {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},temp=0,i2=0;
    int flag=-1;

    char *specialchar="";

    void prochandler()
    {
        if(bpid[p-1]!=0 && flag==0) //to p exei auksithei gia na dwthei i epomeni timi ston pinaka bpid(background pid)
        {
            if(rrpid[j]==0)
            {
                rrpid[j]=bpid[p-1];//prosthiki diergasias sto telos ths ouras
            }
            if(rrpid[0]!=0)
            {
                if(ctime[j]==0)
                {
                    ctime[j]=time(0);
                    kill(rrpid[0],SIGCONT);
                    waitpid(rrpid[0],&bstatus[j],0);
                }

                if((time(0)-ctime[j])>2 && !WIFEXITED(bstatus[j]) ) //time quantum gia round robin 2sec
                {
                    puts("process late");
                    temp=rrpid[j];
                    rrpid[j]=rrpid[0];
                    rrpid[j-1]=temp;
                    for(i2=0; i2<(j-1); i++)
                    {
                        rrpid[i]=rrpid[i+1];
                    }
                    j=p;
                }
                else if(WIFEXITED(bstatus[j])&& j!=0)
                {
                    printf("PID %d complete\n",rrpid[0]);
                    for(i2=0; i2<(j-1); i2++)
                    {
                        rrpid[i]=rrpid[i+1];
                    }
                    j=p-1;
                    flag=0;
                }
                else if(j==0 && rrpid[0]!=0 && WIFEXITED(bstatus[j])) //mono mia diergasia
                {
                    printf("PID %d completee\n",rrpid[0]);
                    rrpid[0]=0;
                    ctime[0]=0;
                    j=0;
                    flag=-1;
                }

            }
        }
        alarm(1);
    }

    signal(SIGALRM,prochandler);
    alarm(1);

    //get local variables
    if(gethostname(hname,sizeof(hname)) != 0)
    {
        puts("Error: cannot get computer's hostname");
    }
    if(getlogin_r(user,sizeof(user)) != 0)
    {
        puts("Error: cannot get current user's name");
    };

    //mainloop
    while(1)
    {
        n=0;
        specialchar=" ";

        //get current working directory
        if(getcwd(cwd, sizeof(cwd)) == NULL)
        {
            puts("Error: cannot get current working directory");
        }

        //print shell line info
        printf("%s@%s:%s$ ",user,hname,cwd);
        fgets(cmd, 512, stdin);

        //get first word
        cmdbuf = strtok(cmd," \n");

        //pinakas me ta arguments (xwrismenes lekseis me space)
        while(cmdbuf)
        {
            cmdargv = realloc (cmdargv, sizeof (char*) * ++n);

            if (cmdargv == NULL)
            {
                puts("Error: memory allocation failed\n");
                exit (-1);
            }

            cmdargv[n-1] = cmdbuf;

            cmdbuf = strtok (NULL," \n");
        }

        //null sto telos gia execvp
        cmdargv = realloc (cmdargv, sizeof (char*) * (n+1));
        cmdargv[n] = 0;

        if(cmdargv != NULL && *cmdargv)
        {

            //eswteriki entoli exit
            if(strcmp(cmdargv[0],exitstr) == 0 )
            {
                exit(0);
            }
            //eswteriki entoli cd
            else if(strcmp(cmdargv[0],cdstr) == 0)
            {
                if(chdir(cmdargv[1]) != 0)
                {
                    puts("Error: there's no such directory");
                }
            }
            //ekswterikes entoles
            else
            {
                //anazhthsh gia eidikous xaraktires
                for (i = 0; i < n; i++)
                {
                    if(strcmp(cmdargv[i],pipestr) == 0)
                    {
                        specialchar="|";
                        loc=i;
                        break;
                    }
                    else if(strcmp(cmdargv[i],oredirstr) == 0)
                    {
                        specialchar=">";
                        loc=i;
                        break;
                    }
                    else if(strcmp(cmdargv[i],oaredirstr) == 0)
                    {
                        specialchar=">>";
                        loc=i;
                        break;
                    }
                    else if(strcmp(cmdargv[i],iredirstr) == 0)
                    {
                        specialchar="<";
                        loc=i;
                        break;
                    }
                    else if(strcmp(cmdargv[i],backgroundstr) == 0)
                    {
                        specialchar="&";
                        loc=i;
                        break;
                    }

                }

                //"apokodikopoihsh" eidikwn xaraktirwn
                if(strcmp(specialchar,pipestr)==0)
                {

                    int pipefd[2];
                    int status;
                    pid_t pid[2];

                    char **cmd1;
                    cmd1=malloc(sizeof(char*) * (loc+1));
                    char **cmd2;
                    cmd2=malloc(sizeof(char*) * (n-loc));

                    for(i=0; i<=loc; i++)
                    {
                        cmd1[i]=malloc(strlen(cmdargv[i]) * sizeof(char));
                        cmd1[i]=cmdargv[i];
                    }


                    for(i=0; i<(n-loc-1); i++)
                    {
                        cmd2[i]=malloc(strlen(cmdargv[loc+i+1]) * sizeof(char));
                        cmd2[i]=cmdargv[loc+i+1];
                    }

                    cmd2[n-loc-1]=0;
                    cmd1[loc] = 0;

                    pipe(pipefd);

                    pid[0]=fork();

                    if(!pid[0])
                    {
                        close(pipefd[0]);
                        dup2(pipefd[1],1);
                        close(pipefd[1]);

                        execvp(cmd1[0],cmd1);
                        perror(cmd1[0]);
                        _exit(1);
                    }
                    if(pid[0])
                    {
                        pid[1]=fork();
                        if(!pid[1])
                        {
                            close(pipefd[1]);
                            dup2(pipefd[0],0);
                            close(pipefd[0]);

                            execvp(cmd2[0],cmd2);
                            perror(cmd2[0]);
                            _exit(1);
                        }
                    }
                    close(pipefd[0]);
                    close(pipefd[1]);
                    waitpid(pid[1],&status,0);

                }
                else if(strcmp(specialchar,oredirstr)==0)
                {
                    int out;
                    int status;
                    pid_t pid;

                    char **cmd1;
                    cmd1=malloc(sizeof(char*) * (loc+1));
                    char *outfile;
                    outfile=malloc(strlen(cmdargv[loc+1]) * sizeof(char));

                    outfile=cmdargv[loc+1];

                    for(i=0; i<=loc; i++)
                    {
                        cmd1[i]=malloc(strlen(cmdargv[i]) * sizeof(char));
                        cmd1[i]=cmdargv[i];
                    }
                    cmd1[loc]=0;

                    pid=fork();

                    if(pid==0)
                    {
                        out=creat(outfile, 0644);
                        dup2(out,STDOUT_FILENO);
                        close(out);
                        execvp(cmd1[0],cmd1);

                        perror(cmd1[0]);
                        _exit(1);
                    }
                    else
                    {
                        while (wait(&status) != pid);
                    }


                }
                else if(strcmp(specialchar,oaredirstr)==0)
                {
                    int out;
                    int status;
                    pid_t pid;

                    char **cmd1;
                    cmd1=malloc(sizeof(char*) * (loc+1));
                    char *outfile;
                    outfile=malloc(strlen(cmdargv[loc+1]) * sizeof(char));

                    outfile=cmdargv[loc+1];

                    for(i=0; i<=loc; i++)
                    {
                        cmd1[i]=malloc(strlen(cmdargv[i]) * sizeof(char));
                        cmd1[i]=cmdargv[i];
                    }
                    cmd1[loc]=0;

                    pid=fork();

                    if(pid==0)
                    {
                        out=open(outfile,O_RDWR|O_CREAT|O_APPEND, 0644);
                        dup2(out,STDOUT_FILENO);
                        close(out);
                        execvp(cmd1[0],cmd1);

                        perror(cmd1[0]);
                        _exit(1);
                    }
                    else
                    {
                        while (wait(&status) != pid);
                    }
                }
                else if(strcmp(specialchar,iredirstr)==0)
                {
                    int in;
                    int status;
                    pid_t pid;

                    char **cmd1;
                    cmd1=malloc(sizeof(char*) * (loc+1));
                    char *infile;
                    infile=malloc(strlen(cmdargv[loc+1]) * sizeof(char));

                    infile=cmdargv[loc+1];

                    for(i=0; i<=loc; i++)
                    {
                        cmd1[i]=malloc(strlen(cmdargv[i]) * sizeof(char));
                        cmd1[i]=cmdargv[i];
                    }
                    cmd1[loc]=0;

                    pid=fork();

                    if(pid==0)
                    {
                        in=open(infile,O_RDONLY,0);
                        dup2(in,STDIN_FILENO);
                        close(in);
                        execvp(cmd1[0],cmd1);

                        perror(cmd1[0]);
                        _exit(1);
                    }
                    else
                    {
                        while (wait(&status) != pid);
                    }
                }
                else if(strcmp(specialchar,backgroundstr)==0)
                {
                    pid_t pid;
                    char **cmd1;
                    cmd1=malloc(sizeof(char*) * (loc+1));

                    for(i=0; i<=loc; i++)
                    {
                        cmd1[i]=malloc(strlen(cmdargv[i]) * sizeof(char));
                        cmd1[i]=cmdargv[i];
                    }
                    cmd1[loc]=0;

                    pid=fork();

                    if(pid==0)
                    {
                        execvp(cmd1[0],cmd1);
                        perror(cmd1[0]);
                        _exit(1);
                    }
                    else if(pid>0)
                    {
                        kill(pid,SIGSTOP);
                        bpid[p]=pid;
                        flag=0;
                        p++;
                    }
                }
                else
                {
                    pid_t pid;
                    int status;
                    pid=fork();
                    if(pid==0)
                    {
                        execvp(cmdargv[0],cmdargv);
                        perror(cmdargv[0]);
                        _exit(1);
                    }
                    else
                    {
                        while (wait(&status) != pid);
                    }
                }

            }

        }

    }


    free(cmdargv);
    return 0;
}

