// Vicente González Morales y Álvaro Cutillas Florido. Grupo 3.3
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_BUF_SIZE 8192
#define MIN_BUF_SIZE 1
#define DEFAULT_BUF_SIZE 16
#define MAX_LINE_SIZE 1024
#define DEFAULT_LINE_SIZE 32
#define MIN_LINE_SIZE 16

#define FILETYPE_IN 1
#define FILETYPE_OUT 0

#define SEPARATOR " "
#define CONTROL_CHAR '\n'

void print_help(char *nombre_programa);
void parse_line(char *linea, int line_size, int num_line);
void execute_command(char **tokens, int num_line);
void normal_command(char *bin, char **args, int num_line);
void redir_command(char *bin, char **args, char *file_name, int fd, int trunc, int type, int num_line);
void pipe_command(char **args, int pipe_pos, int num_line);

int main(int argc, char *argv[])
{
    int opt;
    int buf_size = DEFAULT_BUF_SIZE;
    int line_size = DEFAULT_LINE_SIZE;

    optind = 1;
    while ((opt = getopt(argc, argv, "l:b:h")) != -1)
    {
        switch (opt)
        {
        case 'b':
            buf_size = atoi(optarg);
            break;
        case 'l':
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

    // Comprobamos que el tamaño de buffer introducido esté dentro de los límites
    if (buf_size < MIN_BUF_SIZE || buf_size > MAX_BUF_SIZE)
    {
        fprintf(stderr, "Error: El tamaño de buffer tiene que estar entre 1 y 8192.\n");
        exit(EXIT_FAILURE);
    }

    // Comprobamos que el tamaño de línea introducido esté dentro de los límites
    if (line_size < MIN_LINE_SIZE || line_size > MAX_LINE_SIZE)
    {
        fprintf(stderr, "Error: El tamaño mñximo de linea tiene que estar entre 16 y 1024.\n");
        exit(EXIT_FAILURE);
    }

    // Reservamos memoria para el buffer de lectura
    char *buffer;
    if ((buffer = malloc(buf_size * sizeof(char))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    char line_buf[line_size + 1];   // Array para guardar la línea leída
    int cont = 0;   // Contador de letras que llevamos leídas
    int num_line = 1;
    ssize_t num_read;

    while ((num_read = read(STDIN_FILENO, buffer, buf_size)) > 0)
    {
        for (ssize_t i = 0; i < num_read; i++)
        {
            if (buffer[i] == CONTROL_CHAR)
            {
                line_buf[cont] = '\0';
                parse_line(line_buf, line_size, num_line);

                cont = 0;
                num_line++;
            }
            else
            {
                line_buf[cont] = buffer[i];
                cont++;
                if (cont > line_size)
                {
                    line_buf[cont] = '\0';
                    fprintf(stderr, "Error, línea %d demasiado larga: \"%s...\"\n", num_line, line_buf);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    // Comprobamos si queda alguna línea por procesar
    if (cont > 0) 
    {
        line_buf[cont] = '\0';
        parse_line(line_buf, line_size, num_line);
    }

    free(buffer);

    return EXIT_SUCCESS;
}

void print_help(char *nombre_programa)
{
    fprintf(stderr, "Uso: %s [-b BUF_SIZE] [-l MAX_LINE_SIZE]\n", nombre_programa);
    fprintf(stderr, "Lee de la entrada estándar una secuencia de líneas conteniendo órdenes\npara ser ejecutadas y lanza los procesos necesarios para ejecutar cada línea, esperando a su terminación para ejecutar la siguiente.\n");
    fprintf(stderr, "-b BUF_SIZE\t\tTamaño del buffer de entrada 1<=BUF_SIZE<=8192\n");
    fprintf(stderr, "-l MAX_LINE_SIZE\tTamaño máximo de línea 16<=MAX_LINE_SIZE<=1024\n");
}

void parse_line(char *linea, int line_size, int num_line)
{
    // Creamos la lista de tokens
    int max_tokens = line_size / 2 + 1;
    char **tokens = malloc(max_tokens * sizeof(char *));
    if (tokens == NULL) 
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    int i = 0;

    // Leemos los tokens de la línea y los almacenamos en la lista 
    char *saveptr;
    char *token = strtok_r(linea, SEPARATOR, &saveptr);
    while (token && i < max_tokens - 1)
    {
        tokens[i] = token;
        token = strtok_r(NULL, SEPARATOR, &saveptr);
        i++;
    }
    tokens[i] = NULL;

    if (!tokens[0])
    {
        free(tokens);
        return;
    }

    // Comprobamos el número de operadores que tiene la línea y si hay una tubería
    int num_operators = 0;
    int pipe_pos = -1;
    for (int j = 0; tokens[j]; j++)
    {
        if (strcmp(tokens[j], "<") == 0 || strcmp(tokens[j], ">") == 0 ||
            strcmp(tokens[j], ">>") == 0 || strcmp(tokens[j], "|") == 0)
        {
            num_operators++;
            if (strcmp(tokens[j], "|") == 0)
                pipe_pos = j;
        }
    }

    if (num_operators > 1)
    {
        fprintf(stderr, "Error, línea %d tiene más de un operando de redirección o tubería\n", num_line);
        free(tokens);
        exit(EXIT_FAILURE);
    }

    if (pipe_pos != -1)
    {
        pipe_command(tokens, pipe_pos, num_line);
    }
    else
    {
        execute_command(tokens, num_line);
    }

    free(tokens);
}

void execute_command(char **args, int num_line)
{
    char *bin = args[0];
    for (int i = 0; args[i]; i++)
    {
        if (strcmp(args[i], "<") == 0)
        {
            args[i] = NULL;
            redir_command(bin, args, args[i + 1], STDIN_FILENO, 1, FILETYPE_IN, num_line);
            return;
        }
        else if (strcmp(args[i], ">") == 0)
        {
            args[i] = NULL;
            redir_command(bin, args, args[i + 1], STDOUT_FILENO, 1, FILETYPE_OUT, num_line);
            return;
        }
        else if (strcmp(args[i], ">>") == 0)
        {
            args[i] = NULL;
            redir_command(bin, args, args[i + 1], STDOUT_FILENO, 0, FILETYPE_OUT, num_line);
            return;
        }
    }

    normal_command(bin, args, num_line);
}

void normal_command(char *bin, char **args, int num_line)
{
    int status;
    pid_t pid = fork();

    switch (pid)
    {
        case -1:
            perror("fork()");
            exit(EXIT_FAILURE);
            break;
            
        case 0: // Estamos en el hijo
            close(STDERR_FILENO);   // Cerramos la salida de error para que no informe de errores

            execvp(bin, args);
            perror("execvp()"); // Si todo va bien no deberíamos ejecutar esto
            exit(EXIT_FAILURE);
            break;
            
        default:
            // Esperamos a que acabe el hijo y recuperamos su status de salida
            if (waitpid(pid, &status, 0) == -1)
            {
                perror("wait()");
                exit(EXIT_FAILURE);
            }
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            {
                fprintf(stderr, "Error al ejecutar la línea %d. Terminación normal con código %d.\n", num_line, WEXITSTATUS(status));
                //exit(EXIT_FAILURE);
            }
            else if (WIFSIGNALED(status))
            {
                fprintf(stderr, "Error al ejecutar la línea %d. Terminación anormal por señal %d.\n", num_line, WTERMSIG(status));
                //exit(EXIT_FAILURE);
            }
            break;
    }
}

// type : tipo de fichero. 1 para entrada, 0 para salida.
// También se pueden usar las macros FILETYPE_IN y FILETYPE_OUT.
void redir_command(char *bin, char **args, char *file_name, int fd, int trunc, int type, int num_line)
{
    pid_t pid = fork();
    int status;

    switch (pid)
    {
        case -1:
            perror("fork()");
            exit(EXIT_FAILURE);
            break;

        case 0: // Estamos en el hijo
        {
            close(STDERR_FILENO);   // Cerramos la salida de error para que no informe de errores

            // Comprobamos el tipo de fichero
            int newfd;
            if (type == FILETYPE_OUT)
            {
                int flags = trunc ? (O_WRONLY | O_CREAT | O_TRUNC) : (O_WRONLY | O_CREAT | O_APPEND);
                newfd = open(file_name, flags, S_IWUSR | S_IRUSR);
            }
            else
            {
                newfd = open(file_name, O_RDONLY);
            }

            if (newfd == -1)
            {
                perror("open()");
                exit(EXIT_FAILURE);
            }

            if (dup2(newfd, fd) == -1)
            {
                perror("dup2()");
                exit(EXIT_FAILURE);
            }

            close(newfd);
            execvp(bin, args);
            perror("execvp()");
            exit(EXIT_FAILURE);
            break;
        }

        default:
            // Esperamos a que acabe el hijo y recuperamos su status de salida
            if (waitpid(pid, &status, 0) == -1)
            {
                perror("wait()");
                exit(EXIT_FAILURE);
            }
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
            {
                fprintf(stderr, "Error al ejecutar la línea %d. Terminación normal con código %d.\n", num_line, WEXITSTATUS(status));
                //exit(EXIT_FAILURE);
            }
            else if (WIFSIGNALED(status))
            {
                fprintf(stderr, "Error al ejecutar la línea %d. Terminación anormal por señal %d.\n", num_line, WTERMSIG(status));
                //exit(EXIT_FAILURE);
            }
            break;
    }

}

void pipe_command(char **args, int pipe_pos, int num_line)
{
    args[pipe_pos] = NULL;
    char **left = args;
    char **right = &args[pipe_pos + 1];
    int pipefds[2];
    int status1, status2;

    if (pipe(pipefds) == -1)
    {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }

    // Hijo de la izquierda
    pid_t pid1 = fork();
    switch (pid1)
    {
        case -1:
            perror("fork()");
            exit(EXIT_FAILURE);
            break;

        case 0:
            close(STDERR_FILENO);   // Cerramos la salida de error para que no informe de errores
            
            // Redirigimos la salida estándar hacia la tubería
            dup2(pipefds[1], STDOUT_FILENO);
            close(pipefds[0]);
            close(pipefds[1]);

            execvp(left[0], left);
            perror("execvp(left)");
            exit(EXIT_FAILURE);
            break;
        
        default:
            break;  // El padre continua
    }

    // Hijo de la derecha
    pid_t pid2 = fork();
    switch (pid2)
    {
        case -1:
            perror("fork()");
            exit(EXIT_FAILURE);
            break;

        case 0:
            close(STDERR_FILENO);   // Cerramos la salida de error para que no informe de errores

            // Redirigimos la entrada estándar desde la tubería
            dup2(pipefds[0], STDIN_FILENO);
            close(pipefds[0]);
            close(pipefds[1]);

            execvp(right[0], right);
            perror("execvp(right)");
            exit(EXIT_FAILURE);
            break;

        default:
            break;  // El padre continua
    }

    // Cerramos los descriptores de la tubería en el padre
    close(pipefds[0]);
    close(pipefds[1]);

    // Esperamos a que acaben los hijos y recuperamos sus status de salida
    if (waitpid(pid1, &status1, 0) == -1)
    {
        perror("wait()");
        exit(EXIT_FAILURE);
    }

    if (waitpid(pid2, &status2, 0) == -1)
    {
        perror("wait()");
        exit(EXIT_FAILURE);
    }

    if (WIFEXITED(status1) && WEXITSTATUS(status1) != 0)
    {
        fprintf(stderr, "Error al ejecutar la línea %d. Terminación normal con código %d.\n", num_line, WEXITSTATUS(status1));
        //exit(EXIT_FAILURE);
    }
    else if (WIFSIGNALED(status1))
    {
        fprintf(stderr, "Error al ejecutar la línea %d. Terminación anormal por señal %d.\n", num_line, WTERMSIG(status1));
        //exit(EXIT_FAILURE);
    }

    if (WIFEXITED(status2) && WEXITSTATUS(status2) != 0)
    {
        fprintf(stderr, "Error al ejecutar la línea %d. Terminación normal con código %d.\n", num_line, WEXITSTATUS(status2));
        //exit(EXIT_FAILURE);
    }
    else if (WIFSIGNALED(status2))
    {
        fprintf(stderr, "Error al ejecutar la línea %d. Terminación anormal con por señal %d.\n", num_line, WTERMSIG(status2));
        //exit(EXIT_FAILURE);
    }

}
