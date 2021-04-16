
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mount.h>

static char child_stack[1048576];

struct clone_args{
	const char * rootPath;
};

static int child_fn(void * clone_args) {
  //printf("New `net` Namespace:\n");
  printf("%s\n", "aaya re");
  struct clone_args* args = (struct clone_args *)clone_args;
  mount("../rootfs","./", "ext4", 0, 0);
  printf("%s\n", "mounting completed");
  execlp("/bin/bash", "bin/bash/", NULL);
  printf("%s\n", args->rootPath );
  return 0;
}

int main() {
  printf("Original `net` Namespace:\n");
  // system("ip link");
  // printf("\n\n");
  const char *path = "/home/rshankar/CS695/assignment3/rootfs";

  struct clone_args *args;
  args->rootPath = path;
 
  void * arg = (void *)args;
  printf("%s\n", "came here");
  pid_t child_pid = clone(child_fn, child_stack+1048576, CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, arg);
  printf("%s\n", "terminated");
  sleep(10);
  waitpid(child_pid, NULL, 0);
  return 0;
}
