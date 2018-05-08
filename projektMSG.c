#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <mqueue.h>
void readinput(char* BUF, int *e);
void encipher(char* BUF);
void print_res(char* BUF, int *e);
void sig_stop(int signum);
void sig_cont(int signum);
void sig_kill(int signum);
void sig_msg(int signum);
void sig_rcv(int signum);
mqd_t mq;
int x=1;
sem_t *sem1;
sem_t *sem2;
sem_t *sem3;
void* shm_ad;
int main(void)
{
struct mq_attr attr;
attr.mq_flags=0;
attr.mq_maxmsg=8;
attr.mq_msgsize=sizeof(char);
attr.mq_curmsgs=0;
int shmid;
int e=1;
char *BUF;
if((mq = mq_open("/mqueue" , O_CREAT | O_RDWR , S_IRWXG, &attr))<0)
printf("%d\n",errno);
if ((shmid = shmget(ftok("/tmp",1), 100, IPC_CREAT | 0666)) < 0) 
{
perror("shmget");
exit(1);
}
shm_ad=shmat(shmid,NULL,0);
if ((BUF=(char*)shm_ad)==NULL) 
{
perror("shmat");
exit(1);
}
sem1 = sem_open ("sharedSem1", O_CREAT | O_EXCL, 0666, 0);
sem2 = sem_open ("sharedSem2", O_CREAT | O_EXCL, 0666, 0);
sem3 = sem_open ("sharedSem3", O_CREAT | O_EXCL, 0666, 0);
sem_unlink("sharedSem1");
sem_unlink("sharedSem2");
sem_unlink("sharedSem3");
if(fork()==0)
{
printf("Proces %d %d utworzony.\n", getpid(), getppid());
while(1)
{
if(x)
{
readinput(BUF,&e);
sem_post(sem2);
sem_wait(sem1);
}
signal(SIGFPE, sig_stop);
signal(SIGCONT, sig_cont);
signal(SIGUSR2, sig_kill);
signal(SIGUSR1, sig_rcv);
}
}
if(fork()==0)
{
printf("Proces %d %d utworzony.\n", getpid(), getppid());
while(1)
{
if(x)
{
sem_wait(sem2);
encipher(BUF);
sem_post(sem3);
}
signal(SIGFPE, sig_stop);
signal(SIGCONT, sig_cont);
signal(SIGUSR2, sig_kill);
signal(SIGUSR1, sig_rcv);
}
}
if(fork()==0)
{
printf("Proces %d %d utworzony.\n", getpid(), getppid());
while(1)
{
if(x)
{
sem_wait(sem3);
print_res(BUF,&e);
sem_post(sem1);
}
signal(SIGFPE, sig_stop);
signal(SIGCONT, sig_cont);
signal(SIGUSR2, sig_kill);
signal(SIGUSR1, sig_rcv);
}
}
while(1)
{
signal(SIGUSR1,sig_msg);
}
return 0;
}
void readinput(char* BUF, int *e)
{
if(*e)
printf("\nPodaj tekst do zaszyfrowania: \n");
fgets(BUF,sizeof(char)*100,stdin);
if(BUF[strlen(BUF)-1]!='\n')
*e=0;
else
*e=1;
}
void encipher(char* BUF)
{
int i;
int key=5;
char* etxt=malloc(sizeof(char)*100);
for(i=0;i<strlen(BUF);i++)
{
if((BUF[i]>='A') && (BUF[i]<='Z'))
{
etxt[i]=BUF[i]+key;
if(etxt[i]>'Z')
etxt[i]=(etxt[i]%90)+64;
}
else
{
if((BUF[i]>='a') && (BUF[i]<='z'))
{
etxt[i]=BUF[i]+key;
if(etxt[i]>'z')
etxt[i]=(etxt[i]%122)+96;
}
else
etxt[i]=BUF[i];
}
}
memcpy(BUF,etxt,sizeof(char)*strlen(BUF));
}
void print_res(char* BUF, int *e)
{
if(*e)
printf("\nzaszyfrowany tekst:\n");
printf(BUF);
if(BUF[strlen(BUF)-1]!='\n')
*e=0;
else
*e=1;
}
void sig_stop(int signum) //stop
{
char msg='1';
kill(getppid(),SIGUSR1);
mq_send(mq,&msg,sizeof(msg),10);
}
void sig_cont(int signum) //cont
{
char msg='2';
kill(getppid(),SIGUSR1);
mq_send(mq,&msg,sizeof(msg),10);
}
void sig_kill(int signum) //kill
{
char msg='3';
kill(getppid(),SIGUSR1);
mq_send(mq,&msg,sizeof(msg),10);
}
void sig_msg(int signum)
{
char msg;
mq_receive(mq,&msg,sizeof(msg),NULL);
if(msg=='1')
{
msg='4';
mq_send(mq,&msg,sizeof(msg),10);
mq_send(mq,&msg,sizeof(msg),10);
mq_send(mq,&msg,sizeof(msg),10);
mq_send(mq,&msg,sizeof(msg),10);
killpg(getpid(),SIGUSR1);
}
if(msg=='2')
{
msg='5';
mq_send(mq,&msg,sizeof(msg),10);
mq_send(mq,&msg,sizeof(msg),10);
mq_send(mq,&msg,sizeof(msg),10);
mq_send(mq,&msg,sizeof(msg),10);
killpg(getpid(),SIGUSR1);
}
if(msg=='3')
{
msg='6';
mq_send(mq,&msg,sizeof(msg),10);
mq_send(mq,&msg,sizeof(msg),10);
mq_send(mq,&msg,sizeof(msg),10);
mq_send(mq,&msg,sizeof(msg),10);
killpg(getpid(),SIGUSR1);
}
if(msg=='6')
{
mq_close(mq);
mq_unlink("
/mqueue
");
shmdt(shm_ad);
sem_close(sem1);
sem_close(sem2);
sem_close(sem3);
raise(SIGKILL);
}
}
void sig_rcv(int signum)
{
char msg;
mq_receive(mq,&msg,sizeof(msg),NULL);
if(msg=='4')
{
printf("zatrzymywanie\n");
x=0;
}
if(msg=='5')
{
printf("wznawianie\n");
x=1;
}
if(msg=='6')
{
printf("konczenie\n");
mq_close(mq);
raise(SIGKILL);
}
}