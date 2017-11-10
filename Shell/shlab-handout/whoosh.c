/* This is the main file for the `whoosh` interpreter and the part
   that you modify. */

#include <stdlib.h>
#include <stdio.h>
#include "csapp.h"
#include "ast.h"
#include "fail.h"

static void run_script(script *scr);
static void run_group(script_group *group);
static void run_command(script_command *command);
static void set_var(script_var *var, int new_value);

/* You probably shouldn't change main at all. */

int main(int argc, char **argv) {
  script *scr;
  
  if ((argc != 1) && (argc != 2)) {
    fprintf(stderr, "usage: %s [<script-file>]\n", argv[0]);
    exit(1);
  }

  scr = parse_script_file((argc > 1) ? argv[1] : NULL);

  run_script(scr);

  return 0;
}

static void run_script(script *scr) {
  if (scr->num_groups == 1) {
    run_group(&scr->groups[0]);
  } else {
    int i;
    for (i = 0; i < scr->num_groups; i++) {
      int pid = Fork();
      if (pid == 0) {
	run_group(&scr->groups[i]);
	exit(1);
      }
      else {
	int child_status;
	Waitpid(pid, &child_status, 0);
      }
    }
  }
}

static void run_group(script_group *group) {
  if (group->repeats == 1) {
    if (group->num_commands == 1) {
      run_command(&group->commands[0]);
    }
    else {
      int c;
      int pid_2;
      int child_status;
      for (c = 0; c < group->num_commands; c++) {
        pid_2 = Fork();
        if (pid_2 == 0) {
          run_command(&group->commands[c]);
	  exit(1);
        }
      }
      if (pid_2 != 0) {
	for (c = 0; c < group->num_commands; c++) {
	  Waitpid(-1, &child_status, 0);
	}
      }
      /* logic for or here */
    }
  }
  else {
    int r;
    for (r = 0; r < group->repeats; r++) {
      int pid = Fork();
      if (pid == 0) {
	if (group->num_commands == 1) {
	  run_command(&group->commands[0]);
	}
	else {
	  int c;
	  int pid_2;
	  int child_status;
	  for (c = 0; c < group->num_commands; c++) {
	    pid_2 = Fork();
	    if (pid_2 == 0) {
	      run_command(&group->commands[c]);
	    }
	  }
	  if (pid_2 != 0) {
	    for (c = 0; c < group->num_commands; c++) {
	      Waitpid(-1, &child_status, 0);
	    }
	  }
	  /* logic for or here */
	}
      }
      else {
	int child_status;
	Waitpid(pid, &child_status, 0);
      }
    }
  }
}

/* This run_command function is a good start, but note that it runs
   the command as a replacement for the `whoosh` script, instead of
   creating a new process. */

static void run_command(script_command *command) {
  const char **argv;
  int i;

  if (command->pid_to != NULL) {
    set_var(command->pid_to, getpid());
  }
  if (command->input_from != NULL) {
    int some_var;
    read_to_var(some_var, command->input_from);
  } 
  if (command->output_to != NULL) {
    
  }

  argv = malloc(sizeof(char *) * (command->num_arguments + 2));
  argv[0] = command->program;
  
  for (i = 0; i < command->num_arguments; i++) {
    if (command->arguments[i].kind == ARGUMENT_LITERAL)
      argv[i+1] = command->arguments[i].u.literal;
    else
      argv[i+1] = command->arguments[i].u.var->value;
  }
  
  argv[command->num_arguments + 1] = NULL;

  Execve(argv[0], (char * const *)argv, environ);

  free(argv);
}

/* You'll likely want to use this set_var function for converting a
   numeric value to a string and installing it as a variable's
   value: */
static void set_var(script_var *var, int new_value) {
  char buffer[32];
  free((void*)var->value);
  snprintf(buffer, sizeof(buffer), "%d", new_value);
  var->value = strdup(buffer);
}

/* You'll likely want to use this write_var_to function for writing a
   variable's value to a pipe: */
static void write_var_to(int fd, script_var *var) {
  size_t len = strlen(var->value);
  ssize_t wrote = Write(fd, var->value, len);
  wrote += Write(fd, "\n", 1);
  if (wrote != len + 1)
    app_error("didn't write all expected bytes");
}

/* You'll likely want to use this write_var_to function for reading a
   pipe's content into a variable: */
static void read_to_var(int fd, script_var *var) {
  size_t size = 4097, amt = 0;
  char buffer[size];
  ssize_t got;

  while (1) {
    got = Read(fd, buffer + amt, size - amt);
    if (!got) {
      if (amt && (buffer[amt-1] == '\n'))
        amt--;
      buffer[amt] = 0;
      free((void*)var->value);
      var->value = strdup(buffer);
      return;
    }
    amt += got;
    if (amt > (size - 1))
      app_error("received too much output");
  }
}
