
# Guia de Contribuição — REV.IA

## 1. Sobre o projeto

REV.IA é um jogo educativo de plataforma 2D
desenvolvido em C11 utilizando Raylib.

## 2. Tecnologias necessárias

- GCC
- Raylib
- Make
- CMake
- Git

## 3. Preparação do ambiente

Instale o MSYS2 e configure o ambiente UCRT64.

Instale o GCC, Make, CMake e Raylib.

## 4. Como compilar

Na pasta raiz do projeto, execute:

make

## 5. Como executar

Execute o arquivo gerado pelo processo de compilação.

## 6. Como executar os testes

Na pasta raiz, execute:

make test

## 7. Padrão de commits

Utilize mensagens claras e objetivas:

feat: adiciona nova funcionalidade
fix: corrige erro no jogo
test: adiciona testes automatizados
docs: atualiza documentação
refactor: reorganiza o código

## 8. Padrão de desenvolvimento

- Respeitar a arquitetura atual do projeto.
- Evitar alterações desnecessárias em módulos de outros integrantes.
- Testar as alterações antes de enviar.
- Documentar novas funcionalidades.
- Criar commits pequenos e organizados.

## 9. Pull Requests

Toda alteração deve ser revisada antes de
ser incorporada à branch principal.
