//Vicente Gonzalez Morales y Alvaro Cutillas Florido Subgrupo 3.3

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

#define MAX_BUF_SIZE 8192
#define MIN_BUF_SIZE 1
#define DEFAULT_BUF_SIZE 16
#define MAX_LINE_SIZE 1024
#define DEFAULT_LINE_SIZE 32
#define MIN_LINE_SIZE 16

#define SEPARATOR " "
#define CONTROL_CHAR "\n"



void print_help(char* nombre_programa);

typedef struct line *line_t;
void execute_line(line_t line);


int main(int argc, char *argv[]) 
{
    int opt;
    int buf_size = DEFAULT_BUF_SIZE;
    int line_size = DEFAULT_LINE_SIZE;
    int line;

    optind = 1;
    while ((opt = getopt(argc, argv, "l:b:h")) != -1)
    {
        switch (opt)
        {
        case 'b':
            buf_size = -1;
            buf_size = atoi(optarg);
            break;
		case 'l':
            line_size = -1;
			line_size = atoi(optarg);
			break;
        case 'h':
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        default:
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if(buf_size == -1)
    {
        fprintf(stderr, "%s: option requires an argument -- 'b'\n", argv[0]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    if(line_size == -1)
    {
        fprintf(stderr, "%s: option requires an argument -- 'l'\n", argv[0]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((buf_size < MIN_BUF_SIZE) || (buf_size > MAX_BUF_SIZE)) 
    {
        fprintf(stderr, "Error: El tamaño de buffer tiene que estar entre 1 y 8192.\n");
        fprintf(stderr, "%s: option requires an argument -- 'b'\n", argv[0]);
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

	if ((line_size < MIN_LINE_SIZE) || (line_size > MAX_LINE_SIZE)){
		fprintf(stderr, "Error: El tamaño máximo de linea tiene que estar entre 16 y 1024.\n");
		fprintf(stderr, "%s: option requires an argument -- 'l'\n", argv[0]);
		print_help(argv[0]);
		exit(EXIT_FAILURE);
	}

    //--------------------------------------------------------------//
    ssize_t num_read;
    ssize_t bytesTotal = 0;
    char *buffer;
    char *bufferComando;
    int num_line = 1;
    int cont = 0;
    line_t linea;

    /* Reserva memoria dinámica para buffer de lectura */
    if ((buffer = malloc(buf_size * sizeof(char))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    if ((bufferComando = malloc(line_size * sizeof(char))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    while ((num_read = read(STDIN_FILENO, buffer, buf_size)) > 0)
    {
        cont += num_read;
        if(cont > line_size)
        {
            fprintf(stderr, "Error: línea %d demasiado larga: \n", num_line);
            exit(EXIT_FAILURE);
        }

        for (ssize_t i = 0; i < num_read; i++) 
        {
            if (buffer[i] == CONTROL_CHAR) 
            {
                bufferComando[cont] = '\0';
                linea->number = num_line;
                linea->max_size = (line_size/2) + 1;
                linea->command = bufferComando;
                execute_line(linea);
                cont = 0;
            } 
            else
            {
                bufferComando[cont] = buffer[i];
                cont++;
            }
        }

        num_line++;
    }

    printf("Size of buffer: %d\n", buf_size);
    printf("Max. size of line: %d\n", line_size);

}

struct line 
{
    int number;
    int max_size;
    char *command;
};

void print_help(char* nombre_programa)
{
	fprintf(stderr, "Uso: %s [-b BUF_SIZE] [-l MAX_LINE_SIZE]\nLee de la entrada estándar una secuencia de líneas conteniendo órdenes\npara ser ejecutadas y lanza los procesosnecesarios para ejecutar cada línea, esperando a su terminacion para ejecutar la siguiente.\n-b BUF_SIZE\tTamaño del buffer de entrada 1<=BUF_SIZE<=8192\n-l MAX_LINE_SIZE\tTamaño máximo de línea 16<=MAX_LINE_SIZE<=1024\n", nombre_programa);
}

void execute_line(line_t line)
{
    char **tokens = malloc(line->max_size * sizeof(char *));
    int i = 0;

    char *saveptr;
    char *token_read = strtok_r(line->command, SEPARATOR, &saveptr);
    while(token_read != NULL) {
        tokens[i] = token_read;
        i++;
        token_read = strtok_r(NULL, SEPARATOR, &saveptr);
    }

    tokens[i] = NULL;

    i = 0;
    
}