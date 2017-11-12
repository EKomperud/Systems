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
static void write_var_to(int fd, script_var *var);
static void read_to_var(int fd, script_var *var);


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
  } 
  else {
    int i;
    for (i = 0; i < scr->num_groups; i++) {
		run_group(&scr->groups[i]);
    }
  }
}

static void run_group(script_group *group) {

  if (group->repeats == 1) {
    if (group->num_commands == 1) { 
    	int pid;
    	int child_status;
    	int out_from_child[2];
    	char out;
    	int in_to_child[2];
    	char in;
    	script_command *command = &group->commands[0];
    	out = command->output_to != NULL;
    	in = command->input_from != NULL;
      	if (out) {
      		Pipe(out_from_child);
      	} 
      	if (in) {      		
      		Pipe(in_to_child);      		
  			write_var_to(in_to_child[1], command->input_from);
  			Close(in_to_child[1]);
  		}
  		pid = Fork();
  		if (pid == 0) {
  			if (out) {
  				Dup2(out_from_child[1], 1);
  				Close(out_from_child[0]);
  				Close(out_from_child[1]);
  			}
  			if (in) {
  				Dup2(in_to_child[0], 0);
  				Close(in_to_child[0]);
  				Close(in_to_child[1]);
  			}
  			run_command(command);
  		}
  		else {
  			if (command->pid_to != NULL) {
  				set_var(command->pid_to, pid);
  			}
	      	Waitpid(pid,&child_status,0);		
	    	child_status = WEXITSTATUS(child_status);
	    	if (out) {
      			Close(out_from_child[1]);
	    		if (child_status == 0) {	    			
	    			read_to_var(out_from_child[0], command->output_to);
	    			Close(out_from_child[0]);			
	    		}
	    		else {
	    			set_var(command->output_to, child_status);
	    		}
	    	}
	    	if (in) {
  				Close(in_to_child[0]);
	    	}
    	}
      	return;
    }
    else {
      	int c;
      	int child_status;
      	int pids[group->num_commands];
      	int pipes[group->num_commands][2];
      	for (c = 0; c < group->num_commands; c++) {
      		if (&group->commands[c].output_to != NULL) {
      			Pipe(pipes[c]);
      			Dup2(pipes[c][1], 1);
      		}
        	run_command(&group->commands[c]);
        	pids[c] = *((int *)&group->commands[c].extra_data);
      	}
		for (c = 0; c < group->num_commands; c++) {
	  		Waitpid(-1, &child_status, 0);
	  		int c2;
	  		for (c2 = 0; c2 < group->num_commands; c2++) {
	  			if (child_status == pids[c2])
	  				break;
	  		}
	  		child_status = WEXITSTATUS(child_status);
	  		if (&group->commands[c2].output_to != NULL) {
    			if (child_status == 0) {
    				read_to_var(pipes[c2][0], &group->commands[c2].output_to);
    			}
    			else {
    				set_var(&group->commands[c2].output_to, child_status);
    			}
    		}
		}
    }
  }
  else {
    int r;
    int fds[2];
    for (r = 0; r < group->repeats; r++) {
		if (group->num_commands == 1) {
	  		run_command(&group->commands[0]);
		}
	  	else {
	  		int c;
	  		for (c = 0; c < group->num_commands; c++) {
	      		run_command(&group->commands[c]);
	  		}
	  	}
	}
  }
}

static void run_command(script_command *command) {
  const char **argv;
  int i;

  argv = malloc(sizeof(char *) * (command->num_arguments + 2));
  argv[0] = command->program;
  
  for (i = 0; i < command->num_arguments; i++) {
    if (command->arguments[i].kind == ARGUMENT_LITERAL)
      argv[i+1] = command->arguments[i].u.literal;
    else
      argv[i+1] = command->arguments[i].u.var->value;
  }
  
  argv[command->num_arguments + 1] = NULL;

  //if (Fork() == 0)
  	Execve(argv[0], (char * const *)argv, environ);
  //	command->extra_data = malloc(sizeof(int));
  //	command->extra_data = pid;
  	free(argv);
  	return;
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
