# Rev.IA — Decifra.IA

> Um jogo de plataforma 2D sobre alfabetização em Inteligência Artificial. **Versão 3.0**

Rev.IA (nome do projeto acadêmico) é um jogo de plataforma 2D, feito em **C com Raylib**, cujo objetivo é ensinar, de forma prática e narrativa, como funcionam — e onde falham — os sistemas de Inteligência Artificial presentes no dia a dia: viés algorítmico, alucinações, decisões automatizadas e uso seguro de ferramentas de IA generativa.

O jogo é desenvolvido como projeto da disciplina **Projetos 2** (2º período de ADS, turma **ADS - Embarque Digital**) da **CESAR School**, integrando também os componentes de Lógica, IHC/HCI e FDS/FP2.

---

## Descrição

Você é recém-contratado(a) do **Departamento de Alfabetização Algorítmica** de uma cidade fictícia onde uma IA generalista chamada **NÓVA** foi implantada silenciosamente em quase todos os serviços públicos — trânsito, crédito, notícias, saúde. Ninguém entende bem como ela decide.

Seu cargo, **Decifrador(a)**, existe para auditar as decisões da NÓVA e educar cidadãos que abrem chamados de reclamação sobre ela. Ao começar um novo jogo, você escolhe o seu personagem: **Alex, Bia, Caio ou Dani**.

### Equipe

| Nome | LinkedIn |
| :--- | :--- |
| **Davi Braz** | [Acessar Perfil](https://www.linkedin.com/in/davibrazs) |
| **Davi Pereira** | [Acessar Perfil](https://www.linkedin.com/in/davi-pereira-0040b53b7) |
| **Carlos Daniel** | [Acessar Perfil](https://www.linkedin.com/in/carlos-daniel2) |
| **Antonio** | [Acessar Perfil](https://www.linkedin.com/in/antonio-de-oliveira-chaves-neto-7a71803b7) |
| **Nalbert** | [Acessar Perfil](https://www.linkedin.com/in/nalbert-gonçalves-2a764b375) |
| **João** | [Acessar Perfil](https://www.linkedin.com/in/joaovictortvs) |
| **Luis** | [Acessar Perfil](https://www.linkedin.com/in/lgc-s) |
| **Kauã** | [Acessar Perfil](https://www.linkedin.com/in/kauã-mateus-alves-a499442ba/) |


### Progresso Semanal

| Semana | Atividade                    | Responsável         | Matéria  |
| :---   | :---                         | :---                | :---     |
| 1      | Sem Atividade (Apresentação) | *Nenhum*            |*Nenhum*  |
| 2-3    | Project Model Canvas         | Nalbert  e Davi P   |   IHC    |
| 2-3    | Ideação do Projeto           | Todos               |   FP2    |
| 2-3    | Personas                     | Davi P e Nalbert    |   IHC    |
| 2-3    | Visão e Requsitos            | Luis e Davi B       |   FDS    |
| 4-5    | Setup do GitHub              | Luis e Davi B       |   PIF    |
| 4-5    | Rascunhos da Tela de Jogo    | João e Carlos       |   IHC    |
| 4-5    | Código Inicial               | Kauã  e Antonio     |   PIF    |
| 6-7    | Regras formalizadas          | Antônio e Kauã      |   LMC    |
| 6-7    | Núcleo jogável em C          | Carlos e João       |   PIF    | 
| 6-7    | Regras formalizadas          | Antônio e Kauã      |   LMC    |
| 6-7    | Backlog                      | João e Carlos       |   FP2    |
| 6-7    | Wireframes                   | Nalbert e Davi P    |   FP2    |
| 8-9    | Entrega u1 PIF               | Carlos e João       |   PIF    |
| 8-9    | Entrega u1 IHC               | Davi P e Nalbert    |   IHC    |
| 8-9    | Entrega u1 LMC               | Antonio e Kauã      |   LMC    |
| 8-9    | Entrega u1  FDS              | Davi B e Luis       |   FDS    |
| 8-9    | Entrega u1  FP2              | Kauã e Antonio      |   FP2    |
| 10-11  | Backlog revisado             | João e Carlos       |   FP2    |
| 10-11  | Evolução do código           | Luis e Davi B       |   PIF    |
| 11-12  | Protótipo jogável            | Carlos e Joao       |   PIF    |
| 11-12  | Ajuda na prototipação        |  Luis  e Davi P     |   PIF    |
| 11-12  | Teste de usabilidade         |  Kawan e Antonio G  |   PIF    |
| 11-12  | Teste de uso                 | Carlos Davi         |   PIF    |

### Estrutura narrativa em atos

| Ato | Fase | Descrição |
|---|---|---|
| 1 — Onboarding | Departamento | Tutorial disfarçado de capacitação: o jogador aprende os controles e os terminais explicam IA, dados e treinamento. Termina com a **Avaliação 1** |
| 2 — Rotina de chamados | Distrito NÓVA (Trânsito, Crédito, Notícias, Saúde) | Quatro chamados de cidadãos, com cenários de decisão de custo/benefício não óbvio. Termina com a **Avaliação 2** |
| 3 — O escândalo | Arquivo Sul | O jogador reúne 3 evidências de viés discriminatório, joga o **mini-jogo de viés** e toma a decisão do chamado decisivo. Termina com a **Avaliação 3** |
| 4 — Desfecho | Salão da Cidade | Duas decisões finais e a **Avaliação 4**. A Confiança Pública define um dos **4 finais**: Cidade Decifrada, Reparo Lento, Desconfiança ou Silêncio |

### Mecânicas principais

- **Plataforma com movimento preciso** — pulo de altura variável, dash em 8 direções, escalada com energia limitada e wall jump
- **Obstáculos que ensinam** — plataformas que desmoronam (dados incompletos), plataformas ilusórias (alucinações), espinhos (erros que custam caro), *bugs* que andam pelo chão e *dados corrompidos* que caem do céu depois de um aviso
- **Avaliações (quizzes conceituais)** — 4 sessões de 5 perguntas com dificuldade crescente. Cada acerto vale **+2** e cada erro vale **−3** de Confiança Pública; errar **não** reinicia a sessão, o jogador termina a avaliação e vê a soma ao final
- **Chamados (tickets)** — cenários de decisão com 3-4 opções de resposta, cada uma com custo/benefício não óbvio e uma explicação educativa
- **Medidor de Confiança Pública (0-100)** — reflete o impacto acumulado das decisões, das avaliações e do mini-jogo
- **Mini-jogo de viés** — o jogador monta o conjunto de treino da NÓVA (casos do Bairro Sul, uso do bairro, uso do CEP, correção do histórico) e observa o viés se manifestar nos resultados simulados
- **Enciclopédia** — chips de dados escondidos nas fases desbloqueiam verbetes sobre IA
- **Personagens e dificuldades** — quatro personagens jogáveis e três dificuldades (**Normal**, **Assistido** e **Desafio**). Em dificuldades mais altas, as fases têm mais obstáculos, mais rápidos, e as perguntas sorteadas são mais difíceis

---

## Personas

O design do jogo é guiado por dois perfis-alvo principais:

**Camila Souza Albuquerque, 34** — empreendedora, dona de uma pequena loja online. Tem relação intimidada com IA ("acha que é coisa de programador"). Quer entender como o viés algorítmico afeta o alcance dos seus anúncios e como usar IA para gerar imagens com segurança.

**Roberto de Lima Barreto, 21** — estudante universitário e estagiário. Relação imediatista com IA: usa para resumir textos e confia cegamente nas respostas, o que já lhe custou notas baixas. Precisa aprender a identificar alucinações da IA e a escrever prompts mais precisos.

---

## Requisitos

### Requisitos Funcionais (RF)
- Narrativa com começo, meio e fim que gere engajamento
- Interface simples e acessível, voltada também a públicos não-nativos digitais
- Tutorial, créditos, configurações e dados salvos
- Escolha de personagem ao começar um novo jogo
- Sessões de história entre as fases; gameplay primário baseado em plataforma e em desafios conceituais e contextuais
- 4 atos (fases) e 4 sessões de perguntas (5 questões cada, dificuldade crescente)
- Acertos aumentam a Confiança Pública e erros a reduzem; o jogador sempre termina a avaliação (não há reinício na pergunta 1)
- Dificuldade selecionável (Normal, Assistido, Desafio), que altera os obstáculos das fases e a dificuldade das perguntas

### Requisitos Não Funcionais (RNF)
- Desenvolvido em **C (C11)** com a biblioteca **Raylib**, visando estabilidade e ausência de conflitos técnicos
- Salvamento de progresso (inclui o último checkpoint e o personagem) e feedback para todas as respostas do usuário, corretas ou não
- Compatível com Windows
- Tela de resultado final com Confiança Pública, decisões, tempo de jogo e quantidade de quedas

---

## Como jogar (Windows)

1. Abra a página [**Releases**](https://github.com/lgc-s/decifra.ai/releases) deste repositório e baixe o arquivo `.zip` da versão mais recente.
   Também é possível baixar o pacote mais novo na aba **Actions**, na última execução, em *Artifacts* → `REVIA-Windows`.
2. Extraia a pasta **inteira** (não mova o `revia.exe` sozinho).
3. Dê dois cliques em `revia.exe`.

| Ação | Teclado |
|---|---|
| Mover | Setas ou A / D |
| Pular (segure para pular mais alto) | Z, Espaço ou K |
| Dash (8 direções) | X, Shift ou J |
| Agarrar a parede e escalar | C, Ctrl ou L |
| Interagir (falar, atender, usar terminal) | E ou Enter |
| Pausar | Esc ou P |
| Tela cheia | F11 |

Nos menus: setas para navegar, Z/Enter confirma, X/Esc volta. O jogo também aceita controle (gamepad).

---

## Stack técnica

- **C (C11)** — jogo inteiro: laço de passo fixo, física, fases, narrativa e interface
- **Raylib** — janela, entrada, desenho e áudio. A arte (pixel art) e o áudio são gerados por código
- **Make / CMake** — compilação. As regras do jogo ficam separadas da interface e têm testes automáticos sem janela (`make test`)
- Sem banco de dados: persistência via arquivo binário (struct serializada, com número mágico e checksum para validar integridade)
- **GitHub Actions** — compila o jogo para Windows a cada push e gera o pacote `REVIA-Windows`
- Sem multiplayer nem rede

---

## Como Instalar e Rodar

📖 [Acesse o guia completo de compilação e execução (Windows, Linux e macOS)](./codigoC/revia/docs/README.md)

---

## Diagramas de Atividades

📊 [Ver diagramas de atividades de cada história de usuário](./diagrama-atividades.pdf)

---

## Metodologia

O desenvolvimento segue histórias de usuário no formato **3C (Card, Conversation, Confirmation)**, validadas pelo critério **INVEST**, organizadas em um board Scrum (Jira) com sprints alinhadas aos 4 atos do jogo:

- Sprint 1 — Ato 1 / Onboarding
- Sprint 2 — Ato 2 / Rotina de Chamados
- Sprint 3 — Ato 3 / Escândalo
- Sprint 4 — Ato 4 / Desfecho

---

## Demonstração

[![Assistir ao screencast do protótipo](https://img.youtube.com/vi/Xda-MFSF_Vs/0.jpg)](https://www.youtube.com/watch?v=Xda-MFSF_Vs)

🎥 [Assistir ao screencast do protótipo](https://www.youtube.com/watch?v=Xda-MFSF_Vs)

## SPRINT1 E BACKLOG


<img width="287" height="786" alt="backlog" src="https://github.com/user-attachments/assets/5ae978bf-23c4-4ce0-9d86-7a98be9b0999"/>


<img width="1600" height="508" alt="sprints" src="https://github.com/user-attachments/assets/c018fdfd-5b19-493d-8934-afd76db72780" />

## Screencast Sprint 1

https://www.youtube.com/watch?v=LN18BlokvUo
