CC = gcc
CFLAGS = -Wall -g # adicione aqui flags de compilação
SRCDIR = src
OBJDIR = obj

# Encontra todos os arquivos .c no diretório src e cria a lista correspondente de arquivos .o
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

# Compila o programa
all: $(OBJS)
	$(CC) $(CFLAGS) -o fsh $(OBJS) 

# Cria o diretório obj
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Compila os arquivos .c para .o
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) fsh
