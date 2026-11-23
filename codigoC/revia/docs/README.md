# REV.IA — DECIFRA.IA

Jogo de plataforma 2D em **C11 + Raylib** sobre alfabetização algorítmica. Você é o novo Decifrador do
Departamento de Alfabetização Algorítmica de uma cidade em que a IA generalista **NÓVA** decide trânsito,
crédito, notícias e saúde sem que ninguém entenda como. Investigue, ensine e tome decisões que mexem na
**Confiança Pública**.

> **Versão 3.0.** Os textos de diálogos, chamados, perguntas e finais são originais deste projeto. Para
> alterar o conteúdo, edite apenas `src/narrative/*.c` e `src/education/quiz.c` / `codex.c`: nenhuma outra
> parte do código depende do texto.

## Como jogar

| Ação | Teclado | Controle |
|---|---|---|
| Mover / mirar o dash | Setas ou WASD | Direcional ou analógico |
| Pular (segure para pular mais alto) | Z, Espaço ou K | A |
| Dash (8 direções) | X, Shift ou J | X ou B |
| Agarrar parede (segure) e subir/descer | C, Ctrl ou L + Cima/Baixo | RB ou RT |
| Interagir (falar, atender, usar terminal) | E ou Enter | Y |
| Pausa | Esc ou P | Start |
| Tela cheia | F11 | |

Em menus: setas para navegar, Z/Enter confirma, X/Esc volta.

**Mecânicas de movimento**
- Aceleração/desaceleração, pulo de altura variável e controle no ar; gravidade reduzida perto do ápice.
- **Dash** direcional (recarrega ao tocar o chão ou em um **cristal**).
- **Escalada** com energia limitada, **wall jump** e impulso automático ao chegar à borda.
- **Coyote time**, **jump buffering**, correção de quina no teto, respawn rápido no checkpoint.
- Dificuldades: **Normal**, **Assistido** (mais tolerância, mais energia, dash ilimitado, menos obstáculos e
  perguntas mais fáceis) e **Desafio** (menos tolerância, mais obstáculos e mais rápidos, perguntas mais difíceis).
- Todas as constantes de física ficam em `src/core/config.h` (`PH_*`) e `config.c` (dificuldade).

**Elementos de cenário que ensinam**
- Plataformas que **desmoronam** = dados incompletos (Crédito).
- Plataformas **ilusórias** que não sustentam = alucinações (Notícias).
- Espinhos = erros que custam caro (Saúde).
- **Chips de dados** dourados (3 por ato) desbloqueiam verbetes na **Enciclopédia**.
- **Bugs** andam pelo chão (é preciso pular por cima) e **dados corrompidos** caem do céu depois de um aviso
  piscando no chão. Tocar neles faz o Decifrador voltar ao último checkpoint.

**Personagens**: ao escolher "Novo jogo" abre a seleção entre **Alex, Bia, Caio e Dani** (muda as cores do
sprite). O personagem fica salvo com o progresso. O "Continuar" retorna ao **último checkpoint** da fase.

## Estrutura da campanha

| Ato | Fase | O que acontece |
|---|---|---|
| 1 Onboarding | Departamento | Ensina controles passo a passo; terminais explicam IA, dados e treinamento; **Avaliação 1** |
| 2 Rotina de Chamados | Distrito NÓVA (Trânsito → Crédito → Notícias → Saúde) | 4 chamados com 3–4 alternativas, **Avaliação 2** |
| 3 O Escândalo | Arquivo Sul | 3 evidências, **minigame de viés**, chamado decisivo, **Avaliação 3** |
| 4 Desfecho | Salão da Cidade | 2 decisões finais, **Avaliação 4**, e um dos **4 finais** conforme a Confiança Pública |

- **Avaliações**: 4 sessões de 5 perguntas, dificuldade crescente (a dificuldade do jogo muda o sorteio:
  Assistido usa as mais fáceis, Desafio as mais difíceis). Cada acerto vale **+2** e cada erro **−3** de
  Confiança Pública. Errar **não** reinicia: a pergunta conta como errada e a sessão segue até o fim, quando
  a soma é aplicada (só na primeira vez que a avaliação é concluída).
- **Chamados**: contexto, problema, alternativas, consequência, explicação educativa e impacto na confiança.
- **Minigame de viés**: monte o conjunto de treino (quantidade por bairro, uso de *bairro*, uso de *CEP*,
  correção dos rótulos históricos) e execute a simulação. O CEP é um *proxy*: tirar só a coluna "bairro" não
  basta quando o CEP continua; corrigir os rótulos históricos ataca a causa.
- **Confiança Pública** (0–100): afetada por chamados, avaliações e pelo minigame. Define o final:
  `>=75` e viés corrigido → *Cidade Decifrada*; `>=50` → *Reparo Lento*; `>=25` → *Desconfiança*; abaixo → *Silêncio*.
- **Salvamento**: arquivo `revia_save.dat` (versionado, com checksum, gravação atômica). Salva em checkpoints,
  chips, chamados, avaliações, ao pausar/sair. Guarda progresso, atos liberados, chamados e decisões,
  acertos, confiança, colecionáveis, final alcançado, mortes e tempo.

## Compilar e executar

Dependências: compilador C11 (GCC/Clang), `make` (ou CMake ≥ 3.16) e a **Raylib ≥ 4.5** (testado contra a
5.0). Python 3 só é necessário para regenerar os mapas (`make levels`).

### Windows (MSYS2 UCRT64)
1. Instale o MSYS2 (https://www.msys2.org) e abra o terminal **MSYS2 UCRT64**.
2. Instale as dependências:
   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib \
                      mingw-w64-ucrt-x86_64-pkgconf make
   ```
3. Na pasta do projeto: `make` e depois `./build/revia.exe` (ou `make run`).

### Linux
```bash
# Debian/Ubuntu (se o seu repositório não tiver libraylib-dev, compile a Raylib como abaixo)
sudo apt install build-essential git libasound2-dev libx11-dev libxrandr-dev libxi-dev \
                 libgl1-mesa-dev libxcursor-dev libxinerama-dev pkg-config
git clone --depth 1 --branch 5.0 https://github.com/raysan5/raylib.git
make -C raylib/src PLATFORM=PLATFORM_DESKTOP && sudo make -C raylib/src install
# Arch: sudo pacman -S raylib      Fedora: sudo dnf install raylib-devel
make && ./build/revia
```

### macOS
```bash
xcode-select --install
brew install raylib pkg-config
make && ./build/revia
```

### CMake (alternativa)
```bash
cmake -S . -B build-cmake && cmake --build build-cmake
./build-cmake/revia
```
Se a Raylib não estiver instalada, o CMake a baixa e compila (precisa de internet).
Se o `make` não encontrar a Raylib, use `make RAYLIB_DIR=/caminho/da/raylib`.

Execute a partir da pasta do projeto (ou de `build/`): o jogo procura `levels/` na pasta atual e nas
pastas acima do executável. O save é gravado na pasta atual.

## Estrutura do código

```
src/
  main.c                  só inicia e roda o jogo
  core/    game (estados, laço de passo fixo, transições) · config (constantes) · input · audio · types
  player/  player (movimento, dash, escalada) · physics (movimento com colisão por eixo)
  world/   level (carrega .txt) · collision · camera (suave, antecipação, tremor)
  entities/ checkpoints, cristais, colecionáveis, objetos interativos, saída
  narrative/ dialogue · missions (chamados) · decisions (finais)
  education/ quiz · bias_game · codex (enciclopédia)
  systems/ save · progression
  render/  palette · gfx · fx (partículas) · world_draw (fundo parallax, tiles, sprites)
  ui/      hud · menu (menu, seleção, config, pausa, créditos) · telas de diálogo/chamado/quiz/viés/enciclopédia/finais
levels/act1..act4/        mapas em texto (gerados por tools/gen_levels.py, editáveis à mão)
tests/                    testes de lógica + Raylib "de teste" para execução sem janela
tools/                    gerador de mapas e renderizador de capturas do stub
```

Separação: **regras** (player, world, entities, narrative, education, systems) não conhecem a Raylib e são
testáveis sozinhas; **renderização** (`render/`, `ui/`) só lê o estado; `core/` liga tudo. A física roda em
passo fixo de 1/60 s; o desenho vai para uma textura de 384×216 escalada para a janela.

### Editar fases
Os mapas ficam em `levels/actN/actN.txt` (formato documentado em `src/world/level.h`). Legenda:
`#` sólido · `^` espinhos · `~` plataforma que desmorona · `g` plataforma ilusória · `P` início · `C` checkpoint ·
`*` chip (exatamente 3 por fase) · `o` cristal · `E` saída · `1`–`9` objetos interativos ligados por linhas
`@int <dígito> <dialogue|call|quiz|bias> <id>`. Para regenerar: `make levels`.

## Verificação

```bash
make test    # 3800+ verificações de lógica, com AddressSanitizer/UBSan (não precisa da Raylib)
make smoke   # roda o jogo inteiro sem janela, 100 mil quadros de entradas aleatórias, com sanitizers
```
O que `make test` cobre: carregamento das 4 fases, altura/pulo, coyote time, vão de 5 tiles, dash, cristais
(vão de 12 tiles), escalada de 10 tiles, wall jump, espinhos, queda, plataformas que desmoronam, dificuldade,
quiz (acertos e erros somando a confiança, dificuldade, variações). `make smoke` visita todas as telas (incluindo os 4 atos, chamados, avaliações,
minigame e finais), sem erros de memória.

**Limitações conhecidas**
- A sensação do movimento foi validada por testes numéricos e pelos testes de jogo da equipe. Ajuste `PH_*`
  em `config.h` conforme o gosto.
- As fases são **completáveis por construção** (o gerador limita vãos, subidas e dash) e foram testadas em
  trechos sintéticos, mas **nenhum jogador (humano ou bot) percorreu os mapas inteiros**.
- A arte é procedural (pixel art desenhada por código) e o áudio é sintetizado por código. Não há sprites
  nem músicas em arquivo. Nada de Celeste foi copiado.
- A fonte é a padrão da Raylib (sem acentos fora do Latin-1, que o jogo não usa).
- Não há controle por mouse nos menus nem remapeamento de teclas.
