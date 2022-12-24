/*
    name: Raquel Reyes
    ID: 1001826263
*/

// The MIT License (MIT)
//
// Copyright (c) 2016 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 10     // Mav shell only supports four arguments

#define MAX_CMD_HISTORY 15      // history can hold up to last 15 commands

#define MAX_NUM_PIDS    20      // "listpids" can store up to the last 20 pids


//all function definitions included below

int special_commands(char *token[], char history[][MAX_COMMAND_SIZE],
    pid_t pids[MAX_NUM_PIDS], int, int, int *);
int check_nth_cmd(char *token[], char history[][MAX_COMMAND_SIZE], int);

int exec_func(int *pid_num, char *args[], pid_t pids[]);

void print_history(char history[][MAX_COMMAND_SIZE]);
void add_history(int *, char history[][MAX_COMMAND_SIZE], char *);

void add_pids(int *pid_num, pid_t pid, pid_t pids[]);
void print_pids(pid_t pids[], int);

//global to keep track of zeroth command when commands get shifted after limit
//will store before shifting, so if !0 is called, we will save string to
//tokenize as string stored in zeroth
char zeroth[MAX_COMMAND_SIZE];

int main()
{

  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );

  //create pid and command history storage arrays
  //dimensions for history list mean it will have a max of 15 strings
  // and each string can hold up to 255 characters, since that is the same as the command max itself
  //history list is filled with null chars as habit to prevent random data that could infringe in
  //storing our words
  char history_list[MAX_CMD_HISTORY][MAX_COMMAND_SIZE] = {{'\0'}};
  pid_t pid_list[MAX_NUM_PIDS] = {};

  //create pid and history element number trackers
  //these will keep track of how many elements are in the history/pid arrays
  int history_num, pid_num = 0;

  //this int is to be used as a flag that will denote whether the entered command is a valid
  //will be set as the return value of the check_nth_cmd( ) function, but needs to be seen in
  //main so it's declared here and just passed by reference to the appropriate functions later on
  //holds index of history command needed to run in history arrray if valid
  int valid_nth = -2;

  while( 1 )
  {
    //if the valid_nth command flag is -1, that means the previous command entered is not an
    //executable !n command, and so we can pull in the current command from user input in stdin
    //since its set outside of the loop as -1, for the first iteration it will run the inside code
    if(valid_nth == -2)
    {
        // Print out the msh prompt
        printf ("msh> ");

        // Read the command from the commandline.  The
        // maximum command that will be read is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something since fgets returns NULL when there
        // is no input
        while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );

        //before parsing the input, store element in history array
        //before storing command string in history, cutting out the newline char by
        //replacing it with a null char
        //if running a !n command, we would not to store it b/c all we want is it to
        //execute as it was already stored in the previous cycle so it's inside the
        //valid_nth if()
        command_string[strlen(command_string) - 1] = '\0';

        add_history(&history_num, history_list, command_string);
    }

        /* Parse input */
        char *token[MAX_NUM_ARGUMENTS];

        int   token_count = 0;

        // Pointer to point to the token
        // parsed by strsep
        char *argument_ptr;
        char *working_string;

    //if valid_nth is -2, that is generic condition for if
    //the !n command was not valid and we will be taking regular input
    if(valid_nth == -2)
    {
        //if it's a regular command, we will be tokenizing the command string,
        //which has been filled by user input in stdin
        working_string  = strdup( command_string );
    }
    else
    {
        //if the command was !n, we want the working_string, the one to tokenize,
        //to be whatever history input the !n is referencing
        //valid_nth holds the index of the desired history command, so we just strdup
        //
        //valid_nth == 1 is a special case for the 0th command as it would have
        //been shifted so it is saved in zeroth
        if(valid_nth == -1)
        {
            working_string = strdup(zeroth);
            valid_nth = -2;
        }
        //every other besides 0th command will just pull from history index
        //in function assigning valid_nth, number is subtracted by 1 to grab the
        //accurate, post-shifted command
        else
        {
            working_string = strdup(history_list[valid_nth]);
            valid_nth = -2;
        }
    }

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    //if token is not null or invalid character we can safely use strcmp w/out faut
    if(token[0] != NULL)
    {
        //these are the only 2 cases where shell will exit "normally" via keyboard
        //commands, so we are returning zero in main to exit, rather the strcmps be
        //in a seperate function
        if(!strcmp(token[0], "exit") || !strcmp(token[0], "quit"))
        {
            free( head_ptr );
            return 0;
        }

        //not_special is being used as an integer flag that will be set to 1 if all the
        //created commands (cd, history, listpids, etc) are not selected, meaning they have
        //failed strcmps of the tokenized input
        //if any built in command is found (aka function for them is called), we return zero,
        //setting not-special to zero, causing the exec to be skipped
        int not_special = special_commands(token, history_list, pid_list, history_num, pid_num, &valid_nth);

        if(not_special == 1)
        {
            exec_func(&pid_num, token, pid_list);
        }
    }
    //I was getting errors/random input stored when inputed command has multiple
    //arguments, so I'm just clearing token array here to prevent any potential
    //left-over garbage
    for(int i = 0; i < token_count; i++)
    {
        token[i] = NULL;
    }

    free( head_ptr );

  }
  return 0;
}
//Name:
    //add_history()
//Expected parameters:
    //address of integer equal to current commands stored
    //to manipulate inside function
    //array of strings representing array holding previous history commands
    //string holding command that was just inputted
//return values:
    //VOID (none)
//Description:
    //This function is called at the beginning of the program right after taking input fom
    //stdin and is meant to add the string to the next available index in the given history
    //array whiich is used in the history command
void add_history(int *history_num, char history[][MAX_COMMAND_SIZE], char *command_string)
{
    //Newline character when pressing enter for new input was removed in main
    //but here we check if there is one or, if we only hit an enter, then it has been
    //replaced with the string terminating character, in which case we do not want
    //anything to happen, INCLUDING not incrementing history number so we just skip
    //could also have been done with a return.... oops
    if(command_string[0] != '\n' && command_string[0] != '\0')
    {
        //if we still have not reached the limit of the history array, we can
        //just add the command_string to the first available index at the end of
        //the history array, and then increment the history number
        //since we want to directly change the value of the history number,
        //we are using pointers to the int
        if(*history_num < MAX_CMD_HISTORY)
        {
            strcpy(history[(*history_num)++], command_string);
        }
        //if we have reached the limit of commands stored in the array, we want to
        //begin shifting the commands stored to the left by one, essentially
        //overwriting thee command at index 0
        //we save it off before shifting (to be used in !0) and use a for loop
        //to shift everything over to the previous index, and then copying the
        //new command into the last index, index 14, since after hitting the
        //limit, historry number wil always stay at 15
        else
        {
            strncpy(zeroth, history[0], MAX_COMMAND_SIZE);
            for(int i = 1; i < MAX_CMD_HISTORY; i++)
            {
                strncpy(history[i-1], history[i], MAX_COMMAND_SIZE);
            }
            strncpy(history[(*history_num)-1], command_string, MAX_COMMAND_SIZE);
        }
    }
}

//Name:
    //print_history()
//Expected parameters:
    //array of strings representing array holding
    //previous history commands
//return values:
    //VOID (none)
//Description:
    //This function is used to print out the history array storing previous
    //commands when the created "history" command is used
void print_history(char history[][MAX_COMMAND_SIZE])
{
    //if there is nothing in the array, some type of error, just return
    //else, loop through the indices of the array, printing each one, until
    //the last empty spot or end of array is found
    if(history == NULL)
    {
        return;
    }
    for(int i = 0; strcmp(history[i], "\0"); i++)
    {
        printf("%d: %s\n", i, history[i]);
    }
}

//Name:
    //special_commands()
//Expected parameters:
    //array holding tokenized inputted command, including multiple args
    //array of strings representing array holding previous history commands
    //array holding all values of pids currently stored
    //integer equal to number of history commands currently entered +1
    //integer equal to number of pids currently stored in pid array
    //integer equal to -2 if a command is not a valid !n or set to
    //the n value entered in !n if it is within bounds (checked by function)
//return values:
    //integer set to 1 if its not a command we created, or 0 if one of our
    //built in commands handles it
//Description:
    //function string compares for all our potential created commands and if
    //there is a match it will call the appropriate function w/ parameters
    //and return zero, otherwise we will just return 1, setting a flag that
    //we can go ahead and use exec
int special_commands(char *token[], char history[][MAX_COMMAND_SIZE],
    pid_t pids[MAX_NUM_PIDS], int history_num, int pid_num, int *valid_nth)
{
    if(token[0] == NULL)
    {
        return -1;
    }
    //if strcmp matches with history, we call our history function
    //that takes care of the functionality of the "history" command
    else if(!strcmp(token[0], "history"))
    {
        print_history(history);
        return 0;
    }
    //if strcmp matches with listpids, we call our print pids function
    //that takes care of the functionality of the "listpids" command
    else if(!strcmp(token[0], "listpids"))
    {
        print_pids(pids, pid_num);
        return 0;
    }
    //if strcmp matches with cd, we call chdir
    //that takes care of the functionality of the "cd" command
    else if(!strcmp(token[0], "cd"))
    {
        chdir(token[1]);
        return 0;
    }
    //if strcmp matches with !, we call nth command function
    //that checks if we have a valid !n command
    //if its not valid -2 will be assigned, otherwise returns
    //index of the valid desired command in history array
    else if((token[0][0] == '!') && (token[1] == NULL))
    {
        *valid_nth = check_nth_cmd(token, history, history_num);
        return 0;
    }
    //if all conditions do not apply, return -1 to set flag
    //will cause exec function to run
    else
        return 1;
}

//Name:
    //add_pids()
//Expected parameters:
    //address of int representing number of pids
    //stored in pid arrray
    //pid from newly created process
    //array of pids to store newly created process pid
//return values:
    //VOId (none)
//Description:
    //function will add a pid number to the end of a pid
    //array, and if the max number of pids held is reached
    //we shift all the stored pids to the left and add the
    //new one at the end
void add_pids(int *pid_num, pid_t pid, pid_t pids[])
{
    //if number of pids in array is less than max, add
    //at end of array
    if(*pid_num < MAX_NUM_PIDS)
    {
        pids[(*pid_num)++] = pid;
    }
    //else if max is reached, shift everything over
    else
    {
        for(int i = 1; i < MAX_NUM_PIDS; i++)
        {
            pids[i-1] = pids[i];
        }
        pids[(*pid_num) - 1] = pid;
    }
}

//Name:
    //print_pids()
//Expected parameters:
    //array holding all previously stored pids
    //integer representing number of previously stored pids
//return values:
    //VOID (none)
//Description:
    //function prints out all the pids stored in the pid array
    //when using the "listpids" command
void print_pids(pid_t pids[], int pid_num)
{
    if(pids == NULL)
    {
        return;
    }

    //loops through indices of pid array to print out each one
    for(int i = 0; (i < MAX_NUM_PIDS) && (i < pid_num); i++)
    {
        printf("%d: %d\n", i, pids[i]);
    }
}

//Name:
    //check_nth_cmd()
//Expected parameters:
    //array holding tokenized inputed command
    //array holding all previous commands entered
    //integer equal to number of commands currently held in history array
//return values:
    //integer set to -2 if !n is invalid for any reason, otherwise
    //returns index of desired valid history command
//Description:
    //checks after ! if there is a valid number, if there isnt return -2
    //if there is convert string to number and check if within bounds
    //if so return number, otherwise return -2
int check_nth_cmd(char *token[], char history[][MAX_COMMAND_SIZE], int history_num)
{
    char after_ex[MAX_COMMAND_SIZE] = {};
    int valid_nth = -2;

    strncpy(after_ex, &token[0][1], MAX_COMMAND_SIZE);

    //check to see if values after exclamation point are all numbers
    for(int i = 0; i < strlen(after_ex); i++)
    {
        if(!isdigit(after_ex[i]))
        {
            return valid_nth;
        }
    }

    int cmd_num = atoi(after_ex);

    if((cmd_num >= MAX_CMD_HISTORY) || ((cmd_num >= (history_num-1)) && (cmd_num != 14)))
    {
        printf("Command not in history.\n");
    }
    else if(history_num == MAX_CMD_HISTORY)
    {
        valid_nth = cmd_num - 1;
    }
    else
    {
        valid_nth = cmd_num;
    }
    return valid_nth;
}

//Name:
    //exec_func()
//Expected parameters:
    //integer holding number of pids stored in pid array
    //array holding tokenized input
    //array holding all previous stored pids
//return values:
    //zero just to exit function
//Description:
    //this will check the command if found and can be executed
    //using execvp and if returns -1 (fails) command was not
    //found
    //otherwise if valid command, pid is saved into array
    //and process is created
    //waits until child process is done
    //and theen function finishes, returning 0
int exec_func(int *pid_num, char *args[], pid_t pids[])
{

    char *search_here[] = {"", "/usr/local/bin", "/usr/bin", "/bin"};
    char concatstr[MAX_COMMAND_SIZE + 25] = {};
    int returnval;

    pid_t pid = fork();

    //add_pids(pid_num, pid, pids);

    if(pid == 0)
    {

        for(int i = 0; i < sizeof(search_here)/sizeof(search_here[0]); i++)
        {
            strcpy(concatstr, search_here[i]);
            strncat(concatstr, args[0], MAX_COMMAND_SIZE);
            returnval = execvp(concatstr, args);
        }

        if(returnval = -1)
        {
            printf("%s: Command not found.\n", args[0]);
            exit(0);
        }
    }
    else
    {
        int status;
        wait(&status);
        add_pids(pid_num, pid, pids);
    }
    return 0;
}


