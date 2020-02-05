/* Original Authors https://github.com/zevv/duc
 * ORIGINAL LICENSE GPL V3.0
 * 
 * Modified Version
 * 2020 5 Jan Matteo Bonicolini (matteo.bonicolini@gmail.com)
 * 
 * LICENSE: LGPL V3.0
 * */

#include "config.h"

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>
#include <locale.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include "cmd.h"
#include "duc.h"

#define MAX_DEPTH 32

#define COLOR_RESET  "\e[0m";
#define COLOR_RED    "\e[31m";
#define COLOR_YELLOW "\e[33m";

static char *tree_ascii[] = {
	"####",
	" `+-",
	"  |-",
	"  `-",
	"  | ",
	"    ",
};


static char *tree_utf8[] = {
	"####",
	" ╰┬─",
	"  ├─",
	"  ╰─",
	"  │ ",
	"    ",
};


static bool opt_apparent = false;
static bool opt_count = false;
static bool opt_ascii = false;
static bool opt_bytes = false;
static bool opt_classify = false;
static bool opt_directory = false;
static bool opt_color = false;
static bool opt_full_path = false;
static int width = 80;
static bool opt_graph = false;
static bool opt_recursive = false;
static char *opt_database = NULL;
static bool opt_dirs_only = false;
static int opt_levels = 4;
static bool opt_name_sort = false;


/* 
 * Calculate monospace font widht of string for aligning terminal output
 */

static size_t string_width(char *string)
{
	int w = 0;
	size_t n = mbstowcs(NULL, string, 0) + 1;
	wchar_t *wcstring = malloc(n * sizeof *wcstring);
	if (wcstring) {
		if (mbstowcs(wcstring, string, n) != (size_t)-1) {
			w = wcswidth(wcstring, n);
		}
		free(wcstring);
	}
	if(w <= 0) w = strlen(string);
	return w;
}


/*
 * List one directory. This function is a bit hairy because of the different
 * ways the output can be formatted with optional color, trees, graphs, etc.
 * Maybe one day this should be split up for different renderings 
 */

static char parent_path[DUC_PATH_MAX] = "";
static int prefix[MAX_DEPTH + 1] = { 0 };

static void ls_one(duc_dir *dir, int level, size_t parent_path_len)
{
	off_t max_size = 0;
	size_t max_name_len = 0;
	int max_size_len = 6;
	duc_size_type st = opt_count ? DUC_SIZE_TYPE_COUNT : 
	                   opt_apparent ? DUC_SIZE_TYPE_APPARENT : DUC_SIZE_TYPE_ACTUAL;
	duc_sort sort = opt_name_sort ? DUC_SORT_NAME : DUC_SORT_SIZE;

	if(level > opt_levels) return;

	char **tree = opt_ascii ? tree_ascii : tree_utf8;

	/* Iterate the directory once to get maximum file size and name length */
	
	struct duc_dirent *e;
	while( (e = duc_dir_read(dir, st, sort)) != NULL) {

		off_t size = duc_get_size(&e->size, st);

		if(size > max_size) max_size = size;
		size_t l = string_width(e->name);
		if(l > max_name_len) max_name_len = l;
	}

	if(opt_bytes) max_size_len = 12;
	if(opt_classify) max_name_len ++;

	/* Iterate a second time to print results */

	duc_dir_rewind(dir);

	size_t count = duc_dir_get_count(dir);
	size_t n = 0;

	while( (e = duc_dir_read(dir, st, sort)) != NULL) {

		if(opt_dirs_only && e->type != DUC_FILE_TYPE_DIR) continue;

		off_t size = duc_get_size(&e->size, st);

		if(opt_recursive) {
			if(n == 0)       prefix[level] = 1;
			if(n >= 1)       prefix[level] = 2;
			if(n == count-1) prefix[level] = 3;
		}
			
		char *color_on = "";
		char *color_off = "";

		if(opt_color) {
			color_off = COLOR_RESET;
			if(size >= max_size / 8) color_on = COLOR_YELLOW;
			if(size >= max_size / 2) color_on = COLOR_RED;
		}

		printf("%s", color_on);
		char siz[32];
		duc_human_size(&e->size, st, opt_bytes, siz, sizeof siz);
		printf("%*s", max_size_len, siz);
		printf("%s", color_off);

		if(opt_recursive && !opt_full_path) {
			int *p = prefix;
			while(*p) printf("%s", tree[*p++]);
		}

		putchar(' ');

		if(opt_full_path) {
			printf("%s", parent_path);
			parent_path_len += strlen(e->name) + 1;
			if(parent_path_len + 1 < DUC_PATH_MAX) {
				strcat(parent_path, e->name);
				strcat(parent_path, "/");
			}
		}

		size_t l = string_width(e->name) + 1;
		
		printf("%s", e->name);

		if(opt_classify) {
			putchar(duc_file_type_char(e->type));
			l++;
		}

		if(opt_graph) {
			for(;l<=max_name_len; l++) putchar(' ');
			int w = width - max_name_len - max_size_len - 5;
			w -= (level + 1) * 4;
			int l = max_size ? (w * size / max_size) : 0;
			int j;
			printf(" [%s", color_on);
			for(j=0; j<l; j++) putchar('+');
			for(; j<w; j++) putchar(' ');
			printf("%s]", color_off);
		}

		putchar('\n');
			
		if(opt_recursive && level < MAX_DEPTH && e->type == DUC_FILE_TYPE_DIR) {
			if(n == count-1) {
				prefix[level] = 5;
			} else {
				prefix[level] = 4;
			}
			duc_dir *dir2 = duc_dir_openent(dir, e);
			if(dir2) {
				ls_one(dir2, level+1, parent_path_len);
				duc_dir_close(dir2);
			}
		}

		if(opt_full_path) {
			parent_path_len -= strlen(e->name) + 1;
			parent_path[parent_path_len] = '\0';
		}
		
		n++;
	}

	prefix[level] = 0;
}


/*
 * Show size of this directory only
 */

static void ls_dir_only(const char *path, duc_dir *dir)
{
	struct duc_size size;
	char siz[32];

	duc_size_type st = opt_count ? DUC_SIZE_TYPE_COUNT : 
		           opt_apparent ? DUC_SIZE_TYPE_APPARENT : DUC_SIZE_TYPE_ACTUAL;
	duc_dir_get_size(dir, &size);
	duc_human_size(&size, st, opt_bytes, siz, sizeof siz);

	printf("%s %s", siz, path);

	if(opt_classify) {
		putchar('/');
	}

	putchar('\n');
}



/* auxiliary function added by Matteo Bonicolini LGPL V3.0
 * Check if path is a file
 * */
static int32_t is_file(const char *path)
{
	struct stat s;
	
	memset(&s,0x0,sizeof(struct stat));
	if(stat(path,&s))
		return 0;
		
    if(s.st_mode & S_IFREG)
		return 1;
		
	return 0;
}


/* itoa_s and ftoa are imported from https://gist.github.com/Belgarion
 * */

static int32_t itoa_s(int32_t value, char *buf) 
{
	int32_t index = 0;
    int32_t i = value % 10;
    
    if (value >= 10) index += itoa_s(value / 10, buf);
        
    buf[index] = i+0x30;
    index++;
    return index;
}

static void ftoa(float value, int32_t decimals, char *buf) 
{
        int32_t index = 0;

        if (value < 0) 
        {
			buf[index] = '-';
            index++;
            value = -value;
        }
        
        // Rounding
        float rounding = 0.5;
        for (int32_t d = 0; d < decimals; rounding /= 10.0, d++);
        value += rounding;

        // Integer part
        index += itoa_s((int)(value), buf+index);
        buf[index++] = '.';

        // Remove everything except the decimals
        value = value - (int32_t)(value);

        // Convert decmial part to integer
        int32_t ival = 1;
        for (int32_t d = 0; d < decimals; ival *= 10, d++);
        ival *= value;

        // Add decimal part to string
        index += itoa_s(ival, buf+index);
        buf[index] = '\0';
}



/* auxiliary function added by Matteo Bonicolini LGPL V3.0
 * It make print with stat for file
 * */
static void ls_one_file(duc_dir *dir,char * fileName,char* parent_name,int32_t parent_len,int32_t size_f)
{
	off_t max_size = 0;
	size_t max_name_len = 0;
	int max_size_len = 6;
	duc_size_type st = opt_count ? DUC_SIZE_TYPE_COUNT : 
	                   opt_apparent ? DUC_SIZE_TYPE_APPARENT : DUC_SIZE_TYPE_ACTUAL;
	duc_sort sort = opt_name_sort ? DUC_SORT_NAME : DUC_SORT_SIZE;


	char **tree = opt_ascii ? tree_ascii : tree_utf8;

	/* Iterate the directory once to get maximum file size and name length */
	struct duc_dirent *e;
	while( (e = duc_dir_read(dir, st, sort)) != NULL) 
	{

		if(strcmp(e->name,fileName)!=0)
			continue;
			
		off_t size = duc_get_size(&e->size, st);

		if(size > max_size) max_size = size;
		size_t l = string_width(e->name);
		if(l > max_name_len) max_name_len = l;
	}

	/* Iterate a second time to print results */

	duc_dir_rewind(dir);

	size_t count = duc_dir_get_count(dir);
	size_t n = 0;

	while( (e = duc_dir_read(dir, st, sort)) != NULL) 
	{
		
		if(strcmp(e->name,fileName)!=0)
			continue;

		char siz[32];
		duc_human_size(&e->size, st, opt_bytes, siz, 32);
		off_t size = duc_get_size(&e->size, st);
		printf("%s",siz);
		

		if(opt_recursive && !opt_full_path) 
		{
			int *p = prefix;
			while(*p) printf("%s", tree[*p++]);
		}

		putchar(' ');
		if(opt_directory) 
			printf("%s/%s", parent_name,e->name);
		else
			printf("%s", e->name);
		
		fflush(stdout);

		size_t l = string_width(e->name) + 1;
		
		

		if(opt_classify) {
			putchar(duc_file_type_char(e->type));
			l++;
		}

		if(opt_graph) 
		{
			for(;l<=max_name_len; l++) putchar(' ');
			int w = width - max_name_len - max_size_len - 5;
			w -=  4;
			int l = max_size ? (w * size / max_size) : 0;
			int j;
			printf(" [");
			for(j=0; j<l; j++) putchar('+');
			for(; j<w; j++) putchar(' ');
			printf("]");
		}

		putchar('\n');
			
		if(opt_recursive  && e->type == DUC_FILE_TYPE_DIR) {
			if(n == count-1) {
				prefix[0] = 5;
			} else {
				prefix[0] = 4;
			}
			duc_dir *dir2 = duc_dir_openent(dir, e);
			if(dir2) {
				ls_one(dir2, 1, strlen(parent_path));
				duc_dir_close(dir2);
			}
		}
		
		n++;
	}

	prefix[0] = 0;
}


/* modified version by Matteo Bonicolini 
 * LGPL V3.0
 * 
 * Print an error msg for invalid path and exit with return status 1
 * */
static void no_in_db_error(struct duc *duc,const char *path)
{
	if(duc_error(duc) == DUC_E_PATH_NOT_FOUND) 
	{
		if(!is_file(path))
			duc_log(duc, DUC_LOG_FTL, "The requested path '%s' was not found in the database,", path);
		else
			duc_log(duc, DUC_LOG_FTL, "The requested file '%s' was not found in the database,", path);
			
		duc_log(duc, DUC_LOG_FTL, "Please run 'duc info' for a list of available directories.");
	} 
	else 
		duc_log(duc, DUC_LOG_FTL, "%s", duc_strerror(duc));
		
	exit(1);
}


struct inode
{
        struct inode* next;
        char * item;
};

typedef struct inode node;

static node* newNode()
{
	node *ret=NULL;
	
	ret=(node*) malloc(sizeof(node));
	if(ret==NULL) return NULL;
	
	ret->next=NULL;
	ret->item=NULL;
	
	return ret;
}


static int isNULLnode(node **p)
{
	if(p==NULL || *p==NULL)
		return 1;
	else
		return 0;
}

static int emptyNode(node **p)
{

	if((*p)->next==NULL && (*p)->item==NULL)
		return 1;
	else
		return 0;
	
}

static int addFIFO(node **tail,char*item)
{
	node *tmp=NULL;
	node *p=NULL;
	
	if(isNULLnode(tail))
		return 1;
	
	if(emptyNode(tail))
	{
		(*tail)->item=item;
		(*tail)->next=*tail;
	}
	else
	{			
		tmp=*tail;
		p=newNode();
		p->item=item;
		p->next=tmp->next;
		tmp->next=p;
		(*tail)=p;
	}
	
	return 0;
}

static void* delFIFO(node **tail)
{
	node *tmp=NULL;
	void *ret=NULL;
	
	if(isNULLnode(tail)) 
		return NULL;
	
	if(emptyNode(tail))
	{
		free(*tail);
		*tail=NULL;
		return NULL;
	}
	tmp=(*tail)->next;
	tmp=(*tail)->next;
	if(tmp==*tail)
		*tail=NULL;
	else
		(*tail)->next=tmp->next;
		
	tmp->next=NULL;
	ret=tmp->item;
	tmp->item=NULL;
	free(tmp);
	
	return ret;
}

node *path_error_queue=NULL;


/* modified version of auxiliary function added by Matteo Bonicolini 
 * LGPL V3.0
 * 
 * It is the core of the cmd-ls*/
static void do_one(struct duc *duc, const char *path)
{
	char *name=NULL;
	char *parent=NULL;
	struct stat s;
	uint32_t index=0;
	const char *p;
	uint32_t i=0;
	
	duc_dir *dir = duc_dir_open(duc, path);
	if(dir == NULL && !is_file(path))
	{
		addFIFO(&path_error_queue,(char*)path);
		return;
	}
	
	if(is_file(path))
	{	
		p=&path[0];
		for(i=0;i<strlen(path);i++)
		{
			if(*p=='/')
				index=i;
			p++;
		}
		parent=malloc(index+1);
		memset(parent,0x0,index+1);
		memcpy(parent,path,index);
		if((dir=duc_dir_open(duc, parent))==NULL)
			no_in_db_error(duc,path);
		
		name=strdup(path);
		name=basename(name);
		if(name==NULL)
		{
			free(parent);
			printf("parent check failed\n");
			exit(1);
		}

		memset(&s,0x0,sizeof(struct stat));
		stat(path,&s);
		ls_one_file(dir,name,parent,index,s.st_size);
		duc_dir_close(dir);
		free(parent);
		return;
		
	}

	if(opt_directory) {
		ls_dir_only(path, dir);
	} else {
		ls_one(dir, 0, 0);
	}

	duc_dir_close(dir);
}


static int ls_main(duc *duc, int argc, char **argv)
{
	/* Get terminal width */
	char *path_error_elem=NULL;
#ifdef TIOCGWINSZ
	if(isatty(STDOUT_FILENO)) {
		struct winsize w;
		int r = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		if(r == 0) width = w.ws_col;
	}
#endif

	/* Set locale for wide string calculations */

	setlocale(LC_ALL, "");

	/* Disable color if output is not a tty */

	if(!isatty(1)) {
		opt_color = 0;
	}

	/* Disable graph when --full-path is requested, since there is no good
	 * way to render this */

	if(opt_full_path) {
		opt_graph = 0;
	}

	/* Open database */

	int r = duc_open(duc, opt_database, DUC_OPEN_RO);
	if(r != DUC_OK) {
		return -1;
	}
	
	path_error_queue=newNode();
	if(argc > 0) {
		int i;
		for(i=0; i<argc; i++) {
			do_one(duc, argv[i]);
		}
	} else {
		do_one(duc, ".");
	}
	
	if(isNULLnode(&path_error_queue))
	{
		duc_close(duc);
		return 0;
	}
	
	while(!emptyNode(&path_error_queue))
	{
		path_error_elem=delFIFO(&path_error_queue);
		if(!is_file(path_error_elem))
			printf("The requested path '%s' was not found in the database,\n", path_error_elem);
		else
			printf("The requested file '%s' was not found in the database,\n", path_error_elem);
			
		printf("Please run 'duc info' for a list of available directories.\n");
		fflush(stdout);
	}
	duc_close(duc);

	return 0;
}


static struct ducrc_option options[] = {
	{ &opt_apparent,  "apparent",  'a', DUCRC_TYPE_BOOL,   "show apparent instead of actual file size" },
	{ &opt_ascii,     "ascii",      0,  DUCRC_TYPE_BOOL,   "use ASCII characters instead of UTF-8 to draw tree" },
	{ &opt_bytes,     "bytes",     'b', DUCRC_TYPE_BOOL,   "show file size in exact number of bytes" },
	{ &opt_classify,  "classify",  'F', DUCRC_TYPE_BOOL,   "append file type indicator (one of */) to entries" },
	{ &opt_color,     "color",     'c', DUCRC_TYPE_BOOL,   "colorize output (only on ttys)" },
	{ &opt_count,     "count",      0,  DUCRC_TYPE_BOOL,   "show number of files instead of file size" },
	{ &opt_database,  "database",  'd', DUCRC_TYPE_STRING, "select database file to use [~/.duc.db]" },
	{ &opt_directory, "directory", 'D', DUCRC_TYPE_BOOL,   "show path , not its contents" },
	{ &opt_dirs_only, "dirs-only",  0,  DUCRC_TYPE_BOOL,   "list only directories, skip individual files" },
	{ &opt_full_path, "full-path",  0,  DUCRC_TYPE_BOOL,   "show full path instead of tree in recursive view" },
	{ &opt_graph,     "graph",     'g', DUCRC_TYPE_BOOL,   "draw graph with relative size for each entry" },
	{ &opt_levels,    "levels",    'l', DUCRC_TYPE_INT,    "traverse up to ARG levels deep [4]" },
	{ &opt_name_sort, "name-sort", 'n', DUCRC_TYPE_BOOL,   "sort output by name instead of by size" },
	{ &opt_recursive, "recursive", 'R', DUCRC_TYPE_BOOL,   "recursively list subdirectories" },
	{ NULL }
};

struct cmd cmd_ls = {
	.name = "ls",
	.descr_short = "List sizes of directory",
	.usage = "[options] [PATH]...",
	.main = ls_main,
	.options = options,
	.descr_long = 
		"The 'ls' subcommand queries the duc database and lists the inclusive size of\n"
		"all files and directories on the given path. If no path is given the current\n"
		"working directory is listed.\n"
};


/*
 * End
 */

