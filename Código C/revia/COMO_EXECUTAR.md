# Como baixar e executar o Rev.IA

Este tutorial explica como baixar e executar o **Rev.IA**, projeto da
equipe Decifra.IA, no Windows.

A versão disponibilizada neste repositório já está compilada. Por isso,
você **não precisa instalar Raylib, GCC, MSYS2 ou VS Code** para jogar.

## 1. Requisitos

-   Windows 10 ou Windows 11.
-   Os arquivos do jogo disponibilizados neste repositório.

## 2. Baixar o projeto

1.  Acesse o repositório: <https://github.com/lgc-s/decifra.ai>.
2.  Clique no botão verde **Code**.
3.  Selecione **Download ZIP**.
4.  Aguarde o download.
5.  Extraia o conteúdo do arquivo ZIP para uma pasta do computador.

> É importante extrair o ZIP antes de executar o jogo. Não execute o
> programa diretamente de dentro do arquivo compactado.

## 3. Localizar os arquivos do jogo

Depois de extrair o projeto, abra a pasta:

`Código C/executavel/revia`

Nela, devem estar estes três arquivos:

  -----------------------------------------------------------------------
  Arquivo                             Função
  ----------------------------------- -----------------------------------
  `revia.exe`                         Executável do jogo.

  `libraylib.dll`                     Biblioteca Raylib necessária para o
                                      funcionamento do jogo.

  `glfw3.dll`                         Biblioteca utilizada pela Raylib
                                      para gerenciamento da janela e de
                                      entradas.
  -----------------------------------------------------------------------

**Importante:** mantenha os três arquivos na mesma pasta. Não apague,
renomeie ou mova as DLLs para outro local, pois o executável precisa
delas para iniciar.

## 4. Executar o jogo

1.  Abra a pasta `revia`.
2.  Localize o arquivo `revia.exe`.
3.  Dê dois cliques nele.
4.  Aguarde a inicialização do jogo.

Pronto! O jogo deverá abrir.

## 5. Solução de problemas

### O Windows informa que está faltando uma DLL

Confira se `libraylib.dll` e `glfw3.dll` estão na mesma pasta que
`revia.exe`.

Se algum arquivo estiver faltando, baixe novamente o projeto e extraia o
ZIP por completo.

### O Windows exibe um aviso de segurança

O Windows pode exibir um aviso ao abrir um programa baixado da internet.
Antes de tomar qualquer decisão, confirme que você baixou os arquivos
deste repositório oficial e que eles não foram modificados.

### O jogo não inicia

-   Confirme que o ZIP foi extraído por completo.
-   Verifique se os três arquivos listados acima estão juntos na pasta
    `revia`.
-   Se o problema continuar, abra uma *issue* no repositório e informe a
    mensagem de erro exibida.

------------------------------------------------------------------------

**Desenvolvido pela equipe Decifra.IA --- CESAR School.**

Projeto Rev.IA: um jogo sobre alfabetização em Inteligência Artificial.
