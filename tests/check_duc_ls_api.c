#include <check.h>
#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "duc.h"
#include "src/duc/cmd.h"
#include "src/duc/ducrc.h"  
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static int hidden_argc;
static char **hidden_argv;
extern struct cmd cmd_help;
extern struct cmd cmd_info;
extern struct cmd cmd_index;
extern struct cmd cmd_ls;
extern struct cmd cmd_gui;
extern struct cmd cmd_guigl;
extern struct cmd cmd_graph;
extern struct cmd cmd_xml;
extern struct cmd cmd_cgi;
extern struct cmd cmd_ui;


#define SUBCOMMAND_COUNT (sizeof(cmd_list) / sizeof(cmd_list[0]))

static struct cmd *find_cmd_by_name(const char *name);
static void help_cmd(struct cmd *cmd);
static void show_version(void);

#define TRUE 1
#define FALSE 0

int32_t read_from_fd(char * buffer,int32_t fd)
{
	int32_t r_check=0;
	char *ptr=buffer;
	int32_t count=0;
	
	do
	{
		r_check=read(fd,ptr,128);
		if(r_check>0)
		{
			ptr+=r_check;
			count+=r_check;
		}
			
	}while(r_check!=0 && r_check!=-1 && errno!=EINTR);

	if(count)
		return FALSE;
	
	
	return TRUE;
}


char* call_cmd(struct cmd *cmd_ptr)
{
	if(cmd_ptr==NULL)
		return NULL;
	
	duc *duc = duc_new();
	struct ducrc *ducrc = ducrc_new(cmd_ptr->name);
	ducrc_add_options(ducrc, cmd_ptr->options);
	
	if(cmd_ptr->init)
		cmd_ptr->init(duc, hidden_argc,hidden_argv);
		
	int32_t out = open("report.log", O_RDWR|O_CREAT|O_APPEND, 0600);
	dup2(out, fileno(stdout));
	ducrc_getopt(ducrc, &hidden_argc, &hidden_argv);
	cmd_ptr->main(duc,hidden_argc,hidden_argv);
	close(out);
	
	char *buffer=malloc(128);
	if(buffer==NULL) return NULL;
	
	memset(buffer,0x0,128);
	out = open("report.log", O_RDONLY, 0600);
	int32_t r_ret=read_from_fd(buffer,out);
	close(out);
	
	
	remove("report.log");
	ducrc_free(ducrc);
	duc_del(duc);
	
	if(r_ret)
		return NULL;

	return buffer;
}


START_TEST(open_test)
{
	int32_t out = open("tests/test_read.log", O_RDONLY, 0600);
	ck_assert_int_ne(out,-1);
	ck_assert_int_ne(close(out),-1);
}
END_TEST



START_TEST(read_from_fd_test)
{
	int32_t out = open("tests/test_read.log", O_RDONLY, 0600);
	
	char *buffer=malloc(128);
	ck_assert_ptr_nonnull(buffer);
	
	memset(buffer,0x0,128);
	int32_t r_ret=read_from_fd(buffer,out);
	ck_assert_int_eq(r_ret,FALSE);
	ck_assert_int_eq(strlen(buffer),5);
	ck_assert_str_eq(buffer,"test\n");
}
END_TEST

START_TEST(duc_new_test)
{
	duc *duc=NULL;
	
	ck_assert_ptr_null(duc); 
	ck_assert_ptr_nonnull(duc = duc_new());
	duc_del(duc);
}
END_TEST


START_TEST(ducrc_new_test)
{
	struct ducrc *ducrc = NULL;
	
	ck_assert_ptr_null(ducrc);
	ck_assert_ptr_nonnull(ducrc=ducrc_new(cmd_ls.name));
	ducrc_free(ducrc);
}
END_TEST

START_TEST(ls_cmd)
{
	char *ret=NULL;

	ck_assert_ptr_null(ret);
	ck_assert_ptr_null(ret=call_cmd(NULL));
	ck_assert_ptr_nonnull(ret=call_cmd(&cmd_ls));
	ck_assert_int_eq(strlen(ret),8);
	ck_assert_str_eq(ret,"0 a.txt\n");
}
END_TEST
 
Suite * testUnit_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("testAPI");

    /* Core test case */
    tc_core = tcase_create("Core");
    tcase_add_test(tc_core,open_test);
    tcase_add_test(tc_core,read_from_fd_test);
	tcase_add_test(tc_core, duc_new_test);
	tcase_add_test(tc_core, ducrc_new_test);
    tcase_add_test(tc_core, ls_cmd);
    suite_add_tcase(s, tc_core);

    return s;
}

 int main(int argc,char **argv)
 {
    int number_failed;
    Suite *s;
    SRunner *sr;

	hidden_argv=argv;
	hidden_argc=argc;
    s = testUnit_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }
