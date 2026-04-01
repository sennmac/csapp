#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

enum { MAX_LINE = 1024, MAX_ARGS = 128 };

struct Command {
  char *argv[MAX_ARGS];
  int argc;
  char *redirect_out;
  bool background;
};

static void reset_command(struct Command *cmd) {
  memset(cmd, 0, sizeof(*cmd));
}

static int parse_line(char *line, struct Command *cmd) {
  char *token;
  char *save = NULL;

  reset_command(cmd);

  token = strtok_r(line, " \t\r\n", &save);
  while (token != NULL) {
    if (strcmp(token, "&") == 0) {
      cmd->background = true;
    } else if (strcmp(token, ">") == 0) {
      token = strtok_r(NULL, " \t\r\n", &save);
      if (token == NULL) {
        fprintf(stderr, "minish: missing file after '>'\n");
        return -1;
      }
      cmd->redirect_out = token;
    } else {
      if (cmd->argc + 1 >= MAX_ARGS) {
        fprintf(stderr, "minish: too many arguments\n");
        return -1;
      }
      cmd->argv[cmd->argc++] = token;
    }
    token = strtok_r(NULL, " \t\r\n", &save);
  }

  cmd->argv[cmd->argc] = NULL;
  return 0;
}

static int run_builtin(struct Command *cmd) {
  char cwd[PATH_MAX];

  if (cmd->argc == 0) {
    return 1;
  }

  if (strcmp(cmd->argv[0], "exit") == 0 || strcmp(cmd->argv[0], "quit") == 0) {
    exit(0);
  }

  if (strcmp(cmd->argv[0], "cd") == 0) {
    const char *target = cmd->argc > 1 ? cmd->argv[1] : getenv("HOME");
    if (target == NULL) {
      fprintf(stderr, "minish: HOME is not set\n");
      return 1;
    }
    if (chdir(target) != 0) {
      perror("minish: cd");
    }
    return 1;
  }

  if (strcmp(cmd->argv[0], "pwd") == 0) {
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
      perror("minish: pwd");
      return 1;
    }
    puts(cwd);
    return 1;
  }

  if (strcmp(cmd->argv[0], "help") == 0) {
    puts("minish builtins: help, pwd, cd, exit");
    puts("TODO Day 4: install signal handling and process groups");
    puts("TODO Day 5: add a real jobs table plus bg/fg builtins");
    return 1;
  }

  if (strcmp(cmd->argv[0], "jobs") == 0 ||
      strcmp(cmd->argv[0], "bg") == 0 ||
      strcmp(cmd->argv[0], "fg") == 0) {
    puts("TODO: implement job control in Day 5");
    return 1;
  }

  return 0;
}

static void apply_redirection(const struct Command *cmd) {
  int fd;

  if (cmd->redirect_out == NULL) {
    return;
  }

  fd = open(cmd->redirect_out, O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (fd < 0) {
    perror("minish: open");
    _exit(1);
  }

  if (dup2(fd, STDOUT_FILENO) < 0) {
    perror("minish: dup2");
    close(fd);
    _exit(1);
  }

  close(fd);
}

static void run_external(struct Command *cmd) {
  pid_t pid = fork();
  int status = 0;

  if (pid < 0) {
    perror("minish: fork");
    return;
  }

  if (pid == 0) {
    /*
     * Day 4 TODO:
     * - setpgid(0, 0) to isolate the child in its own process group
     * - restore default SIGINT/SIGTSTP handling in the child
     */
    apply_redirection(cmd);
    execvp(cmd->argv[0], cmd->argv);
    perror(cmd->argv[0]);
    _exit(errno == ENOENT ? 127 : 126);
  }

  if (cmd->background) {
    printf("[bg pid %d] %s\n", pid, cmd->argv[0]);
    return;
  }

  if (waitpid(pid, &status, 0) < 0) {
    perror("minish: waitpid");
    return;
  }

  if (WIFSIGNALED(status)) {
    printf("[fg pid %d] terminated by signal %d\n", pid, WTERMSIG(status));
  }
}

int main(void) {
  char line[MAX_LINE];

  while (1) {
    struct Command cmd;

    printf("minish> ");
    fflush(stdout);

    if (fgets(line, sizeof(line), stdin) == NULL) {
      if (feof(stdin)) {
        putchar('\n');
        break;
      }
      perror("minish: fgets");
      return 1;
    }

    if (parse_line(line, &cmd) != 0) {
      continue;
    }

    if (run_builtin(&cmd)) {
      continue;
    }

    run_external(&cmd);
  }

  return 0;
}
