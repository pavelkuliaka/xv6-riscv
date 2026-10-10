#include "kernel/types.h"
#include "user/user.h"

static void
run(int argc, char *argv[], int m)
{
  int pid = getpid();
  char s[2] = {0, 0};

  for (int i = 0; i < argc; i++) {
    for (char *p = argv[i]; *p; p++) {
      s[0] = *p;
      if (m >= 0)
        mutex_lock(m);
      printf("%d: arg %d, char '%s'\n", pid, i, s);
      if (m >= 0)
        mutex_unlock(m);
    }
  }
}

int
main(int argc, char *argv[])
{
  int m = -1;

  if (argc > 1 && strcmp(argv[1], "-s") == 0) {
    argc--;
    argv++;
    if ((m = mutex()) < 0) {
      printf("mutex failed\n");
      exit(1);
    }
  }

  int pid = fork();
  if (pid < 0)
    exit(1);
  run(argc, argv, m);
  if (pid > 0)
    wait(0);
  if (m >= 0)
    close(m);
  exit(0);
}
