# 📐 Arquitetura do Sistema Concorrente de Gerenciamento de Pedidos

Este documento descreve a arquitetura completa do projeto, cobrindo processos, threads, IPC, sincronização e fluxo lógico da aplicação.  
É uma versão detalhada, ideal para documentação final e apresentação.

---

# 🏗 Visão Geral da Arquitetura

O sistema é composto por **4 componentes principais**, cada um com responsabilidades bem definidas:

```
+------------------------+      +-------------------------+
|       CLIENTES         |      |        LAUNCHER         |
|   (Processos externos) |      |   (fork/exec/wait)      |
+-----------+------------+      +-----------+-------------+
            |                                |
            | mq_send                        |
            v                                v
+---------------------------------------------------------+
|                       SERVIDOR                          |
|            (Processo principal do sistema)              |
|                                                         |
| +----------- Comunicação IPC via MQ ------------------+ |
| | mq_receive -> recebe pedidos dos clientes           | |
| +-----------------------------------------------------+ |
|                                                         |
| +-------------- Fila Interna (Produtor) -------------+ |
| | Buffer Circular: fila[], in, out, count            | |
| | Proteção: mutex + condvar + sem_itens + sem_espacos| |
| +-----------------------------------------------------+ |
|                                                         |
| +---------------- Threads Cozinheiras ---------------+ |
| | pthreads executando consumidores                   | |
| | Retiram pedidos -> Preparam -> Atualizam painel    | |
| +-----------------------------------------------------+ |
|                                                         |
| +---------------- Painel (Shared Memory) -------------+ |
| | PainelStatus { total, em_preparo, finalizados }     | |
| | shm_open + mmap                                      | |
| | mutex para escrita                                  | |
| +-----------------------------------------------------+ |
+---------------------------------------------------------+

               |
               | memória compartilhada
               v
+---------------------------------------------------------+
|                         PAINEL                          |
|              (Processo externo de leitura)              |
|   Lê o PainelStatus atualizado e exibe em tempo real    |
+---------------------------------------------------------+
```

---

# 🧩 Componentes em Detalhes

## 1️⃣ **Clientes**
- São executáveis independentes.
- Enviam pedidos ao servidor usando **POSIX Message Queue (mq_send)**.
- Cada execução representa um processo distinto.

Exemplo:  
```bash
./cliente "Pizza"
./cliente "Hamburguer"
```

---

## 2️⃣ **Launcher (Processos Concorrentes)**
Responsável por demonstrar uso de:
- `fork()`
- `exec()`
- `wait()`

Ele cria vários processos filhos que executam o programa cliente para gerar carga concorrente automaticamente.

Fluxo:

```
launcher
 ├─ fork → cliente "Pedido X"
 ├─ fork → cliente "Pedido Y"
 └─ fork → cliente "Pedido Z"
```

---

## 3️⃣ **Servidor (processo principal)**

O servidor é o núcleo da aplicação.  
Possui 3 subsistemas:

---

### 🔶 **A. Sistema de Comunicação (Message Queue)**

O servidor recebe pedidos usando:

- `mq_open`
- `mq_receive`
- `mq_close`
- `mq_unlink`

Fluxo:

```
cliente → mq_send → servidor → inserir na fila interna
```

---

### 🔶 **B. Buffer Interno (Produtor–Consumidor)**

Implementado com:

- **Buffer circular**
- **Mutex** (`pthread_mutex_t mutex_fila`)
- **Condition Variable** (`pthread_cond_t cond_fila`)
- **Semáforos POSIX**:
  - `sem_itens` (quantos pedidos na fila)
  - `sem_espacos` (quantos espaços disponíveis)

Responsabilidades:
- Garantir **exclusão mútua** no acesso à fila.
- Impedir overflow/underflow.
- Coordenar produtores e consumidores.

```
Servidor (produtor):
    sem_wait(espacos)
    mutex -> insere pedido -> mutex
    sem_post(itens)

Cozinheiros (consumidores):
    sem_wait(itens)
    mutex -> retira pedido -> mutex
    sem_post(espacos)
```

---

### 🔶 **C. Threads Cozinheiras**

Criadas com `pthread_create`.

Cada thread:

1. Espera pedido disponível  
2. Retira do buffer interno  
3. Atualiza painel (shared memory)  
4. Simula preparo  
5. Atualiza painel novamente  

Número de threads configurável via `#define NUM_COZINHEIROS`.

---

## 4️⃣ **Painel (Leitor de Memória Compartilhada)**

Processo separado.

Usa:
- `shm_open`
- `mmap`
- `PROT_READ`
- Atualização a cada 1 segundo

Ele não interfere no servidor — apenas **lê** os dados do painel.

Mostra:

- Total de pedidos recebidos
- Pedidos em preparo
- Pedidos finalizados

---

# 🔐 Sincronização: Mecanismos Utilizados

| Mecanismo | Local | Função |
|----------|-------|--------|
| `pthread_mutex` | servidor (fila/painel) | Exclusão mútua |
| `pthread_cond` | servidor (fila) | Bloquear até condição ser atendida |
| `sem_init` | servidor | Controlar contagem de itens/espaços |
| `mq_open` / `mq_send` | cliente → servidor | Comunicação entre processos |
| `shm_open` / `mmap` | servidor ↔ painel | Compartilhar estado global |
| `fork/exec/wait` | launcher | Criar diversos clientes concorrentes |
| `signal(SIGINT)` | servidor | Finalização limpa |
| `mq_unlink`, `shm_unlink` | servidor | Cleanup de IPC |

---

# 🔄 Fluxo Completo da Aplicação

```
1. Servidor inicia
2. Cria fila POSIX
3. Cria shared memory
4. Cria threads cozinheiras
5. Clientes enviam pedidos via MQ
6. Servidor insere pedidos no buffer circular
7. Cozinheiros retira pedidos e processam
8. Painel lê shared memory e exibe progresso
9. CTRL+C → cleanup completo (unlink MQ/SHM)
```

---

# 🧹 Processo de Encerramento (Cleanup)

Quando o usuário pressiona **CTRL+C**:

```
signal(SIGINT) → cleanup()

cleanup:
    mq_close + mq_unlink
    munmap + shm_unlink
    destroy mutex/condvar
    destroy semáforos
    exit(0)
```

Evita vazamentos no:

- `/dev/mqueue`
- `/dev/shm`

---

# 🧠 Requisitos da Disciplina – Onde Cada Um Foi Atendido

| Requisito | Local |
|----------|-------|
| Threads POSIX | Servidor (cozinheiros) |
| Processos (fork/exec/wait) | Launcher |
| Message Queue POSIX | Cliente → Servidor |
| Shared Memory POSIX | Servidor ↔ Painel |
| Semáforos POSIX | Buffer circular |
| Mutex | Buffer e painel |
| CondVar | Buffer circular |
| Sinais | SIGINT → cleanup |
| Produtor–Consumidor | Fila interna |

---

# 🎯 Conclusão

Esta arquitetura demonstra, de forma integrada:

✔ Concorrência real entre processos e threads  
✔ Diversos mecanismos de IPC POSIX  
✔ Sincronização segura e bem estruturada  
✔ Arquitetura escalável e modular  
✔ Alta aderência ao conteúdo da disciplina  

---

