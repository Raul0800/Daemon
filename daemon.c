#include <syslog.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define FiveMB  5 * 1024 * 1024

void createDir(char *nameDir){
    chdir(getenv("HOME"));
    if (mkdir(nameDir, S_IRWXU | S_IRWXO | S_IRWXG)){
        syslog(LOG_INFO, "Dir is already create");    
    }
}

FILE* openFile(char *nameFile, char* nameDir){
    char fullPath[512];
    snprintf(fullPath, sizeof(fullPath), "%s%s", "HOME/", nameDir);
    chdir(getenv(fullPath));
    FILE* fileStream;

    if (( fileStream = fopen(nameFile, "ab+")) &&
        fileStream == NULL){
            syslog(LOG_INFO, "ERROR: Failed to create a file (# %d)", errno);
            exit(1); 
    }
    return fileStream;
}

void closeFile(FILE* fileStream){
    if (fileStream != NULL) fclose(fileStream);
}

int getSizeFile(char *nameFile, char* nameDir){
    FILE* fileStream = openFile(nameFile, nameDir);

    int size = 0;
    
    int currPos = ftell( fileStream );
    fseek(fileStream, 0, SEEK_END);
    
    size = ftell( fileStream ); 
    
    fseek( fileStream, currPos, SEEK_SET );
    
    closeFile(fileStream);
    
    return size;
}

void myRand(int *buf, int sizeBuf, char *nameFile, char* nameDir){
    int i = 0;
    time_t rawtime; 
    int clock1;
    int* rand_buf = malloc(sizeBuf*sizeof(int));
    int shift;
    int j = 0;
    for (i = 0; i < sizeBuf; i++){
	clock1 = clock();	
	time ( &rawtime );
	struct sysinfo info;
  	sysinfo(&info);	
	for(j = 0; j < rand() % 1000; j++)	
		buf[i] +=(int) ((rand() ^ clock1) * (info.bufferram ^ rand()) * (info.bufferram ^ info.procs));    	
    }
    free(rand_buf);
}

void writeToFile(char *nameFile, char* nameDir){
    
    int buf1[256];
    int buf2[256];
    int i = 0;
    while (getSizeFile(nameFile, nameDir) < FiveMB){
	FILE* fileStream = openFile(nameFile, nameDir);        
	myRand(buf1, 256, nameFile, nameDir);	

	fwrite(buf1, sizeof(int), 256, fileStream);

 	closeFile(fileStream);      
    }   
}

pid_t getPid(char *nameFile, char* nameDir){
    FILE* fileStream = openFile(nameFile, nameDir);

    pid_t pid = -1;
    fscanf(fileStream, "%d", &pid);

    closeFile(fileStream);
    return pid;
}

void sig_handler(int signo)
{
  if(signo == SIGTERM || signo == SIGINT)
  {
    syslog(LOG_INFO, "SIGTERM has been caught! Exiting...");
    if(remove("run/daemon.pid") != 0)
    {
      syslog(LOG_ERR, "Failed to remove the pid file. Error number is %d", errno);
      exit(1);
    }
    exit(0);
  }
}

void handle_signals()
{
  if((signal(SIGTERM, sig_handler) == SIG_ERR) ||
        (signal(SIGINT, sig_handler) == SIG_ERR))
  {
    syslog(LOG_ERR, "Error! Can't catch SIGTERM or SIGINT");
    exit(1);
  }
}

void daemonise()
{
  pid_t pid, sid;
  FILE *pid_fp;

  syslog(LOG_INFO, "Starting daemonisation.");

  //First fork
  pid = fork();
  if(pid < 0)
  {
    syslog(LOG_ERR, "Error occured in the first fork while daemonising. Error number is %d", errno);
    exit(1);
  }

  if(pid > 0)
  {
    syslog(LOG_INFO, "First fork successful (Parent)");
    exit(0);
  }
  syslog(LOG_INFO, "First fork successful (Child)");

  //Create a new session
  sid = setsid();
  if(sid < 0) 
  {
    syslog(LOG_ERR, "Error occured in making a new session while daemonising. Error number is %d", errno);
    exit(1);
  }
  syslog(LOG_INFO, "New session was created successfuly!");

  //Second fork
  pid = fork();
  if(pid < 0)
  {
    syslog(LOG_ERR, "Error occured in the second fork while daemonising. Error number is %d", errno);
    exit(1);
  }

  if(pid > 0)
  {
    syslog(LOG_INFO, "Second fork successful (Parent)");
    exit(0);
  }
  syslog(LOG_INFO, "Second fork successful (Child)");

  pid = getpid();

  //Change working directory to Home directory
  if(chdir(getenv("HOME")) == -1)
  {
    syslog(LOG_ERR, "Failed to change working directory while daemonising. Error number is %d", errno);
    exit(1);
  }

  //Grant all permisions for all files and directories created by the daemon
  umask(0);

  //Redirect std IO
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  if(open("/dev/null",O_RDONLY) == -1)
  {
    syslog(LOG_ERR, "Failed to reopen stdin while daemonising. Error number is %d", errno);
    exit(1);
  }
  if(open("/dev/null",O_WRONLY) == -1)
  {
    syslog(LOG_ERR, "Failed to reopen stdout while daemonising. Error number is %d", errno);
    exit(1);
  }
  if(open("/dev/null",O_RDWR) == -1)
  {
    syslog(LOG_ERR, "Failed to reopen stderr while daemonising. Error number is %d", errno);
    exit(1);
  }

  chdir(getenv("HOME"));
  pid_fp = fopen("run/daemon.pid", "w");
  if(pid_fp == NULL)
  {
    syslog(LOG_ERR, "Failed to create a pid file while daemonising. Error number is %d", errno);
    exit(1);
  }
  if(fprintf(pid_fp, "%d\n", pid) < 0)
  {
    syslog(LOG_ERR, "Failed to write pid to pid file while daemonising. Error number is %d, trying to remove file", errno);
    fclose(pid_fp);
    if(remove("run/daemon.pid") != 0)
    {
      syslog(LOG_ERR, "Failed to remove pid file. Error number is %d", errno);
    }
    exit(1);
  }
  fclose(pid_fp);
}

void main( int argc, char** argv )
{
    char * start = "start";
    char * stop = "stop";

    char * nameDirData = "random/";
    char * nameDirPid = "run/";
    char * nameFileData = "random/data";
    char * nameFilePid = "run/daemon.pid";

    FILE* filePid;
    FILE* fileData;

    if(argc != 2){
        syslog( LOG_INFO, "ERROR: arg != 2" );
        exit(1);
    }
    
    createDir(nameDirPid);
    closeFile(openFile(nameFilePid, nameDirPid));

    createDir(nameDirData);
    closeFile(openFile(nameFileData, nameDirData));
        
    if(!strcmp(argv[1], start)){
        if(getSizeFile(nameFilePid, nameDirPid)){
            syslog( LOG_INFO, "WARNING: Daemon still running");
            exit(0);
        }

        syslog( LOG_INFO, "Work: Daemon started");
        daemonise();
        handle_signals();
        
        while(1){
            sleep(3);
            writeToFile(nameFileData, nameDirData);
        }
    }
    if(!strcmp(argv[1], stop)){
        if(getSizeFile(nameFilePid, nameDirPid) < 1){
            syslog( LOG_INFO, "WARNING: Daemon is already stop");
            exit(0);
        }

        syslog( LOG_INFO, "Work: Daemon stopped");
        
        kill(getPid(nameFilePid, nameDirPid), SIGTERM);
        exit(0);        
    }
}
