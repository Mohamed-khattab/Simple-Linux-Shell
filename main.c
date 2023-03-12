#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
///////////////////// functions prototype ///////////////////////////////////////
char *read_input(void) ;
void print_prompt1(void ) ;
void print_prompt2(void ) ;
void welcomeScreen(void) ;
//void log_handle(int sig);			// signal handler to add log statements
char** convert_command(char *cmd_command) ;
int shell();
void setup_environment();
void execute_builtin_commands(char* command,char **argv) ;
void execute_external_command( char **argv);
void on_child_exit();

void execute_mkdir(char **argv);
FILE* logfile;
int pids[100];
int pidCount =0 ;
bool reaped[100];
//Updates log whenever a child terminates
void write_to_log_file(int pid){
    fprintf(logfile, "Child process [%d] has terminated\n", pid);
    fflush(logfile);
}
int main(){
    // tie the handler to the SGNCHLD signal
    signal(SIGCHLD,&on_child_exit);
     setup_environment();
    shell();
    return 0;
}

int shell(){
    char *cmd_command;
    welcomeScreen() ;
    do {
        print_prompt1();
        cmd_command = read_input();
        if (!cmd_command) {   // no input
            exit(EXIT_SUCCESS);
        }
        if (cmd_command[0] == '\0' || strcmp(cmd_command, "\n") == 0) { // empty line
            free(cmd_command);
            continue;
        }
        if (strlen(cmd_command) > 0 && cmd_command[strlen(cmd_command) -1] == '\n') {
            cmd_command[strlen(cmd_command) -1] = '\0'; // Replace \n with \0
        }
        if (strcmp(cmd_command, "exit") == 0) // exit command
        {
            free(cmd_command);
            break;
        }
        char** argv = convert_command(cmd_command);
        if(strcmp(argv[0], "cd") == 0
           || strcmp(argv[0], "echo") == 0
           || strcmp(argv[0], "export") == 0
           || strcmp(argv[0], "ls") == 0
           || strcmp(argv[0], "rm") == 0
           || strcmp(argv[0], "rmdir") == 0
           || strcmp(argv[0], "pwd") == 0
           || strcmp(argv[0], "mkdir") == 0
           || argv[0][0] =='$'
           )
            execute_builtin_commands(argv[0], argv);
        else
            execute_external_command( argv);
//        printf("%s\n", cmd_command);
        free(cmd_command);
    } while (1);
    exit(EXIT_SUCCESS);
}

//Reaps zombie processes
void on_child_exit(){
    for(int i=0; i< pidCount; i++){
        if( reaped[i])
            continue;
        if(waitpid(pids[i], NULL, WNOHANG) != 0)
            reaped[i] = true, write_to_log_file(pids[i]);
    }
}
//////////////////////////set and get in environment variables ////////////////////////////////////
void setup_environment(){
    char* directory = (char*) malloc(100);
    char* logfileDirectory = (char*) malloc(1010 * sizeof(char));
    getcwd(directory, 100);
    chdir(directory);
    strcpy(logfileDirectory, directory);
    strcat(logfileDirectory, "/logs.txt");
    logfile = fopen(logfileDirectory, "w+");
    free(directory);
    free(logfileDirectory);
}
//////////////////// execute the external commands ////////////////////////////////////
void execute_cd(char **argv) {
    if (argv[1] ==NULL) {
        const char *home_dir = getenv("HOME");
        if (home_dir == NULL) {
            fprintf(stderr, "cd error: $HOME is not set\n");
        }
        if (chdir(home_dir) == -1) {
            perror("cd error");
        }
    }else if(strcmp(argv[1] , "~") ==0 )
        argv[1] ="/";

    if (chdir(argv[1]) == -1) {
        perror("cd error");
    }
}
void execute_echo(char **argv) {
    if(argv[1] != NULL){
        int i =1 ;
        printf("\t");
        while(argv[i] != NULL){
            printf("\033[1;36;1m %s\033[0m",argv[i] ) ;
            printf(" ") ;
            i++;
        }
    }
    printf("\n");
}
void execute_export(char **argv) {
    if (argv[1] == NULL) {
        printf("No arguments provided.\n");
        return;
    }
    char *arg = argv[1];
    char *variable = strtok(arg, "=");
    char *value = strtok(NULL, "=");

    if (variable == NULL || value == NULL) {
        printf("Invalid command: %s\n", argv[1]);
        return;
    }
    if (value[0] == '"')
        value++;

    if(value[strlen(value) - 1] == '"')
        value[strlen(value) - 1] = '\0';

    int i =2 ;
    while(argv[i] != NULL ){
        strcat(value , argv[i]) ;
        i++ ;
    }
    // Remove leading and trailing quotes from the value
    if (setenv(variable, value, 1) == -1) {
        printf("Error setting environment variable: %s\n", variable);
    }
}
char *get_exported_variable(char *variable) {
    // Check if both the variable and value are provided
    if (variable == NULL ) {
        printf("Invalid variable : %s\n",variable);
        return NULL;
    } else {
        return getenv(variable) ;
    }
}
void execute_$(char * command) {
    char *variable = command +1 ;
    char *value = get_exported_variable(variable) ;
    printf("\033[1;32;1m \t %s \033[0m", value) ;
}
void execute_ls( char ** argv){
     int i =1 ;
    int show_hidden = 0;
    int show_details = 0;
    DIR *dir;
    struct dirent *ent;
    while(argv[i] != NULL){
        int flag = 0;
        if (strstr(argv[i], "-a") != NULL){
            show_hidden=1 ;
            flag =1 ;
        }
        if (strstr(argv[i], "-l") != NULL){
            show_details=1 ;
            flag =1 ;
        }
        if ((strstr(argv[i], "-la") != NULL) || (strstr(argv[i], "-al") != NULL)){
            show_hidden=1 ;
            show_details=1 ;
            flag =1  ;
        }
        if( !flag) {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
        i++ ;
    }
    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (show_hidden || ent->d_name[0] != '.') {
                if (show_details) {
                    printf("\033[1;36;1m \t %s\t%d\t \033[0m",ent->d_name, ent->d_type);
                } else {
                    printf("\033[1;36;1m \t %s\t \033[0m", ent->d_name);
                }
            }
        }
        closedir(dir);
        printf("\n") ;
    } else {
        perror("Unable to open directory");
        exit(EXIT_FAILURE);
    }
}
void execute_rm(char **argv) {
//    printf("hello from rm ") ;
    if (argv[1] == NULL) {
        printf("NO arguments \n");
    }else {
        if (unlink(argv[1]) == -1) {
            perror("unlink() error");
        }
    }
}
void execute_pwd(){
    //    printf("hello from pwd ") ;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("\033[1;36;1m \t %s\n \033[0m", cwd);
    } else {
        perror("getcwdu() error");
    }
}
void execute_mkdir(char **argv) {
    char* dirname ;
    if( argv[1] == NULL)
        dirname = "newDir";
    else
        dirname = argv[1] ;
    int result = mkdir(dirname, 0777); // make new directory with all privileges
    if (result != 0) {
        printf("Error creating directory %s\n", dirname);
    }
    printf("Directory created successfully with name : %s \n", dirname);
}
void execute_builtin_commands(char* command,char **argv) {
    int i =1 ;
    while(argv[i] != NULL){
        if(argv[i][0] =='$'){
            argv[i] = get_exported_variable(argv[i]+1) ;
        }
        i++ ;
    }
    if(strcmp(command, "cd") == 0){
        execute_cd(argv);
    }else if(strcmp(command, "echo") == 0){
        execute_echo(argv);
    }else if(strcmp(command, "export") == 0){
        execute_export(argv);
    }else if(strcmp(command, "ls") == 0){
         execute_ls(argv);
    }else if(strcmp(command, "rm") == 0
          || strcmp(command , "rmdir")==0){
         execute_rm(argv);
    }else if(strcmp(command, "pwd") == 0) {
        execute_pwd();
    } else if(strcmp(command, "mkdir") ==0){
        execute_mkdir(argv);
    }else if( command[0] == '$'){
        execute_$(command) ;
    }else{
        printf("Unsupported Command\n");
    }
    fprintf(logfile, "%s command hs been used \n",command );
    fflush(logfile);
}

//////////////////// execute the external commands ///////////////////////////////////
void execute_external_command(char **argv) {
     int background = 0;
     int i = 0;
     while (argv[i] != NULL) {
         if (strcmp(argv[i], "&") == 0) {
             background = 1;
             argv[i] = NULL;
             break;
         }
         i++;
     }
     pid_t pid = fork();
     if (pid == 0) {   // child
         /* in the child process, we redirect the standard input, output,
          * and error streams to /dev/null if the command is run in the background.
          * This prevents the output of the command from interfering with other commands entered by the user.
          * */
         if (background) {  // to prevent the parent to be blocked
             int null_fd = open("/dev/null", O_RDWR);
             dup2(null_fd, STDIN_FILENO);
             dup2(null_fd, STDOUT_FILENO);
             dup2(null_fd, STDERR_FILENO);
             close(null_fd);
         }
         // Run external command in the child process
         execvp(argv[0], argv);
         perror("execvp failed");
         exit(EXIT_FAILURE);

     } else if (pid > 0) {  // parent
         pids[pidCount++] = pid;
         if (!background) {
             int status;
             waitpid(pid, &status, 0); // Wait for child process to finish
             reaped[pidCount - 1] = true;
         } else {
             printf("Running command in background with PID: %d\n", pid);
         }
     } else {
         perror("fork failed");
         exit(EXIT_FAILURE);
     }
 }

char** convert_command(char *cmd_command) {
    char** argv;
    // Allocate memory for argv
    argv = malloc(sizeof(char*) * strlen(cmd_command));
    char *ptr;
    int  i = 0;
    ptr = strtok( cmd_command, " ");
    while(ptr != NULL){
        if(ptr[0] =='"')
            ptr++ ;
        if(ptr[strlen(ptr)-1 ]=='"')
            ptr[strlen(ptr)-1 ] ='\0' ;
        argv[i] = ptr;
        i++;
        ptr = strtok(NULL, " ");
    }
    return argv;
}
void welcomeScreen(){
    printf("\n\033[1;33;1m \t\t\t=================================================================================\n\033[0m");
    printf("\n\033[1;32;1m \t\t\t\t\t\t\t\t\t\t\t Simple C Shell\n\033[0m");
    printf("\n\033[1;33;1m \t\t\t===================================================================================\n\033[0m");
    printf("\n\n");
}
void print_prompt1(void ) {
    printf("\n\033[1;33;1m $> \033[0m");
}
void print_prompt2(void){
    fprintf(stderr , ">> ") ;
}
char *read_input(void) {
    char buf[1024]; // size of chunk
    char *ptr = NULL ;
    char ptrlen= 0 ;
    while(fgets(buf , 1024 , stdin )){
        int bufflen = strlen(buf) ;
        if(!ptr) {
            ptr = malloc(bufflen + 1);
        } else{
            char *ptr2 = realloc(ptr , ptrlen+bufflen+1) ;
            if(ptr2){
                ptr = ptr2 ;
            }else
            {
                free(ptr);
                ptr= NULL ;
            }
        }
        if(!ptr){
            fprintf(stderr , "error : failed to allocate memory %s\\n ", strerror(errno));
            return NULL ;
        }
        strcpy(ptr+ptrlen , buf) ;
        if(buf[bufflen-1] == '\n') // if your input has multiple lines
        {
            if(bufflen == 1 || buf[bufflen-2] != '\\')
            {
                return ptr;
            }
            ptr[ptrlen+bufflen-2] = '\0';
            bufflen -= 2;
            print_prompt2();
        }
        ptrlen += bufflen;
    }
    return ptr;
}
