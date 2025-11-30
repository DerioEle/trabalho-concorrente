# 🍔 Sistema Concorrente de Gerenciamento de Pedidos

Projeto desenvolvido para a disciplina de Sistemas Eletrônicos de Tempo-Real.  
O objetivo é demonstrar, na prática, o domínio de **threads**, **processos**, **IPC POSIX**, **sincronização** e **modelos clássicos de concorrência**, aplicados em um sistema realista de cozinha/restaurante.

---

## 📌 Objetivo do Projeto

Criar uma aplicação concorrente composta por múltiplos processos e threads que simulam:

- **Clientes** enviando pedidos ao servidor de forma concorrente  
- **Servidor** recebendo pedidos através de uma fila POSIX  
- **Cozinheiros (threads)** processando pedidos em paralelo  
- **Painel** monitorando o status através de memória compartilhada  

O projeto demonstra **todos os principais conceitos da disciplina**, incluindo processos, threads, mutexes, semáforos, variáveis de condição, filas de mensagens POSIX e memória compartilhada.

---

## 🛠 Como Compilar

```bash
make clean
make
```

---

## ▶️ Como Executar

### Servidor
```bash
./servidor
```

### Painel
```bash
./painel
```

### Cliente
```bash
./cliente "Pizza"
```

### Launcher
```bash
./launcher 5
```

---

## 🔧 Mecanismos Utilizados

- Threads POSIX  
- Mutexes e Condition Variables  
- Semáforos POSIX  
- Fila de Mensagens POSIX  
- Memória Compartilhada  
- Sinais (SIGINT)  
- Modelo Produtor–Consumidor  

---

## 👥 Integrantes

- Dério Crisóstomo Oliveira  
- Laura Nunes Belém  
- Bruno Henrique Peres Silva  
