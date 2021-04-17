#define _GNU_SOURCE
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
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

void set_hostname_uts_namespace(const char *child_host_name)
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

void assign_ip_and_bring_up_veth(const char *veth){
	//command -> ip addr add 10.1.1.1/24 dev eth0
	//command -> ip link set dev veth up
	char ip[100];
	printf("Enter the ip for %s :", veth);
	scanf("%s", ip);
	printf("assigning ip and bringing up interface : %s",veth);

	char buffer[1024];
	sprintf(buffer, "ip addr add %s/24 dev %s", ip, veth);
	system(buffer);
	printf("%s\n",buffer);

	sprintf(buffer, "ip link set dev %s up ", veth);
	printf("%s\n",buffer);
	system(buffer);

}

void configure_network_namespace_in_parent(char *argv[]){
	//create a veth pair
	//command -> ip link add veth0 type veth peer name veth1
	char * veth0 = argv[3];
	char * veth1 = argv[4];
	char * new_nw_namespace = argv[5];
	char buffer[1024];
	sprintf(buffer,"ip link add %s type veth peer name %s", veth0, veth1);
	printf("%s\n",buffer);
	system(buffer);


	//Move the veth1 end to the new network namspace
	//command -> ip  link set veth1 netns coke
	sprintf(buffer, "ip link set %s netns %s", veth1, new_nw_namespace);
	printf("%s\n",buffer);
	system(buffer);

	//Provide ip addresses and bring the interfaces up
	//For parent namespace
	assign_ip_and_bring_up_veth(veth0);
	//For new network namespace
}

void configure_network_namespace_in_child(const char *new_nw_namespace)
{

    // printf("path:%s\n",netspace_name);
    char netns_path[64] = "/var/run/netns/";
    strcat(netns_path,new_nw_namespace);
    int fd = open(netns_path,O_RDONLY | O_CLOEXEC);
    if(fd == -1)
        errExit("open");
    int ret_no = setns(fd,0);
    // printf("%d\n",temp);
    if(ret_no == -1)
        errExit("setns");
    system("ip link set dev lo up");
}

int child_process(void *args)
{
    printf("NAMESPACE HANDLER\n");
    char **argv = (char **)args;

    const char *root_file_system = argv[1];
    mount_root_file_system(root_file_system);

    mount_proc_file_system();

    const char *child_hostname = argv[2];
    set_hostname_uts_namespace(child_hostname);

    const char *child_veth1 = argv[4];
    const char *new_nw_namespace = argv[5];

    configure_network_namespace_in_child(new_nw_namespace);
    assign_ip_and_bring_up_veth(child_veth1);
    execlp("bin/bash","bin/bash",NULL);
    
    return 0;
}

int main(int argc, char *argv[])
{
    char child_stack[STACK_SIZE];
    pid_t child_pid;
    struct utsname uts;

    if (argc < 6) {
        fprintf(stderr, "Usage: %s <rootfs> <child-hostname> <veth0> <veth1> <new nw_namespace> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    configure_network_namespace_in_parent(argv);
    /* Create a child that has its own mount,pid,uts namespace;
       the child commences execution in child_process() */

    printf("CREATING NEW NAMESPACE\n");
    child_pid = clone(child_process,
                        child_stack + STACK_SIZE,   /* Points to start of downwardly growing stack */ 
                        CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD,
                        argv);
    if (child_pid == -1)
    {
        errExit("clone");
    }
    printf("PID of child created by clone() is %ld\n", (long) child_pid);

//system("echo 200000000 > /sys/fs/cgroup/memory/demo/memory.limit_in_bytes");
  //  system("echo 0 > /sys/fs/cgroup/memory/demo/memory.swapiness");
    char buffer[1024];
    sprintf(buffer, "echo %d > /sys/fs/cgroup/memory/demo/tasks", child_pid);
    system(buffer);    
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
