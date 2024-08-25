CC = gcc
CFLAGS = -Wall -g # adicione aqui flags de compilação
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# Adicione aqui os arquivos .c dentro da pasta src
OBJS = $(OBJDIR)/main.o

all: $(BINDIR)/fsh

# Cria o executável
$(BINDIR)/fsh: $(OBJS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $(BINDIR)/fsh $(OBJS)

# Cria o diretório bin
$(BINDIR):
	mkdir -p $(BINDIR)

# Cria o diretório obj
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Compila
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

run: 
	./$(BINDIR)/fsh

clean:
	rm -rf $(BINDIR) $(OBJDIR)
