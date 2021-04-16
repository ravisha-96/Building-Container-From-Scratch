#define _GNU_SOURCE
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <error.h>

#define STACK_SIZE (1024 * 1024)    /* Stack size for cloned child */

/* A simple error-handling function: print an error message based
   on the value in 'errno' and terminate the calling process */

#define errExit(msg) {perror(msg); exit(EXIT_FAILURE);}

void mount_root_file_system(const char *file_system)
{
    mount(file_system,file_system,"ext4",MS_BIND,"");
    chdir(file_system);
    const char *old_fs = ".old_fs";
    mkdir(old_fs,0777);
    pivot_root(".",old_fs);
    chdir("/");
}

void mount_proc_file_system()
{
    const char *mount_point = "/proc";
    if (mount_point != NULL) 
    {
        if (mount("proc", mount_point, "proc", 0, NULL) == -1)
            errExit("mount");
        printf("Mounting procfs at %s\n", mount_point);
    }
}

void uts_namespace(const char *child_host_name)
{
    struct utsname uts;
    /* Change hostname in UTS namespace of child */
    if (sethostname(child_host_name, strlen(child_host_name)) == -1)
        errExit("sethostname");

    /* Retrieve and display hostname */
    if (uname(&uts) == -1)
        errExit("uname");
    printf("nodename in child uts namespace: %s\n", uts.nodename);
}

int namespace_handler(void *args)
{
    printf("NAMESPACE HANDLER\n");
    char **argv = (char **)args;

    const char *root_file_system = argv[1];
    mount_root_file_system(root_file_system);

    mount_proc_file_system();
    const char *child_hostname = argv[2];
    uts_namespace(child_hostname);

    execlp("bin/bash","bin/bash",NULL);
    return 0;
}

int main(int argc, char *argv[])
{
    static char child_stack[STACK_SIZE];
    pid_t child_pid;
    struct utsname uts;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <child-hostname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Create a child that has its own mount,pid,uts namespace;
       the child commences execution in namespace_handler() */

    printf("CREATING NEW NAMESPACE\n");
    child_pid = clone(namespace_handler,
                        child_stack + STACK_SIZE,   /* Points to start of downwardly growing stack */ 
                        CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD,
                        argv);
    if (child_pid == -1)
    {
        errExit("clone");
    }
    printf("PID of child created by clone() is %ld\n", (long) child_pid);

    /* Display the hostname in parent's UTS namespace. This will be 
       different from the hostname in child's UTS namespace. */

    if (uname(&uts) == -1)
        errExit("uname");    
    printf("nodename in parent uts namespace: %s\n", uts.nodename);

    if (waitpid(child_pid, NULL, 0) == -1)      /* Wait for child */
        errExit("waitpid");
    printf("END\n");
    exit(EXIT_SUCCESS);

    return 0;
}

// #include <sched.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <sys/utsname.h>
// #include <sys/wait.h>
// #include <unistd.h>
// #include <sys/mount.h>

// static char child_stack[1048576];

// struct clone_args{
// 	const char * rootPath;
// };

// static int child_fn(void * clone_args) {
//   printf("New `net` Namespace:\n");
//   struct clone_args* args = (struct clone_args *)clone_args;
//   mount("../rootfs","./", "ext4", MS_BIND, 0);
//   execlp("/bin/bash", "bin/bash/", NULL);
//   printf("%s\n", args->rootPath );
//   return 0;
// }

// int main() {
//   printf("Original `net` Namespace:\n");
//   // system("ip link");
//   // printf("\n\n");
//   const char *path = "/home/rshankar/CS695/assignment3/rootfs";

//   struct clone_args *args;
//   args->rootPath = path;
 
//   void * arg = (void *)args;
//   printf("%s\n", "came here");
//   pid_t child_pid = clone(child_fn, child_stack+1048576, CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, arg);
//   printf("%s\n", "terminated");
//   sleep(10);
//   waitpid(child_pid, NULL, 0);
//   return 0;
// }
