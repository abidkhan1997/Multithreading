// C program to demonstrate use of fork() and pipe()

//#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
////#include<vector>
//#include <filesystem>
//#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
//#include <math.h>
#include <stdbool.h>
#include <sys/utsname.h>
#define MAX_MEM 524288000 //500mb
#define EOF_MARKER "$EOF$"
#define MAX_FILE_NO 50
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int fd1[2]; // Used to store two ends of first pipe
int fd2[2]; // Used to store two ends of second pipe

int filedes[2];

int total_file = 0;
char input_files[MAX_FILE_NO][200];

//vector<string> files;
int buffersize = 32;

char *recv_buff;
int recv_size = 0;

struct Result {
	int chars;
	int word;
	int lines;
	int maxLines;
};

struct Filter {
	bool n;
	bool c;
	bool b;
	bool m;
};

struct Result res[MAX_FILE_NO];
struct Filter filter;

int find_max_lines(const char *sentence, int size) {
	int i = 0;
	int maxSize = 0;
	int cur_size = 0;
	while (i < size) {
		char ch = sentence[i];
		i++;
		if (ch == '\n') {
			maxSize = MAX(maxSize, cur_size);
			cur_size = 0;
			continue;
		}
		cur_size++;
	}
	return maxSize;
}

int words(const char *sentence, int size) {
	int counted = 0; // result
	// state:
	const char *it = sentence;
	int inword = 0;

	int i = 0;
	do {
		char ch = sentence[i];
		i++;

		switch (ch) {
		case '\0':
		case ' ':
		case '\t':
		case '\n':
		case '\r': // TODO others?
			if (inword) {
				inword = 0;
				counted++;
			}
			break;
		default:
			inword = 1;
		}
	} while (i < size);

	return counted;
}

int count_char(const char *s, int size) {
	int total = 0;
	char ch;
	int i = 0;
	int chars = 0;
	while (i < size) {

		ch = s[i];
//		printf("%c", ch);
		i++;
		if (ch == '\r')
			continue;
		if (ch == '\n') {
//			res.lines++;
			continue;
		}

		chars++;
	}

	return chars;
}

int count_lines(const char *s, int size) {
	int total = 0;
	char ch;
	int i = 0;
	int lines = 0;
	while (i < size) {
		ch = s[i];
		i++;

		if (ch == '\n') {
			lines++;
		}
	}

	return lines + 1;
}

void process_result() {
	char *start = recv_buff;
	int cur_size = 0;

	char *pch = strstr(start, EOF_MARKER);

	int i = 0;
	while (pch) {
		cur_size = pch - start;
		int chars = count_char(start, cur_size);
		int lines = count_lines(start, cur_size);
		int wrd = words(start, cur_size);
		int maxLines = find_max_lines(start, cur_size);

		start = pch + 5;
		pch = strstr(start, EOF_MARKER);

		res[i].chars = chars;
		res[i].lines = lines;
		res[i].word = wrd;
		res[i].maxLines = maxLines;
		i++;
	}
}

void show_result() {

	for (int i = 0; i < total_file; i++) {

		if (filter.n)
			printf("\nINPUT_FILE:\t%s newline count is:\t%d", input_files[i],
					res[i].lines);

		if (filter.c)
			printf("\nINPUT_FILE:\t%s word count is:\t%d", input_files[i],
					res[i].word);

		if (filter.b)
			printf("\nINPUT_FILE:\t%s character count is:\t%d", input_files[i],
					res[i].chars);

		if (filter.m)
			printf("\nINPUT_FILE:\t%s maximum line length is:\t%d\n",
					input_files[i], res[i].maxLines);
	}

	printf("\n");
}

void reader_process() {

	usleep(2000);

	int file_id = 0;

	/* Parent process closes up output side of pipe */
	close(filedes[1]); //Parent process does not need this end of the pipe

	int count = 0;

	char readbuffer[buffersize + 5];
	int nbytes = 0;
	while (0 < (nbytes = read(filedes[0], readbuffer, buffersize))) {
		strncpy(recv_buff + recv_size, readbuffer, nbytes);
		recv_size += nbytes;

	}

	process_result();
	show_result();

}

char* get_file_content(char *filename, int *file_size) {
	FILE *f = fopen(filename, "r");

	// Determine file size
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);

	char *where = (char*) malloc(size);

	rewind(f);
	fread(where, sizeof(char), size, f);
	*file_size = size;

//	string data = string(where);
//
//	delete[] where;

	return where;
}

void writer_process() {

//	string dd = "";

//	char string[] = "Hello, world!\n";

	close(filedes[0]); //Child process does not need this end of the pipe

	for (int i = 0; i < total_file; i++) {

		int size = 0;
		char *dd = get_file_content(input_files[i], &size);
//		cout<<"writing contents : "<<dd<<"\n";
//		write(filedes[1], string, (strlen(string) + 1));

		for (int j = 0; j < size; j += buffersize) {
			int sz = MIN(buffersize, size - j);
//			string chunk = dd.substr(j, sz);
			write(filedes[1], dd + j, sz);
		}

		write(filedes[1], "$EOF$", 5);

	}
	exit(0);
}

void set_default_filter() {
	filter.n = true;
	filter.c = true;
	filter.b = true;
}

void set_filter(const char *s) {
	int i = 0;
	while ((*s) != '\0') {
		char ch = *s;
		switch (ch) {
		case 'n':
			filter.n = true;
			break;
		case 'c':
			filter.c = true;
			break;
		case 'b':
			filter.b = true;
			break;
		case 'm':
			filter.m = true;
			break;
		default:
			break;

		}

		s++;
	}

}

void init() {

	printf("System info\n");
	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);
	printf("Hostname is:\t %s\n", hostname);

	struct utsname unameData;
	uname(&unameData);
	printf("OS name is:\t %s \n", unameData.sysname);
	printf("OS release is:\t %s \n", unameData.release);
	printf("OS version is:\t %s \n", unameData.version);
	printf("\n\n");

	recv_buff = (char*) malloc(MAX_MEM);
	for (int i = 0; i < MAX_FILE_NO; i++) {
		res[i].chars = 0;
		res[i].word = 0;
		res[i].lines = 0;
		res[i].maxLines = 0;
	}

	filter.n = false;
	filter.c = false;
	filter.b = false;
	filter.m = false;

}

int main(int argc, char *argv[]) {

	init();
//	cout << argc << " " << argv << "\n";

//	printf("%s", argv[1]);

	if (argv[1][0] == '-') {
		set_filter(argv[1]);
		buffersize = atoi(argv[2]);
		for (int i = 3; i < argc; i++) {
			strcpy(input_files[i - 3], argv[i]);
			total_file++;
		}
	} else {
		set_default_filter();
		buffersize = atoi(argv[1]);
		for (int i = 2; i < argc; i++) {
			strcpy(input_files[i - 2], argv[i]);
			total_file++;
		}
	}

	if (total_file == 0) {
		total_file = 1;
		strcpy((char*) input_files[0], "myinpfile.txt");
	}

//	cout << options << " " << buffersize << " " << files[0] << "\n";

	pipe(filedes);

	pid_t p;
	p = fork();

	if (p < 0) {
		fprintf(stderr, "fork Failed");
		return 1;
	}

	// child process
	else if (p == 0) {

		printf("Process ID is:\t %d \n", getpid());
		printf("Parent Process ID is:\t %d \n", getppid());

		char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd)) != NULL) {

			printf("Process current working directory is:\t%s\n", cwd);
		} else {
			perror("getcwd() error");
			return 1;
		}

		writer_process();
	}

	//
	else {
		printf("Process ID is:\t %d \n", getpid());
		printf("Parent Process ID is:\t %d \n", getppid());

		char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd)) != NULL) {

			printf("Process current working directory is:\t%s\n", cwd);
		} else {
			perror("getcwd() error");
			return 1;
		}

		reader_process();
	}
}
