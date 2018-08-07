/*
 ============================================================================
 Name        : Controller.c
 Author      : Harry Detsis
 Version     : 1.0
 Copyright   : Nop
 Description : Controlling proccess for injector
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "helpers.h"

#define HALT "halting"

const char* BCC = "/opt/bcc-2.0.2-gcc/bin/sparc-gaisler-elf-gcc ";
const char* SOURCE = " /home/harry/workspace/Injector_Sparc/injector_sparc.c ";
char OPT[256];
const char* OUTPUT = " injector_sparc_auto";
const char* OUT_FILE = "output";

const char* TSIM = "/opt/TSIM\\ Evaluation/tsim-eval/tsim/linux/tsim-leon3 ";


int
search_in_file (char *fname, char *str)
{
  FILE *fp;
  int line_num = 1;
  int find_result = 0;
  char temp[512];
  int found = 0;

  if ((fp = fopen (fname, "r")) == NULL)
    {
      return (-1);
    }

  while (fgets (temp, 512, fp) != NULL)
    {
      if ((strstr (temp, str)) != NULL)
	{
	  //printf ("A match found on line: %d\n", line_num);
	  //printf ("\n%s\n", temp);
	  find_result++;
	  found = line_num;
	}
      line_num++;
    }

  if (find_result == 0)
    {
      //printf ("\nSorry, couldn't find a match.\n");
    }

  //Close the file if still open.
  if (fp)
    {
      fclose (fp);
    }
  return (found);
}

void
generate_options (char *svalue, char *evalue)
{
  //int start_hex = (int)strtol(svalue, NULL, 16);
  //int end_hex = (int)strtol(evalue, NULL, 16);
  char* opt_start[50];
  char* opt_end[50];

  char* sub0 = malloc (3);
  char* sub1 = malloc (3);
  char* sub2 = malloc (3);
  char* sub3 = malloc (3);
  if (svalue)
    {
      strncpy (sub0, svalue, 2);
      strncpy (sub1, svalue + 2, 2);
      strncpy (sub2, svalue + 4, 2);
      strncpy (sub3, svalue + 6, 2);
      sub0[2] = '\0';
      sub1[2] = '\0';
      sub2[2] = '\0';
      sub3[2] = '\0';
      snprintf (opt_start, sizeof opt_start, " -DSTART0=0x%s"
		" -DSTART1=0x%s"
		" -DSTART2=0x%s"
		" -DSTART3=0x%s",
		sub0, sub1, sub2, sub3);
    }
  if (evalue)
    {
      strncpy (sub0, evalue, 2);
      strncpy (sub1, evalue + 2, 2);
      strncpy (sub2, evalue + 4, 2);
      strncpy (sub3, evalue + 6, 2);
      sub0[2] = '\0';
      sub1[2] = '\0';
      sub2[2] = '\0';
      sub3[2] = '\0';
      snprintf (opt_end, sizeof opt_end, " -DEND0=0x%s"
		" -DEND1=0x%s"
		" -DEND2=0x%s"
		" -DEND3=0x%s",
		sub0, sub1, sub2, sub3);
    }
  if (!evalue)
    {
      snprintf (OPT, sizeof OPT, "%s -o", opt_start);
    }
  else
    {
      snprintf (OPT, sizeof OPT, "%s%s -o", opt_start, opt_end);
    }

}

int
compile_cont (char *svalue)
{
  generate_options (svalue, NULL);
  char buff[256];
  snprintf (buff, sizeof buff, "%s%s%s%s", BCC, SOURCE, OPT, OUTPUT);
  system (buff);

  return 1;
}

int
compile (int argc, char **argv)
{
  int c;
  char *svalue = NULL;
  char *evalue = NULL;

  opterr = 0;

  while ((c = getopt (argc, argv, "s:e:")) != -1)
    switch (c)
      {
      case 's':
	svalue = optarg;
	break;
      case 'e':
	evalue = optarg;
	break;
      case '?':
	if (optopt == 's' || optopt == 'e')
	  fprintf (stderr, "Option -%c requires an argument.\n", optopt);
	else if (isprint (optopt))
	  fprintf (stderr, "Unknown option `-%c'.\n", optopt);
	else
	  fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);

	return 0;
      }

  generate_options (svalue, evalue);
  char buff[256];
  snprintf (buff, sizeof buff, "%s%s%s%s", BCC, SOURCE, OPT, OUTPUT);
  system (buff);

  return 1;
}

void
execute (char* file)
{
  char execution_buff[256];
  char buff[256];
  snprintf (buff, sizeof buff, "%s%s >%s", TSIM, OUTPUT, file);

  FILE *fp = NULL;
  char path[512];

  fp = popen (buff, "w");

  if (fp == NULL)
    {
      printf ("\nFailed command\n");
      return;
    }
  else
    {
      printf ("\nSuccessful command\n");
    }

  fputs ("run", fp);
  fputc ('\n', fp);
  printf ("starting with : %d", fp);

  printf ("\nRunning...\n");
  /* close */
  pclose (fp);

  printf ("\nEnd\n");
}

int
main (int argc, char **argv)
{
  char *instruction = NULL;
  int file_id = 0;
  char file_buf[50];
  snprintf(file_buf, sizeof file_buf, "%s_%d.txt", OUT_FILE, file_id);

  int line_no = search_in_file (file_buf, HALT);

  if (line_no == -1)
    {
      printf ("Creating new file\n");
      if (!compile (argc, argv))
	return EXIT_FAILURE;
    }
  else if (line_no == 0)
    {
      printf ("Starting from the beginning\n");
      if (!compile (argc, argv))
	return EXIT_FAILURE;
    }
  else
    {
      extract_last_instruction (file_buf, line_no - 1, &instruction);
      printf ("Last instruction: %s\n", instruction);
      if (!compile_cont (instruction))
	{
	  return EXIT_FAILURE;
	}
    }

  execute (file_buf);
  line_no = search_in_file (file_buf, HALT);

  while (line_no > 0)
    {
      extract_last_instruction (file_buf, line_no - 1, &instruction);
      printf ("Simulation halted, continuing at: %s", instruction);
      compile_cont (instruction);
      file_id++;
      snprintf(file_buf, sizeof file_buf, "%s_%d.txt", OUT_FILE, file_id);
      execute (file_buf);
      line_no = search_in_file (file_buf, HALT);
    }
  //if (!compile (argc, argv))
  //return EXIT_FAILURE;
  //execute ();
  return EXIT_SUCCESS;
}
