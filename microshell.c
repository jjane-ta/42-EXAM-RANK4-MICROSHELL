#include <stdlib.h> 
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>  

int e(char *str){
    int size= 0;
    while(str[size])
        size++;
    write(2, str, size);
    return 1;
}

int m_wait(int pid)
{
    int pid2;
    int status;
    int ret;

	while((pid2 = waitpid(-1, &status, 0) )!= -1)
        if (pid == pid2)
            ret = WEXITSTATUS(status);
    return ret;
}
char **split(char **args, char *spliter)
{

    while (args && *args)
    {
        if (strcmp(*args, spliter ) == 0)
        {
            *args = NULL;
            return args + 1;
        }
        args++;
    }
    return NULL;
}
int m_fork()
{
    int pid;

    pid = fork(); 
    if(pid == -1)
        exit(e("error: fatal\n"));
    return pid;
}

int cd(char **argv)
{
    argv++;
    if (!argv || (argv + 1))
        return (e("error: cd: bad arguments\n"));
    if (chdir(*argv))
        return (e("error: cd: cannot change directory to ") && e(*argv) && e("\n"));   
    return (0); 
}
void    m_pipe(int fd[])
{
    if(pipe(fd) == -1)
        exit(e("error: fatal\n"));
    
}
void m_close(int fd[])
{
    if (fd[0] != 0)
        if (close(fd[0]))
            exit(e("error: fatal\n"));
    if (fd[1] != 1)    
        if(close(fd[1]))
            exit(e("error: fatal\n"));
}
void m_dup(int fd[])
{
    if(fd[0] != 0)
        if (dup2(fd[0], STDIN_FILENO) == -1)
            exit(e("error: fatal\n"));
    if(fd[1] != 1)
        if (dup2(fd[1], STDOUT_FILENO) == -1)
            exit(e("error: fatal\n"));
    m_close(fd);
}

void m_exec(int fd[], char **argv, char **envp)
{
    m_dup(fd);
    if (execve(argv[0], argv, envp))
        exit(e("error: cannot execute ") && e(argv[0]) && e("\n"));
}

void check_syntax(char **argv, int fd_in, int is_pipe){
    if (!argv || !(*argv) ||
     (is_pipe && !strcmp(*argv, "cd")) ||
      !strcmp(*argv, "|") ||
      !strcmp(*argv, ";"))
    {
        m_close((int []){fd_in, 1});
        exit (1);
    }
}

int pipex(char **argv, char **envp)
{
    char    **next_pipe;
    int     fd[2];
    int     fd_in = 0;
    int     pid;
    
    while((next_pipe = split(argv , "|")))
    {  
        check_syntax(argv, fd_in, 1);
        m_pipe((int *)fd);
        if (m_fork() == 0)
        {
            m_close((int []){fd[0], 1});
            m_exec((int []){fd_in, fd[1]}, argv, envp);
        }
        m_close((int []){fd_in, fd[1]});
        fd_in = fd[0];
        argv = next_pipe;
    }
    check_syntax(argv, fd_in, 0);
    if (!strcmp(*argv, "cd"))
        return cd(argv);
    if ((pid = m_fork()) == 0)
        m_exec((int []){fd_in, 1}, argv, envp);
    m_close((int []){fd_in, 1});
    return m_wait(pid);
}

int main(int argc, char **argv, char **envp)
{
    char    **next_cmd;
    int     ret = 0;

    argv++; 
    while (argc && argv && *argv)
    {  
        next_cmd = split(argv, ";");
        if (argv && *argv)
            ret = pipex(argv, envp);
        argv = next_cmd;
    }
    return (ret);
}